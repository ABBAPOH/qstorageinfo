/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2014 Ivan Komissarov
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
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>

#include <errno.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#if defined(Q_OS_BSD4)
#  include <sys/mount.h>
#elif defined(Q_OS_LINUX)
#  include <mntent.h>
#elif defined(Q_OS_SOLARIS)
#  include <sys/mnttab.h>
#endif

#include <QtCore/private/qcore_unix_p.h>

#ifdef Q_OS_BSD4
#  define QT_STATFSBUF struct statvfs
#  define QT_STATFS    ::statvfs
#else
#  if defined(QT_LARGEFILE_SUPPORT)
#    define QT_STATFSBUF struct statvfs64
#    define QT_STATFS    ::statvfs64
#  else
#    define QT_STATFSBUF struct statvfs
#    define QT_STATFS    ::statvfs
#  endif // QT_LARGEFILE_SUPPORT
#endif // Q_OS_BSD4

QT_BEGIN_NAMESPACE

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
            || type == "fuse.gvfsd-fuse"
            || type == "binfmt_misc"
            || type.startsWith("fuse.vmware"))
        return true;

    return false;
}

class DriveIterator
{
public:
    DriveIterator();
    ~DriveIterator();

    bool isValid() const;
    bool next();
    QString rootPath() const;
    QByteArray fileSystemName() const;
    QByteArray device() const;
private:
#if defined(Q_OS_BSD4)
    struct statfs *stat_buf;
    int count;
    int i;
#elif defined(Q_OS_LINUX)
    FILE *fp;
    struct mntent *mnt;
#elif defined(Q_OS_SOLARIS)
    FILE *fp;
    struct mnttab mnt;
#endif
};

#if defined(Q_OS_BSD4)

inline DriveIterator::DriveIterator():
    count(getmntinfo(&stat_buf, 0)),
    i(-1)
{
}

inline DriveIterator::~DriveIterator()
{
}

inline bool DriveIterator::isValid() const
{
    return count != -1;
}

inline bool DriveIterator::next()
{
    return ++i < count;
}

inline QString DriveIterator::rootPath() const
{
    return QFile::decodeName(stat_buf[i].f_mntonname);
}

inline QByteArray DriveIterator::fileSystemName() const
{
    return QByteArray(stat_buf[i].f_fstypename);
}

inline QByteArray DriveIterator::device() const
{
    return QByteArray(stat_buf[i].f_mntfromname);
}

#elif defined(Q_OS_LINUX)

static const char pathMounted[] = "/etc/mtab";

inline DriveIterator::DriveIterator():
    fp(::setmntent(pathMounted, "r"))
{
}

inline DriveIterator::~DriveIterator()
{
    if (fp)
        ::endmntent(fp);
}

inline bool DriveIterator::isValid() const
{
    return fp != 0;
}

inline bool DriveIterator::next()
{
    return (mnt = ::getmntent(fp));
}

inline QString DriveIterator::rootPath() const
{
    return QFile::decodeName(mnt->mnt_dir);
}

inline QByteArray DriveIterator::fileSystemName() const
{
    return QByteArray(mnt->mnt_type);
}

inline QByteArray DriveIterator::device() const
{
    return QByteArray(mnt->mnt_fsname);
}

#elif defined(Q_OS_SOLARIS)

static const char pathMounted[] = "/etc/mnttab";

inline DriveIterator::DriveIterator():
    fp(::fopen(pathMounted, "r"))
{
}

inline DriveIterator::~DriveIterator()
{
    if (fp)
        ::fclose(fp);
}

inline bool DriveIterator::isValid() const
{
    return fp != 0;
}

inline bool DriveIterator::next()
{
    return (getmntent(fp, &mnt) == 0);
}

inline QString DriveIterator::rootPath() const
{
    return QFile::decodeName(mnt->mnt_mountp);
}

inline QByteArray DriveIterator::fileSystemName() const
{
    return QByteArray(mnt->mnt_fstype);
}

inline QByteArray DriveIterator::device() const
{
    return QByteArray(mnt->mnt_mntopts);
}

