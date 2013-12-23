/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Ivan Komissarov
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdriveinfo_p.h"

#include <QtCore/QDirIterator>
#include <QtCore/QTextStream>

#include <errno.h>
#include <mntent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <private/qcore_unix_p.h>

#if defined(QT_LARGEFILE_SUPPORT)
#  define QT_STATFSBUF struct statvfs64
#  define QT_STATFS    ::statvfs64
#else
#  define QT_STATFSBUF struct statvfs
#  define QT_STATFS    ::statvfs
#endif

static const char pathMounted[] = "/etc/mtab";
static const char pathDiskByLabel[] = "/dev/disk/by-label";

static bool isPseudoFs(const QString &mountDir, const QByteArray &type)
{
    if (type.startsWith('/'))
        return false;
    if (mountDir.startsWith(QStringLiteral("/dev"))
            || mountDir.startsWith(QStringLiteral("/proc")))
        return true;
    if (type == "rootfs"
            || type == "none"
            || type == "proc"
            || type == "tmpfs"
            || type == "sysfs"
            || type == "usbfs"
            || type == "cgroup"
            || type == "cpuset"
            || type == "rpc_pipefs"
            || type == "devpts"
            || type == "securityfs"
            || type == "debugfs"
            || type == "fusectl"
            || type == "binfmt_misc")
        return true;

    return false;
}

void QDriveInfoPrivate::initRootPath()
{
    if (rootPath.isEmpty())
        return;

    FILE *fp = ::setmntent(pathMounted, "r");
    if (fp) {
        quint32 maxLength = 0;
        const QString oldRootPath = rootPath;
        rootPath.clear();

        struct mntent *mnt;
        while ((mnt = ::getmntent(fp))) {
            const QString mountDir = QFile::decodeName(mnt->mnt_dir);
            const QByteArray mountType(mnt->mnt_type);
            if (isPseudoFs(mountDir, mountType))
                continue;
            // we try to find most suitable entry
            if (oldRootPath.startsWith(mountDir) && maxLength < (quint32)mountDir.length()) {
                maxLength = mountDir.length();
                rootPath = mountDir;
                device = QByteArray(mnt->mnt_fsname);
                fileSystemName = mountType;
            }
        }
        ::endmntent(fp);
    }
}

static inline QDriveInfo::DriveType determineType(const QByteArray &device)
{
    QString dmFile;
    if (device.contains("mapper")) {
        QT_STATBUF stat_buf;
        int result;
        EINTR_LOOP(result, QT_STAT(device.constData(), &stat_buf));
        if (result == 0)
            dmFile = QStringLiteral("dm-") + QString::number(stat_buf.st_rdev & 0377);
    } else {
        dmFile = QString::fromLatin1(device).section(QLatin1Char('/'), 2, 3);
        if (dmFile.startsWith(QStringLiteral("mmc"))) {
            // assume this dev is removable sd/mmc card.
            return QDriveInfo::RemovableDrive;
        }

        if (dmFile.length() > 3) {
            // if device has number, we need the 'parent' device
            dmFile.chop(1);
            if (dmFile.endsWith(QStringLiteral("p")))
                dmFile.chop(1); // get rid of partition number
        }
    }
    if (!dmFile.isEmpty()) {
        dmFile = QStringLiteral("/sys/block/") + dmFile + QStringLiteral("/removable");

        QFile file(dmFile);
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream sysinfo(&file); // ### can we get rid of QTextStream ?
            const QString line = sysinfo.readAll();
            if (line.contains(QLatin1Char('1')))
                return QDriveInfo::RemovableDrive;
        }

        if (device.startsWith("/dev"))
            return QDriveInfo::InternalDrive;
    }

    return QDriveInfo::UnknownDrive;
}

// Unfortunately, I don't know other way to get labels without root privileges.
// Maybe libudev can provide this information. TODO: explore it
// If we have udev installed in the system, proper symlinks are created by it,
// so we don't need to link to libudev.
static inline QString getName(const QByteArray &device)
{
    QDirIterator it(QLatin1String(pathDiskByLabel), QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo(it.fileInfo());
        if (fileInfo.isSymLink() && fileInfo.symLinkTarget().toLatin1() == device)
            return fileInfo.fileName();
    }

    return QString();
}

