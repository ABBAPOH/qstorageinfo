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

#include "qdriveinfo.h"
#include "qdriveinfo_p.h"

QT_BEGIN_NAMESPACE

void QDriveInfoPrivate::ensureCached(const QDriveInfo *q, uint flags)
{
    if ((q->d->cachedFlags & flags) != flags) {
        QDriveInfo *that = const_cast<QDriveInfo *>(q);
        that->d->doStat(flags);
    }
}

/*!
    \class QDriveInfo
    \inmodule QtCore
    \since 5.3
    \brief Provides information about currently mounted drives and volumes.

    \ingroup io
    \ingroup shared

    Allows retrieving information about the drive's space, its mount point,
    label, filesystem name and type.

    You can create an instance of QDriveInfo passing the path to the drive's
    mount point as the constructor parameter, or you can set it using
    setPath() method. The static drives() method can be used to get the
    list of all mounted filesystems.

    QDriveInfo always caches the retrieved information but you can call
    refresh() to invalidate the cache.

    The following example retrieves the most common information about the root
    drive of the system and prints information about it.

    \snippet code/src_corelib_io_qdriveinfo.cpp 2
*/

/*!
    \enum QDriveInfo::DriveType
    This enum describes the type of a drive or volume

    \value UnknownDrive          Drive type cannot be determined.
    \value InternalDrive         Is an internal mass storage drive like a hard drive.
    \value RemovableDrive        Is a removable disk like flash disk or MMC.
    \value RemoteDrive           Is a network drive.
    \value OpticalDrive          Is a CD ROM or DVD drive.
    \value InternalFlashDrive    Is an internal flash disk, or phone memory.
    \value RamDrive              Is a virtual drive made in RAM.
*/

/*!
    \enum QDriveInfo::Capability
    This enum describes the capabilities provided by filesystem represented by
    the QDriveInfo object.

    \value SupportsSymbolicLinks Whether the filesystem supports symbolic links.
    \value SupportsHardLinks Whether the filesystem supports hard links.
    \value SupportsCaseSensitiveNames Whether the filesystem supports case-sensitive names.
    \value SupportsCasePreservedNames Whether the filesystem supports case-preserved names.
    \value SupportsJournaling Whether the filesystem supports journaling.
    \value SupportsSparseFiles Whether the filesystem supports sparse files.
    \value SupportsPersistentIDs Whether the filesystem supports persistent IDs.
*/

/*!
    Constructs an empty QDriveInfo object.

    This object is not ready for use, invalid and all its parameters are empty.

    \sa setPath(), isReady(), isValid()
*/
QDriveInfo::QDriveInfo()
    : d(new QDriveInfoPrivate)
{
}

/*!
    Constructs a new QDriveInfo that gives information about the drive mounted
    at \a path.

    If you pass a directory or file, the QDriveInfo object will refer to the
    volume where this directory or file is located.
    You can check if the created object is correct using the isValid() method.

    The following example shows how to get drive on which application is
    located. It is recommended to always check that drive is ready and valid.

    \snippet code/src_corelib_io_qdriveinfo.cpp 0

    \sa setPath()
*/
QDriveInfo::QDriveInfo(const QString &path)
    : d(new QDriveInfoPrivate)
{
    d->rootPath = path;
}

/*!
    Constructs a new QDriveInfo that is a copy of the \a other QDriveInfo.
*/
QDriveInfo::QDriveInfo(const QDriveInfo &other)
    : d(other.d)
{
}

/*!
    \internal
*/
QDriveInfo::QDriveInfo(QDriveInfoPrivate &dd)
    : d(&dd)
{
}

/*!
    Destroys the QDriveInfo and frees its resources.
*/
QDriveInfo::~QDriveInfo()
{
}

