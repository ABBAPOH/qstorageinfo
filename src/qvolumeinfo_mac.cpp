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

#include <QtCore/QFileInfo>
#include <QtCore/private/qcore_mac_p.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFURLEnumerator.h>
#if !defined(Q_OS_IOS)
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IODVDMedia.h>
#endif

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

static inline QVolumeInfo::VolumeTypeFlags determineType(const QByteArray &device)
{
    QVolumeInfo::VolumeTypeFlags volumeType = QVolumeInfo::UnknownVolume;

#if !defined(Q_OS_IOS)
    DASessionRef sessionRef;
    DADiskRef diskRef;
    CFDictionaryRef descriptionDictionary;

    sessionRef = DASessionCreate(NULL);
    if (sessionRef == NULL)
        return QVolumeInfo::UnknownVolume;

    diskRef = DADiskCreateFromBSDName(NULL, sessionRef, device.constData());
    if (diskRef == NULL) {
        CFRelease(sessionRef);
        return QVolumeInfo::UnknownVolume;
    }

    descriptionDictionary = DADiskCopyDescription(diskRef);
    if (descriptionDictionary == NULL) {
        CFRelease(diskRef);
        CFRelease(sessionRef);
        return QVolumeInfo::RemoteVolume;
    }

    CFBooleanRef boolRef;
    boolRef = (CFBooleanRef)CFDictionaryGetValue(descriptionDictionary,
                                                 kDADiskDescriptionVolumeNetworkKey);
    if (boolRef && CFBooleanGetValue(boolRef)){
        CFRelease(descriptionDictionary);
        CFRelease(diskRef);
        CFRelease(sessionRef);
        return QVolumeInfo::RemoteVolume;
    }

    boolRef = (CFBooleanRef)CFDictionaryGetValue(descriptionDictionary,
                                                 kDADiskDescriptionMediaRemovableKey);
    if (boolRef)
        volumeType |= CFBooleanGetValue(boolRef) ? QVolumeInfo::RemovableVolume : QVolumeInfo::InternalVolume;

    DADiskRef wholeDisk;
    wholeDisk = DADiskCopyWholeDisk(diskRef);
    if (wholeDisk) {
        io_service_t mediaService;
        mediaService = DADiskCopyIOMedia(wholeDisk);
        if (mediaService) {
            if (IOObjectConformsTo(mediaService, kIOCDMediaClass)
                    || IOObjectConformsTo(mediaService, kIODVDMediaClass)) {
                volumeType |= QVolumeInfo::VolumeTypeFlags(QVolumeInfo::RemovableVolume
                                                           | QVolumeInfo::OpticalVolume);
            }
            IOObjectRelease(mediaService);
        }
        CFRelease(wholeDisk);
    }

    CFRelease(descriptionDictionary);
    CFRelease(diskRef);
    CFRelease(sessionRef);
#else
    Q_UNUSED(device);
#endif

    return volumeType;
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

    if (!getCachedFlag(CachedNameFlag)) {
        getLabel();
        setCachedFlag(CachedNameFlag);
    }

    uint bitmask = 0;

    bitmask = CachedDeviceFlag | CachedReadOnlyFlag | CachedFileSystemNameFlag;
    if (requiredFlags & bitmask) {
        getPosixInfo();
        setCachedFlag(bitmask);
    }

    bitmask = CachedBytesTotalFlag
              | CachedBytesFreeFlag
              | CachedBytesAvailableFlag
              | CachedCapabilitiesFlag;
    if (requiredFlags & bitmask) {
        getUrlProperties();
        setCachedFlag(bitmask);
    }

    bitmask = CachedTypeFlag;
    if (requiredFlags & bitmask) {
        typeFlags = determineType(device);
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
        fileSystemName = statfs_buf.f_fstypename;
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

static inline QString CFDictionaryGetQString(CFDictionaryRef dictionary, const void *key)
{
    return QCFString::toQString((CFStringRef)CFDictionaryGetValue(dictionary, key));
}

static inline bool CFDictionaryGetBool(CFDictionaryRef dictionary, const void *key)
{
    return CFBooleanGetValue((CFBooleanRef)CFDictionaryGetValue(dictionary, key));
}

void QVolumeInfoPrivate::getUrlProperties(bool initRootPath)
{
    static const void *rootPathKey[] = { kCFURLVolumeURLKey };
    static const void *propertyKeys[] = {
        // kCFURLVolumeNameKey, // 10.7
        // kCFURLVolumeLocalizedNameKey, // 10.7
        kCFURLVolumeTotalCapacityKey,
        kCFURLVolumeAvailableCapacityKey,
        kCFURLVolumeSupportsPersistentIDsKey,
        kCFURLVolumeSupportsSymbolicLinksKey,
        kCFURLVolumeSupportsHardLinksKey,
        kCFURLVolumeSupportsJournalingKey,
        kCFURLVolumeSupportsSparseFilesKey,
        kCFURLVolumeSupportsCaseSensitiveNamesKey,
        kCFURLVolumeSupportsCasePreservedNamesKey,
        // kCFURLVolumeIsReadOnlyKey // 10.7
    };
    size_t size = (initRootPath ? sizeof(rootPathKey) : sizeof(propertyKeys) ) / sizeof(void*);
    CFArrayRef keys = CFArrayCreate(kCFAllocatorDefault,
                                    initRootPath ? rootPathKey : propertyKeys,
                                    size,
                                    0);

    if (!keys)
        return;

    QCFString cfPath = rootPath;
    if (initRootPath)
        rootPath.clear();

    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                 cfPath,
                                                 kCFURLPOSIXPathStyle,
                                                 true);
    if (!url) {
        CFRelease(keys);
        return;
    }

    CFErrorRef error;
    CFDictionaryRef map = CFURLCopyResourcePropertiesForKeys(url, keys, &error);
    CFRelease(url);
    CFRelease(keys);

    if (!map)
        return;

    if (initRootPath) {
        CFURLRef rootUrl = (CFURLRef)CFDictionaryGetValue(map, kCFURLVolumeURLKey);
        if (!rootUrl)
            return;

        rootPath = QCFString(CFURLCopyFileSystemPath(rootUrl, kCFURLPOSIXPathStyle));
        valid = true;
        ready = true;

        CFRelease(map);
        return;
    }

    bytesTotal = CFDictionaryGetInt64(map, kCFURLVolumeTotalCapacityKey);
    bytesAvailable = CFDictionaryGetInt64(map, kCFURLVolumeAvailableCapacityKey);
    bytesFree = bytesAvailable;

    capabilities = 0;
    if (CFDictionaryGetBool(map, kCFURLVolumeSupportsPersistentIDsKey))
        capabilities |= QVolumeInfo::SupportsPersistentIDs;
    if (CFDictionaryGetBool(map, kCFURLVolumeSupportsSymbolicLinksKey))
        capabilities |= QVolumeInfo::SupportsSymbolicLinks;
    if (CFDictionaryGetBool(map, kCFURLVolumeSupportsHardLinksKey))
        capabilities |= QVolumeInfo::SupportsHardLinks;
    if (CFDictionaryGetBool(map, kCFURLVolumeSupportsJournalingKey))
        capabilities |= QVolumeInfo::SupportsJournaling;
    if (CFDictionaryGetBool(map, kCFURLVolumeSupportsSparseFilesKey))
        capabilities |= QVolumeInfo::SupportsSparseFiles;
    if (CFDictionaryGetBool(map, kCFURLVolumeSupportsCaseSensitiveNamesKey))
        capabilities |= QVolumeInfo::SupportsCaseSensitiveNames;
    if (CFDictionaryGetBool(map, kCFURLVolumeSupportsCasePreservedNamesKey))
        capabilities |= QVolumeInfo::SupportsCasePreservedNames;

    CFRelease(map);
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

    CFURLEnumeratorRef enumerator;
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

    CFRelease(enumerator);
    return volumes;
}

QVolumeInfo QVolumeInfoPrivate::rootVolume()
{
    return QVolumeInfo(QStringLiteral("/"));
}

QT_END_NAMESPACE