void QDriveInfoPrivate::doStat(uint requiredFlags)
{
    if (getCachedFlag(requiredFlags))
        return;

    if (requiredFlags & CachedCapabilitiesFlag) // force fs name
        requiredFlags |= CachedFileSystemNameFlag;

    if (!getCachedFlag(CachedRootPathFlag | CachedDeviceFlag | CachedFileSystemNameFlag)) {
        initRootPath();
        setCachedFlag(CachedRootPathFlag | CachedDeviceFlag | CachedFileSystemNameFlag);
    }

    if (rootPath.isEmpty() || (getCachedFlag(CachedValidFlag) && !valid))
        return;

    if (!getCachedFlag(CachedValidFlag))
        requiredFlags |= CachedValidFlag; // force drive validation


    uint bitmask = 0;

    bitmask = CachedBytesTotalFlag | CachedBytesFreeFlag | CachedBytesAvailableFlag |
              CachedReadOnlyFlag | CachedReadyFlag | CachedValidFlag;
    if (requiredFlags & bitmask) {
        getVolumeInfo();
        setCachedFlag(bitmask);

        if (!valid)
            return;
    }

    bitmask = CachedNameFlag;
    if (requiredFlags & bitmask) {
        name = getName(device);
        setCachedFlag(bitmask);
    }

    bitmask = CachedCapabilitiesFlag;
    if (requiredFlags & bitmask) {
        getCapabilities();
        setCachedFlag(bitmask);
    }

    bitmask = CachedTypeFlag;
    if (requiredFlags & bitmask) {
        type = determineType(device);
        if (type == QDriveInfo::UnknownDrive) {
            // test for UNC shares
            if (rootPath.startsWith(QStringLiteral("//"))
                || fileSystemName == "nfs"
                || fileSystemName == "cifs"
                || fileSystemName.startsWith("smb")) {
                type = QDriveInfo::RemoteDrive;
            }
        }
        setCachedFlag(bitmask);
    }
}

void QDriveInfoPrivate::getVolumeInfo()
{
    QT_STATFSBUF statfs_buf;
    int result;
    EINTR_LOOP(result, QT_STATFS(QFile::encodeName(rootPath).constData(), &statfs_buf));
    if (result == 0) {
        valid = true;
        ready = true;

        bytesTotal = statfs_buf.f_blocks * statfs_buf.f_bsize;
        bytesFree = statfs_buf.f_bfree * statfs_buf.f_bsize;
        bytesAvailable = statfs_buf.f_bavail * statfs_buf.f_bsize;

        readOnly = (statfs_buf.f_flag & ST_RDONLY) != 0;
    }
}

void QDriveInfoPrivate::getCapabilities()
{
    uint flags = 0;

    const QByteArray fileSystem = fileSystemName.toLower();
    if (fileSystem == "ext4"
            || fileSystem == "ext3"
            || fileSystem == "ext3cow"
            || fileSystem == "xfs"
            || fileSystem == "jfs"
            || fileSystem == "ntfs"
            || fileSystem == "reiserfs"
            || fileSystem == "hfsplus")
        flags = QDriveInfo::SupportsSymbolicLinks
                | QDriveInfo::SupportsHardLinks
                | QDriveInfo::SupportsCaseSensitiveNames
                | QDriveInfo::SupportsCasePreservedNames
                | QDriveInfo::SupportsJournaling
                | QDriveInfo::SupportsSparseFiles;
    else if (fileSystem == "ext2"
             || fileSystem == "btrfs"
             || fileSystem == "reiser4"
             || fileSystem == "zfs")
        flags = QDriveInfo::SupportsSymbolicLinks
                | QDriveInfo::SupportsHardLinks
                | QDriveInfo::SupportsCaseSensitiveNames
                | QDriveInfo::SupportsCasePreservedNames
                | QDriveInfo::SupportsSparseFiles;
    else if (fileSystem == "ntfs-3g")
        flags = QDriveInfo::SupportsSparseFiles;
    else if (fileSystem == "fat32"
             || fileSystem == "vfat"
             || fileSystem == "fat16"
             || fileSystem == "fat12")
        flags = 0;
    else if (fileSystem == "exfat")
        flags = QDriveInfo::SupportsCasePreservedNames;
    else if (fileSystem == "hfs")
        flags = QDriveInfo::SupportsSymbolicLinks
                | QDriveInfo::SupportsCasePreservedNames
                | QDriveInfo::SupportsSparseFiles;
    else if (fileSystem == "nfs"
             || fileSystem == "cifs"
             || fileSystem.startsWith("smb"))
        flags = 0;

    capabilities = QDriveInfo::Capabilities(flags);
}


QList<QDriveInfo> QDriveInfoPrivate::drives()
{
    QList<QDriveInfo> drives;

    FILE *fp = ::setmntent(pathMounted, "r");
    if (fp) {
        struct mntent *mnt;
        while ((mnt = ::getmntent(fp))) {
            QByteArray mountType(mnt->mnt_type);
            if (isPseudoFs(mountType))
                continue;

            QDriveInfoPrivate *data = new QDriveInfoPrivate;
            data->rootPath = QFile::decodeName(mnt->mnt_dir);
            data->device = QByteArray(mnt->mnt_fsname);
            data->fileSystemName = mountType;
            data->setCachedFlag(CachedRootPathFlag |
                               CachedFileSystemNameFlag |
                               CachedDeviceFlag);
            drives.append(QDriveInfo(*data));
        }
        ::endmntent(fp);
    }

    return drives;
}

QDriveInfo QDriveInfoPrivate::rootDrive()
{
    return QDriveInfo(QStringLiteral("/"));
}
