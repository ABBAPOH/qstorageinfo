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

#include "qvolumeinfo_p.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QVarLengthArray>

#include <userenv.h>

QT_BEGIN_NAMESPACE

void QVolumeInfoPrivate::initRootPath()
{
    rootPath = QFileInfo(rootPath).canonicalFilePath();

    if (rootPath.isEmpty())
        return;

    QString path = QDir::toNativeSeparators(rootPath);
    rootPath.clear();

    if (path.startsWith(QStringLiteral("\\\\?\\")))
        path.remove(0, 4);
    if (path.length() < 2 || path.at(1) != QLatin1Char(':'))
        return;
    path[0] = path[0].toUpper();
    if (!(path.at(0).unicode() >= 'A' && path.at(0).unicode() <= 'Z'))
        return;
    if (!path.endsWith(QLatin1Char('\\')))
        path.append(QLatin1Char('\\'));

    // ### test if disk mounted to folder on other disk
    wchar_t buffer[MAX_PATH + 1];
    if (::GetVolumePathName(reinterpret_cast<const wchar_t *>(path.utf16()), buffer, MAX_PATH))
        rootPath = QDir::fromNativeSeparators(QString::fromWCharArray(buffer));
}

static inline QByteArray getDevice(QVolumeInfo::VolumeType type, const QString &rootPath)
{
    const QString path = QDir::toNativeSeparators(rootPath);
    if (type != QVolumeInfo::RemoteVolume) {
        wchar_t deviceBuffer[MAX_PATH + 1];
        if (::GetVolumeNameForVolumeMountPoint(reinterpret_cast<const wchar_t *>(path.utf16()),
                                               deviceBuffer,
                                               MAX_PATH))
            return QString::fromWCharArray(deviceBuffer).toLatin1();
    } else {
        wchar_t buffer[1024];
        UNIVERSAL_NAME_INFO *remoteNameInfo = reinterpret_cast<UNIVERSAL_NAME_INFO *>(buffer);
        DWORD bufferLength = 1024;
        if (::WNetGetUniversalName(reinterpret_cast<const wchar_t *>(path.utf16()),
                                   UNIVERSAL_NAME_INFO_LEVEL,
                                   remoteNameInfo,
                                   &bufferLength) == NO_ERROR) {
            return QString::fromWCharArray(remoteNameInfo->lpUniversalName).toUtf8();
        }
    }

    return QByteArray();
}

static inline QVolumeInfo::VolumeType determineType(const QString &rootPath)
{
#if !defined(Q_OS_WINCE)
    UINT result = ::GetDriveType(reinterpret_cast<const wchar_t *>(rootPath.utf16()));
    switch (result) {
    case DRIVE_REMOVABLE:
        return QVolumeInfo::RemovableVolume;
    case DRIVE_FIXED:
        return QVolumeInfo::InternalVolume;
    case DRIVE_REMOTE:
        return QVolumeInfo::RemoteVolume;
    case DRIVE_CDROM:
        return QVolumeInfo::OpticalVolume;
    case DRIVE_RAMDISK:
        return QVolumeInfo::RamVolume;
    case DRIVE_UNKNOWN:
    case DRIVE_NO_ROOT_DIR:
    // fall through
    default:
        break;
    };
#else
    Q_UNUSED(rootPath)
#endif
    return QVolumeInfo::UnknownVolume;
}

void QVolumeInfoPrivate::doStat(uint requiredFlags)
{
    if (getCachedFlag(requiredFlags))
        return;

    if (!getCachedFlag(CachedRootPathFlag)) {
        initRootPath();
        setCachedFlag(CachedRootPathFlag);
    }

    if (rootPath.isEmpty() || (getCachedFlag(CachedValidFlag) && !valid))
        return;

    if (!getCachedFlag(CachedValidFlag))
        requiredFlags |= CachedValidFlag; // force volume validation

    uint bitmask = 0;

    bitmask = CachedFileSystemNameFlag | CachedNameFlag
            | CachedReadOnlyFlag | CachedReadyFlag | CachedValidFlag;
    if (requiredFlags & bitmask) {
        getVolumeInfo();
        if (valid && !ready)
            bitmask = CachedValidFlag;
        setCachedFlag(bitmask);

        if (!valid)
            return;
    }

    bitmask = CachedDeviceFlag | CachedTypeFlag;
    if (requiredFlags & bitmask) {
        type = determineType(rootPath);
        device = getDevice(QVolumeInfo::VolumeType(type), rootPath);
        setCachedFlag(bitmask);
    }

    bitmask = CachedBytesTotalFlag | CachedBytesFreeFlag | CachedBytesAvailableFlag;
    if (requiredFlags & bitmask) {
        getDiskFreeSpace();
        setCachedFlag(bitmask);
    }
}