#else // no information available

inline DriveIterator::DriveIterator()
{
}

inline DriveIterator::~DriveIterator()
{
}

inline bool DriveIterator::isValid() const
{
    return false;
}

inline bool DriveIterator::next()
{
    return false;
}

inline QString DriveIterator::rootPath() const
{
    return QString();
}

inline QByteArray DriveIterator::fileSystemName() const
{
    return QByteArray();
}

inline QByteArray DriveIterator::device() const
{
    return QByteArray();
}
#endif

void QDriveInfoPrivate::initRootPath()
{
    rootPath = QFileInfo(rootPath).canonicalFilePath();

    if (rootPath.isEmpty())
        return;

    DriveIterator it;
    if (!it.isValid()) {
        rootPath = QStringLiteral("/");
        return;
    }

    int maxLength = 0;
    const QString oldRootPath = rootPath;
    rootPath.clear();

    while (it.next()) {
        const QString mountDir = it.rootPath();
        const QByteArray fsName = it.fileSystemName();
        if (isPseudoFs(mountDir, fsName))
            continue;
        // we try to find most suitable entry
        if (oldRootPath.startsWith(mountDir) && maxLength < mountDir.length()) {
            maxLength = mountDir.length();
            rootPath = mountDir;
            device = it.device();
            fileSystemName = fsName;
        }
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
#ifdef Q_OS_LINUX
    QDirIterator it(QLatin1String(pathDiskByLabel), QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo(it.fileInfo());
        if (fileInfo.isSymLink() && fileInfo.symLinkTarget().toLatin1() == device)
            return fileInfo.fileName();
    }
#else
    Q_UNUSED(device);
#endif

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

    bitmask = CachedBytesTotalFlag | CachedBytesFreeFlag | CachedBytesAvailableFlag
            | CachedReadOnlyFlag | CachedReadyFlag | CachedValidFlag;
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
                    || fileSystemName == "autofs"
                    || fileSystemName == "subfs"
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
    else if (fileSystem == "ntfs-3g"
             || fileSystem.contains("fuse.ntfs")
             || fileSystem.contains("fuseblk.ntfs")
             || fileSystem == "fuseblk")
        flags = QDriveInfo::SupportsSparseFiles;
    else if (fileSystem == "fat32"
             || fileSystem == "vfat"
             || fileSystem == "fat16"
             || fileSystem == "fat12"
             || fileSystem == "msdos")
        flags = 0;
    else if (fileSystem == "exfat")
        flags = QDriveInfo::SupportsCasePreservedNames;
    else if (fileSystem == "hfs")
        flags = QDriveInfo::SupportsSymbolicLinks
                | QDriveInfo::SupportsCasePreservedNames
                | QDriveInfo::SupportsSparseFiles;
    else if (fileSystem == "nfs"
             || fileSystem == "cifs"
             || fileSystem.startsWith("smb")
             || fileSystem == "autofs"
             || fileSystem == "subfs")
        flags = 0;

    capabilities = QDriveInfo::Capabilities(flags);
}

QList<QDriveInfo> QDriveInfoPrivate::drives()
{
    DriveIterator it;
    if (!it.isValid())
        return QList<QDriveInfo>() << rootDrive();

    QList<QDriveInfo> drives;

    while (it.next()) {
        const QString mountDir = it.rootPath();
        const QByteArray fsName = it.fileSystemName();
        if (isPseudoFs(mountDir, fsName))
            continue;

        QDriveInfoPrivate *data = new QDriveInfoPrivate;
        data->rootPath = mountDir;
        data->device = QByteArray(it.device());
        data->fileSystemName = fsName;
        data->setCachedFlag(CachedRootPathFlag |
                            CachedFileSystemNameFlag |
                            CachedDeviceFlag);
        drives.append(QDriveInfo(*data));
    }

    return drives;
}

QDriveInfo QDriveInfoPrivate::rootDrive()
{
    return QDriveInfo(QStringLiteral("/"));
}

QT_END_NAMESPACE