/*!
    Makes a copy of \a other QDriveInfo and assigns it to this QDriveInfo.
*/
QDriveInfo &QDriveInfo::operator=(const QDriveInfo &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn bool QDriveInfo::operator!=(const QDriveInfo &other) const

    Returns true if this QDriveInfo object refers to a different drive or volume
    than the one specified by \a other; otherwise returns false.

    \sa operator==()
*/

/*!
    Returns true if this QDriveInfo object refers to the same drive or volume
    as the \a other; otherwise returns false.

    Note that the result of comparing two invalid QDriveInfo objects is always positive.

    \sa operator!=()
*/
bool QDriveInfo::operator==(const QDriveInfo &other) const
{
    if (d == other.d)
        return true;
    return device() == other.device();
}

/*!
    Sets QDriveInfo to the filesystem mounted where \a path is located.

    Path can either be a root path of the filesystem, or a directory or a file within that
    filesystem.

    \sa rootPath()
*/
void QDriveInfo::setPath(const QString &path)
{
    d->clear();
    d->rootPath = path;
}

/*!
    Returns the mount point of the filesystem this QDriveInfo object represents.

    On Windows, returns drive letter in case the drive is not mounted to directory.

    Note that the value returned by rootPath() is the real mount point of a drive
    and may not be equal to the value passed to constructor or setPath() method.
    For example, if you have only the root drive in the system and pass '/directory'
    to setPath(), then this method will return '/'.

    \sa setPath(), device()
*/
QString QDriveInfo::rootPath() const
{
    QDriveInfoPrivate::ensureCached(this, QDriveInfoPrivate::CachedRootPathFlag);
    return d->rootPath;
}

/*!
    Returns the size (in bytes) available for the current user. If the user is the root user or a
    system administrator returns all available size.

    This size can be less than or equal to the free size, returned by bytesFree() function.

    \sa bytesTotal(), bytesFree()
*/
qint64 QDriveInfo::bytesAvailable() const
{
    QDriveInfoPrivate::ensureCached(this, QDriveInfoPrivate::CachedBytesAvailableFlag);
    return d->bytesAvailable;
}

/*!
    Returns the free size (in bytes) available on the drive. Note, that if there are some kind of
    quotas on the filesystem, this value can be bigger than bytesAvailable().

    \sa bytesTotal(), bytesAvailable()
*/
qint64 QDriveInfo::bytesFree() const
{
    QDriveInfoPrivate::ensureCached(this, QDriveInfoPrivate::CachedBytesFreeFlag);
    return d->bytesFree;
}

/*!
    Returns total drive size in bytes.

    \sa bytesFree(), bytesAvailable()
*/
qint64 QDriveInfo::bytesTotal() const
{
    QDriveInfoPrivate::ensureCached(this, QDriveInfoPrivate::CachedBytesTotalFlag);
    return d->bytesTotal;
}

/*!
    Returns the name of the filesystem.

    This is a platform-dependent function, and filesystem names can vary between different
    operating systems. For example, on Windows filesystems can be named as 'NTFS' and on Linux
    as 'ntfs-3g' or 'fuseblk'.

    \sa name()
*/
QByteArray QDriveInfo::fileSystemName() const
{
    QDriveInfoPrivate::ensureCached(this, QDriveInfoPrivate::CachedFileSystemNameFlag);
    return d->fileSystemName;
}

/*!
    Returns the device for this drive.

    The result of this function is platform-dependent and usually should not be used. However,
    you can retrieve this value for some platform-specific purposes. By example, you can get device
    on Unix and try to read from it manually.

    For example, on Unix filesystems (including Mac OS), this returns the devpath like '/dev/sda0'
    for local drives. On Windows, returns the UNC path starting with \\\\?\\ for local drives
    (i.e. volume GUID).

    \sa rootPath()
*/
QByteArray QDriveInfo::device() const
{
    QDriveInfoPrivate::ensureCached(this, QDriveInfoPrivate::CachedDeviceFlag);
    return d->device;
}

/*!
    Returns the human-readable name of a filesystem, usually called 'label'.

    Not all filesystems support this feature, in this case value returned by this method could
    be empty. An empty string is returned if the file system does not support labels or no label
    is set.

    On Linux, retrieving the drive's label requires udev to be present in the system.

    \sa fileSystemName()
*/
QString QDriveInfo::name() const
{
    QDriveInfoPrivate::ensureCached(this, QDriveInfoPrivate::CachedNameFlag);
    return d->name;
}

/*!
    \fn bool QDriveInfo::isRoot() const

    Returns true if this QDriveInfo represents the system root volume or drive; false otherwise.

    On Unix filesystems, the root drive is a drive mounted at "/", on Windows the root drive is a
    drive where OS is installed.

    \sa rootDrive()
*/

/*!
    Returns true if the current filesystem is protected from writing; false otherwise.
*/
bool QDriveInfo::isReadOnly() const
{
    QDriveInfoPrivate::ensureCached(this, QDriveInfoPrivate::CachedReadOnlyFlag);
    return d->readOnly;
}

/*!
    Returns true if current filesystem is ready to work; false otherwise. For example, false is
    returned if CD drive is not inserted.

    Note that fileSystemName(), name(), bytesTotal(), bytesFree(), and bytesAvailable() will return
    invalid data until the drive is ready.

    \sa isValid()
*/
bool QDriveInfo::isReady() const
{
    QDriveInfoPrivate::ensureCached(this, QDriveInfoPrivate::CachedReadyFlag);
    return d->ready;
}

/*!
    Returns true if the QDriveInfo specified by rootPath exists and is mounted correctly.

    \sa isReady()
*/
bool QDriveInfo::isValid() const
{
    QDriveInfoPrivate::ensureCached(this, QDriveInfoPrivate::CachedValidFlag);
    return d->valid;
}

/*!
    Returns the type of the filesystem (i.e. remote drive, removable and so on).

    The following example prints the type of a drive.

    \snippet code/src_corelib_io_qdriveinfo.cpp 3

    \sa QDriveInfo::DriveType
*/
QDriveInfo::DriveType QDriveInfo::type() const
{
    QDriveInfoPrivate::ensureCached(this, QDriveInfoPrivate::CachedTypeFlag);
    return QDriveInfo::DriveType(d->type);
}

/*!
    Returns the flags supported by drive's filesystem.
*/
QDriveInfo::Capabilities QDriveInfo::capabilities() const
{
    QDriveInfoPrivate::ensureCached(this, QDriveInfoPrivate::CachedCapabilitiesFlag);
    return d->capabilities;
}

/*!
    \fn bool QDriveInfo::hasCapability(QDriveInfo::Capability capability) const

    Returns true if drive's filesystem supports specified \a capability.
*/

/*!
    Resets QDriveInfo's internal cache.

    QDriveInfo caches information about drives to speed up performance. Some information can be
    retrieved by only one native function call (for example, if you call bytesTotal(),
    QDriveInfo will also cache information for bytesAvailable() and bytesFree()).
    Also, QDriveInfo won't update information for future calls and you have to manually
    reset the cache when needed.
*/
void QDriveInfo::refresh()
{
    d->clear();
}

/*!
    Returns list of QDriveInfo's that corresponds to the list of currently mounted filesystems.

    On Windows, this returns drives presented in 'My Computer' folder. On Unix operation systems,
    returns list of all mounted filesystems (except for Mac, where devfs is ignored).

    The example shows how to retrieve all drives present in the system and skip read-only drives.

    \snippet code/src_corelib_io_qdriveinfo.cpp 1

    \sa rootDrive()
*/
QList<QDriveInfo> QDriveInfo::drives()
{
    return QDriveInfoPrivate::drives();
}

Q_GLOBAL_STATIC_WITH_ARGS(QDriveInfo, theRootDrive, (QDriveInfoPrivate::rootDrive()))

/*!
    Returns a QDriveInfo object that represents the system root volume or drive.

    On Unix systems this call returns '/' volume, on Windows the volume where operating
    system is installed is returned.

    \sa isRoot()
*/
QDriveInfo QDriveInfo::rootDrive()
{
    return *theRootDrive();
}

QT_END_NAMESPACE