void QVolumeInfoPrivate::getVolumeInfo()
{
    const UINT oldmode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    const QString path = QDir::toNativeSeparators(rootPath);
    wchar_t nameBuf[MAX_PATH + 1];
    DWORD fileSystemFlags = 0;
    wchar_t fileSystemNameBuf[MAX_PATH + 1];
    const bool result = ::GetVolumeInformation(reinterpret_cast<const wchar_t *>(path.utf16()),
                                               nameBuf, MAX_PATH,
                                               0, 0,
                                               &fileSystemFlags,
                                               fileSystemNameBuf, MAX_PATH);
    if (!result) {
        ready = false;
        valid = ::GetLastError() == ERROR_NOT_READY;
    } else {
        ready = true;
        valid = true;

        fileSystemName = QString::fromWCharArray(fileSystemNameBuf).toLatin1();
        name = QString::fromWCharArray(nameBuf);

        readOnly = (fileSystemFlags & FILE_READ_ONLY_VOLUME) != 0;

        capabilities = 0;
        if (fileSystemFlags & FILE_SUPPORTS_OBJECT_IDS) // ?
            capabilities |= QVolumeInfo::SupportsPersistentIDs;
        if (fileSystemName.toLower() == "ntfs") // ###
            capabilities |= QVolumeInfo::SupportsSymbolicLinks;
        if (fileSystemFlags & FILE_SUPPORTS_HARD_LINKS)
            capabilities |= QVolumeInfo::SupportsHardLinks;
        if (fileSystemFlags & FILE_SUPPORTS_USN_JOURNAL) // ?
            capabilities |= QVolumeInfo::SupportsJournaling;
        if (fileSystemFlags & FILE_SUPPORTS_SPARSE_FILES)
            capabilities |= QVolumeInfo::SupportsSparseFiles;
        if (fileSystemFlags & FILE_CASE_SENSITIVE_SEARCH)
            capabilities |= QVolumeInfo::SupportsCaseSensitiveNames;
        if (fileSystemFlags & FILE_CASE_PRESERVED_NAMES)
            capabilities |= QVolumeInfo::SupportsCasePreservedNames;
    }

    ::SetErrorMode(oldmode);
}

void QVolumeInfoPrivate::getDiskFreeSpace()
{
    const UINT oldmode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    const QString path = QDir::toNativeSeparators(rootPath);
    ::GetDiskFreeSpaceEx(reinterpret_cast<const wchar_t *>(path.utf16()),
                         (PULARGE_INTEGER)&bytesAvailable,
                         (PULARGE_INTEGER)&bytesTotal,
                         (PULARGE_INTEGER)&bytesFree);

    ::SetErrorMode(oldmode);
}

QList<QVolumeInfo> QVolumeInfoPrivate::volumes()
{
    QList<QVolumeInfo> volumes;

    char driveName[] = "A:/";
    quint32 driveBits = quint32(::GetLogicalDrives()) & 0x3ffffff;
    while (driveBits) {
        if (driveBits & 1) {
            QVolumeInfoPrivate *data = new QVolumeInfoPrivate;
            data->rootPath = QString::fromLatin1(driveName);
            data->setCachedFlag(CachedRootPathFlag);
            QVolumeInfo drive(*data);
            if (!drive.rootPath().isEmpty()) // drive exists, but not mounted
                volumes.append(drive);
        }
        driveName[0]++;
        driveBits = driveBits >> 1;
    }

    return volumes;
}

QVolumeInfo QVolumeInfoPrivate::rootVolume()
{
    DWORD dwBufferSize = 128;
    QVarLengthArray<wchar_t, 128> profilesDirectory(dwBufferSize);
    bool ok;
    do {
        if ((int)dwBufferSize > profilesDirectory.size())
            profilesDirectory.resize(dwBufferSize);
        ok = ::GetProfilesDirectory(profilesDirectory.data(), &dwBufferSize);
    } while (!ok && GetLastError() == ERROR_INSUFFICIENT_BUFFER);
    if (ok)
        return QVolumeInfo(QString::fromWCharArray(profilesDirectory.data(), profilesDirectory.size()));

    return QVolumeInfo();
}

QT_END_NAMESPACE
