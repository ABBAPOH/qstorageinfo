/****************************************************************************
**
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

#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qvarlengtharray.h>

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
    QVarLengthArray<wchar_t, MAX_PATH + 1> buffer(MAX_PATH + 1);
    if (::GetVolumePathName(reinterpret_cast<const wchar_t *>(path.utf16()), buffer.data(), buffer.size()))
        rootPath = QDir::fromNativeSeparators(QString::fromWCharArray(buffer.data()));
}

static inline QByteArray getDevice(const QString &rootPath)
{
    const QString path = QDir::toNativeSeparators(rootPath);
#if !defined(Q_OS_WINCE)
    const UINT type = ::GetDriveType(reinterpret_cast<const wchar_t *>(path.utf16()));
    if (type == DRIVE_REMOTE) {
        QVarLengthArray<char, 256> buffer(256);
        DWORD bufferLength = buffer.size();
        DWORD result;
        UNIVERSAL_NAME_INFO *remoteNameInfo;
        do {
            buffer.resize(bufferLength);
            remoteNameInfo = reinterpret_cast<UNIVERSAL_NAME_INFO *>(buffer.data());
            result = ::WNetGetUniversalName(reinterpret_cast<const wchar_t *>(path.utf16()),
                                            UNIVERSAL_NAME_INFO_LEVEL,
                                            remoteNameInfo,
                                            &bufferLength);
        } while (result == ERROR_MORE_DATA);
        if (result == NO_ERROR)
            return QString::fromWCharArray(remoteNameInfo->lpUniversalName).toUtf8();
        return QByteArray();
    }
#endif

    QVarLengthArray<wchar_t, 51> deviceBuffer(51);
    if (::GetVolumeNameForVolumeMountPoint(reinterpret_cast<const wchar_t *>(path.utf16()),
                                           deviceBuffer.data(),
                                           deviceBuffer.size())) {
        return QString::fromWCharArray(deviceBuffer.data()).toLatin1();
    }
    return QByteArray();
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

    uint bitmask = CachedFileSystemTypeFlag
            | CachedLabelFlag
            | CachedReadOnlyFlag
            | CachedReadyFlag
            | CachedValidFlag;
    if (requiredFlags & bitmask) {
        retreiveVolumeInfo();
        if (valid && !ready)
            bitmask = CachedValidFlag;
        setCachedFlag(bitmask);

        if (!valid)
            return;
    }

    bitmask = CachedDeviceFlag;
    if (requiredFlags & bitmask) {
        device = getDevice(rootPath);
        setCachedFlag(bitmask);
    }

    bitmask = CachedBytesTotalFlag | CachedBytesFreeFlag | CachedBytesAvailableFlag;
    if (requiredFlags & bitmask) {
        retreiveDiskFreeSpace();
        setCachedFlag(bitmask);
    }
}

void QVolumeInfoPrivate::retreiveVolumeInfo()
{
    const UINT oldmode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    const QString path = QDir::toNativeSeparators(rootPath);
    QVarLengthArray<wchar_t, MAX_PATH + 1> nameBuffer(MAX_PATH + 1);
    QVarLengthArray<wchar_t, MAX_PATH + 1> fileSystemTypeBuffer(MAX_PATH + 1);
    DWORD fileSystemFlags = 0;
    const bool result = ::GetVolumeInformation(reinterpret_cast<const wchar_t *>(path.utf16()),
                                               nameBuffer.data(),
                                               nameBuffer.size(),
                                               Q_NULLPTR,
                                               Q_NULLPTR,
                                               &fileSystemFlags,
                                               fileSystemTypeBuffer.data(),
                                               fileSystemTypeBuffer.size());
    if (!result) {
        ready = false;
        valid = ::GetLastError() == ERROR_NOT_READY;
    } else {
        ready = true;
        valid = true;

        fileSystemType = QString::fromWCharArray(fileSystemTypeBuffer.data()).toLatin1();
        name = QString::fromWCharArray(nameBuffer.data());

        readOnly = (fileSystemFlags & FILE_READ_ONLY_VOLUME) != 0;
    }

    ::SetErrorMode(oldmode);
}

void QVolumeInfoPrivate::retreiveDiskFreeSpace()
{
    const UINT oldmode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    const QString path = QDir::toNativeSeparators(rootPath);
    ::GetDiskFreeSpaceEx(reinterpret_cast<const wchar_t *>(path.utf16()),
                         PULARGE_INTEGER(&bytesAvailable),
                         PULARGE_INTEGER(&bytesTotal),
                         PULARGE_INTEGER(&bytesFree));

    ::SetErrorMode(oldmode);
}

QList<QVolumeInfo> QVolumeInfoPrivate::volumes()
{
    QList<QVolumeInfo> volumes;

    QString driveName = QStringLiteral("A:/");
    const UINT oldmode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    quint32 driveBits = quint32(::GetLogicalDrives()) & 0x3ffffff;
    ::SetErrorMode(oldmode);
    while (driveBits) {
        if (driveBits & 1) {
            QVolumeInfoPrivate *data = new QVolumeInfoPrivate;
            data->rootPath = driveName;
            data->setCachedFlag(CachedRootPathFlag);
            QVolumeInfo drive(*data);
            if (!drive.rootPath().isEmpty()) // drive exists, but not mounted
                volumes.append(drive);
        }
        driveName[0] = driveName[0].unicode() + 1;
        driveBits = driveBits >> 1;
    }

    return volumes;
}

QVolumeInfo QVolumeInfoPrivate::rootVolume()
{
    QVarLengthArray<wchar_t, 128> buffer(128);
    DWORD bufferSize = buffer.size();
    bool ok;
    do {
        buffer.resize(bufferSize);
        ok = ::GetProfilesDirectory(buffer.data(), &bufferSize);
    } while (!ok && GetLastError() == ERROR_INSUFFICIENT_BUFFER);
    if (ok)
        return QVolumeInfo(QString::fromWCharArray(buffer.data(), buffer.size()));

    return QVolumeInfo();
}

QT_END_NAMESPACE
