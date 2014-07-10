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

#include <QtCore/qfileinfo.h>
#include <QtCore/private/qcore_mac_p.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFURLEnumerator.h>

#include <sys/mount.h>

#define QT_STATFSBUF struct statfs
#define QT_STATFS    ::statfs

QT_BEGIN_NAMESPACE

void QVolumeInfoPrivate::initRootPath()
{
    rootPath = QFileInfo(rootPath).canonicalFilePath();

    if (rootPath.isEmpty())
        return;

    getUrlProperties(true);
}

void QVolumeInfoPrivate::doStat(uint requiredFlags)
{
    if (getCachedFlag(requiredFlags))
        return;

    if (!getCachedFlag(CachedValidFlag))
        requiredFlags |= CachedValidFlag; // force volume validation

    if (!getCachedFlag(CachedRootPathFlag | CachedValidFlag | CachedReadyFlag)) {
        initRootPath();
        setCachedFlag(CachedRootPathFlag | CachedValidFlag | CachedReadyFlag);
    }

    if (rootPath.isEmpty() || (getCachedFlag(CachedValidFlag) && !valid))
        return;

    if (!getCachedFlag(CachedLabelFlag)) {
        getLabel();
        setCachedFlag(CachedLabelFlag);
    }

    uint bitmask;

    bitmask = CachedDeviceFlag | CachedReadOnlyFlag | CachedFileSystemTypeFlag;
    if (requiredFlags & bitmask) {
        getPosixInfo();
        setCachedFlag(bitmask);
    }

    bitmask = CachedBytesTotalFlag | CachedBytesFreeFlag | CachedBytesAvailableFlag;
    if (requiredFlags & bitmask) {
        getUrlProperties();
        setCachedFlag(bitmask);
    }
}

void QVolumeInfoPrivate::getPosixInfo()
{
    QT_STATFSBUF statfs_buf;
    int result = QT_STATFS(QFile::encodeName(rootPath).constData(), &statfs_buf);
    if (result == 0) {
        device = QByteArray(statfs_buf.f_mntfromname);
        readOnly = (statfs_buf.f_flags & MNT_RDONLY) != 0;
        fileSystemType = statfs_buf.f_fstypename;
    }
}

static inline qint64 CFDictionaryGetInt64(CFDictionaryRef dictionary, const void *key)
{
    CFNumberRef cfNumber = (CFNumberRef)CFDictionaryGetValue(dictionary, key);
    if (!cfNumber)
        return -1;
    qint64 result;
    bool ok = CFNumberGetValue(cfNumber, kCFNumberSInt64Type, &result);
    if (!ok)
        return -1;
    return result;
}

void QVolumeInfoPrivate::getUrlProperties(bool initRootPath)
{
    static const void *rootPathKey[] = { kCFURLVolumeURLKey };
    static const void *propertyKeys[] = {
        // kCFURLVolumeNameKey, // 10.7
        // kCFURLVolumeLocalizedNameKey, // 10.7
        kCFURLVolumeTotalCapacityKey,
        kCFURLVolumeAvailableCapacityKey,
        // kCFURLVolumeIsReadOnlyKey // 10.7
    };
    size_t size = (initRootPath ? sizeof(rootPathKey) : sizeof(propertyKeys) ) / sizeof(void*);
    QCFType<CFArrayRef> keys = CFArrayCreate(kCFAllocatorDefault,
                                             initRootPath ? rootPathKey : propertyKeys,
                                             size,
                                             0);

    if (!keys)
        return;

    QCFString cfPath = rootPath;
    if (initRootPath)
        rootPath.clear();

    QCFType<CFURLRef> url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                          cfPath,
                                                          kCFURLPOSIXPathStyle,
                                                          true);
    if (!url)
        return;

    CFErrorRef error;
    QCFType<CFDictionaryRef> map = CFURLCopyResourcePropertiesForKeys(url, keys, &error);

    if (!map)
        return;

    if (initRootPath) {
        CFURLRef rootUrl = (CFURLRef)CFDictionaryGetValue(map, kCFURLVolumeURLKey);
        if (!rootUrl)
            return;

        rootPath = QCFString(CFURLCopyFileSystemPath(rootUrl, kCFURLPOSIXPathStyle));
        valid = true;
        ready = true;

        return;
    }

    bytesTotal = CFDictionaryGetInt64(map, kCFURLVolumeTotalCapacityKey);
    bytesAvailable = CFDictionaryGetInt64(map, kCFURLVolumeAvailableCapacityKey);
    bytesFree = bytesAvailable;
}

void QVolumeInfoPrivate::getLabel()
{
#if !defined(Q_OS_IOS)
    // deprecated since 10.8
    FSRef ref;
    FSPathMakeRef(reinterpret_cast<const UInt8*>(QFile::encodeName(rootPath).constData()), &ref, 0);

    // deprecated since 10.8
    FSCatalogInfo catalogInfo;
    if (FSGetCatalogInfo(&ref, kFSCatInfoVolume, &catalogInfo, 0, 0, 0) != noErr)
        return;

    // deprecated (use CFURLCopyResourcePropertiesForKeys for 10.7 and higher)
    HFSUniStr255 volumeName;
    OSErr error = FSGetVolumeInfo(catalogInfo.volume,
                                  0,
                                  0,
                                  kFSVolInfoFSInfo,
                                  0,
                                  &volumeName,
                                  0);
    if (error == noErr)
        name = QCFString(FSCreateStringFromHFSUniStr(NULL, &volumeName));
#endif
}

QList<QVolumeInfo> QVolumeInfoPrivate::volumes()
{
    QList<QVolumeInfo> volumes;

    QCFType<CFURLEnumeratorRef> enumerator;
    enumerator = CFURLEnumeratorCreateForMountedVolumes(NULL,
                                                        kCFURLEnumeratorSkipInvisibles,
                                                        NULL);

    CFURLEnumeratorResult result = kCFURLEnumeratorSuccess;
    do {
        CFURLRef url;
        CFErrorRef error;
        result = CFURLEnumeratorGetNextURL(enumerator, &url, &error);
        if (result == kCFURLEnumeratorSuccess) {
            QCFString urlString = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);

            QVolumeInfoPrivate *data = new QVolumeInfoPrivate;
            data->rootPath = urlString;
            data->setCachedFlag(CachedRootPathFlag);
            volumes.append(QVolumeInfo(*data));
        }
    } while (result != kCFURLEnumeratorEnd);

    return volumes;
}

QVolumeInfo QVolumeInfoPrivate::rootVolume()
{
    return QVolumeInfo(QStringLiteral("/"));
}

QT_END_NAMESPACE
