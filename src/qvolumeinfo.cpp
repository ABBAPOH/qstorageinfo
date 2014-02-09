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

#include "qvolumeinfo.h"
#include "qvolumeinfo_p.h"

QT_BEGIN_NAMESPACE

void QVolumeInfoPrivate::ensureCached(const QVolumeInfo *q, uint flags)
{
    if ((q->d->cachedFlags & flags) != flags) {
        QVolumeInfo *that = const_cast<QVolumeInfo *>(q);
        that->d->doStat(flags);
    }
}

/*!
    \class QVolumeInfo
    \inmodule QtCore
    \since 5.3
    \brief Provides information about currently mounted volumes.

    \ingroup io
    \ingroup shared

    Allows retrieving information about the volume's space, its mount point,
    label, filesystem name and type.

    You can create an instance of QVolumeInfo passing the path to the volume's
    mount point as the constructor parameter, or you can set it using
    setPath() method. The static volumes() method can be used to get the
    list of all mounted filesystems.

    QVolumeInfo always caches the retrieved information but you can call
    refresh() to invalidate the cache.

    The following example retrieves the most common information about the root
    volume of the system and prints information about it.

    \snippet code/src_corelib_io_qvolumeinfo.cpp 2
*/

/*!
    \enum QVolumeInfo::VolumeType
    This enum describes the type of a volume

    \value UnknownVolume          Volume type cannot be determined.
    \value InternalVolume         Is an internal mass storage Volume like a hard drive.
    \value RemovableVolume        Is a removable disk like flash disk.
    \value RemoteVolume           Is a network Volume.
    \value OpticalVolume          Is a CD ROM or DVD Volume.
    \value InternalFlashVolume    Is an internal flash disk, or phone memory.
    \value RamVolume              Is a virtual Volume made in RAM.
*/

/*!
    \enum QVolumeInfo::Capability
    This enum describes the capabilities provided by filesystem represented by
    the QVolumeInfo object.

    \value SupportsSymbolicLinks Whether the filesystem supports symbolic links.
    \value SupportsHardLinks Whether the filesystem supports hard links.
    \value SupportsCaseSensitiveNames Whether the filesystem supports case-sensitive names.
    \value SupportsCasePreservedNames Whether the filesystem supports case-preserved names.
    \value SupportsJournaling Whether the filesystem supports journaling.
    \value SupportsSparseFiles Whether the filesystem supports sparse files.
    \value SupportsPersistentIDs Whether the filesystem supports persistent IDs.
*/

/*!
    Constructs an empty QVolumeInfo object.

    This object is not ready for use, invalid and all its parameters are empty.

    \sa setPath(), isReady(), isValid()
*/
QVolumeInfo::QVolumeInfo()
    : d(new QVolumeInfoPrivate)
{
}

/*!
    Constructs a new QVolumeInfo that gives information about the volume
    mounted at \a path.

    If you pass a directory or file, the QVolumeInfo object will refer to the
    volume where this directory or file is located.
    You can check if the created object is correct using the isValid() method.

    The following example shows how to get volume on which application is
    located. It is recommended to always check that volume is ready and valid.

    \snippet code/src_corelib_io_qvolumeinfo.cpp 0

    \sa setPath()
*/
QVolumeInfo::QVolumeInfo(const QString &path)
    : d(new QVolumeInfoPrivate)
{
    d->rootPath = path;
}

/*!
    Constructs a new QVolumeInfo that is a copy of the \a other QVolumeInfo.
*/
QVolumeInfo::QVolumeInfo(const QVolumeInfo &other)
    : d(other.d)
{
}

/*!
    \internal
*/
QVolumeInfo::QVolumeInfo(QVolumeInfoPrivate &dd)
    : d(&dd)
{
}

/*!
    Destroys the QVolumeInfo and frees its resources.
*/
QVolumeInfo::~QVolumeInfo()
{
}

/*!
    Makes a copy of \a other QVolumeInfo and assigns it to this QVolumeInfo.
*/
QVolumeInfo &QVolumeInfo::operator=(const QVolumeInfo &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn bool QVolumeInfo::operator!=(const QVolumeInfo &other) const

    Returns true if this QVolumeInfo object refers to a different drive or volume
    than the one specified by \a other; otherwise returns false.

    \sa operator==()
*/

/*!
    Returns true if this QVolumeInfo object refers to the same drive or volume
    as the \a other; otherwise returns false.

    Note that the result of comparing two invalid QVolumeInfo objects is always positive.

    \sa operator!=()
*/
bool QVolumeInfo::operator==(const QVolumeInfo &other) const
{
    if (d == other.d)
        return true;
    return device() == other.device();
}

/*!
    Sets QVolumeInfo to the filesystem mounted where \a path is located.

    Path can either be a root path of the filesystem, or a directory or a file within that
    filesystem.

    \sa rootPath()
*/
void QVolumeInfo::setPath(const QString &path)
{
    d->clear();
    d->rootPath = path;
}

/*!
    Returns the mount point of the filesystem this QVolumeInfo object represents.

    On Windows, returns the volume letter in case the volume is not mounted to directory.

    Note that the value returned by rootPath() is the real mount point of a volume
    and may not be equal to the value passed to constructor or setPath() method.
    For example, if you have only the root volume in the system and pass '/directory'
    to setPath(), then this method will return '/'.

    \sa setPath(), device()
*/
QString QVolumeInfo::rootPath() const
{
    QVolumeInfoPrivate::ensureCached(this, QVolumeInfoPrivate::CachedRootPathFlag);
    return d->rootPath;
}

/*!
    Returns the size (in bytes) available for the current user. If the user is the root user or a
    system administrator returns all available size.

    This size can be less than or equal to the free size, returned by bytesFree() function.

    \sa bytesTotal(), bytesFree()
*/
qint64 QVolumeInfo::bytesAvailable() const
{
    QVolumeInfoPrivate::ensureCached(this, QVolumeInfoPrivate::CachedBytesAvailableFlag);
    return d->bytesAvailable;
}

/*!
    Returns the free size (in bytes) available on the volume. Note, that if there are some kind of
    quotas on the filesystem, this value can be bigger than bytesAvailable().

    \sa bytesTotal(), bytesAvailable()
*/
qint64 QVolumeInfo::bytesFree() const
{
    QVolumeInfoPrivate::ensureCached(this, QVolumeInfoPrivate::CachedBytesFreeFlag);
    return d->bytesFree;
}

/*!
    Returns total volume size in bytes.

    \sa bytesFree(), bytesAvailable()
*/
qint64 QVolumeInfo::bytesTotal() const
{
    QVolumeInfoPrivate::ensureCached(this, QVolumeInfoPrivate::CachedBytesTotalFlag);
    return d->bytesTotal;
}

/*!
    Returns the name of the filesystem.

    This is a platform-dependent function, and filesystem names can vary between different
    operating systems. For example, on Windows filesystems can be named as 'NTFS' and on Linux
    as 'ntfs-3g' or 'fuseblk'.

    \sa name()
*/
QByteArray QVolumeInfo::fileSystemName() const
{
    QVolumeInfoPrivate::ensureCached(this, QVolumeInfoPrivate::CachedFileSystemNameFlag);
    return d->fileSystemName;
}

/*!
    Returns the device for this volume.

    The result of this function is platform-dependent and usually should not be used. However,
    you can retrieve this value for some platform-specific purposes. By example, you can get device
    on Unix and try to read from it manually.

    For example, on Unix filesystems (including Mac OS), this returns the devpath like '/dev/sda0'
    for local volumes. On Windows, returns the UNC path starting with \\\\?\\ for local volumes
    (i.e. volume GUID).

    \sa rootPath()
*/
QByteArray QVolumeInfo::device() const
{
    QVolumeInfoPrivate::ensureCached(this, QVolumeInfoPrivate::CachedDeviceFlag);
    return d->device;
}

/*!
    Returns the human-readable name of a filesystem, usually called 'label'.

    Not all filesystems support this feature, in this case value returned by this method could
    be empty. An empty string is returned if the file system does not support labels or no label
    is set.

    On Linux, retrieving the volume's label requires udev to be present in the system.

    \sa fileSystemName()
*/
QString QVolumeInfo::name() const
{
    QVolumeInfoPrivate::ensureCached(this, QVolumeInfoPrivate::CachedNameFlag);
    return d->name;
}

/*!
    Returns non-empty name of a volume.

    First, tries if name() is not empty, then checks volume's device(), then
    fallbacks to rootPath().
*/
QString QVolumeInfo::displayName() const
{
    QVolumeInfoPrivate::ensureCached(this, QVolumeInfoPrivate::CachedNameFlag
                                     | QVolumeInfoPrivate::CachedDeviceFlag
                                     | QVolumeInfoPrivate::CachedRootPathFlag);
    if (!d->name.isEmpty())
        return d->name;
    if (!d->device.isEmpty())
        return QString::fromUtf8(d->device);
    return d->rootPath;
}

/*!
    \fn bool QVolumeInfo::isRoot() const

    Returns true if this QVolumeInfo represents the system root volume; false otherwise.

    On Unix filesystems, the root volume is a volume mounted at "/", on Windows the root volume is a
    volume where OS is installed.

    \sa rootVolume()
*/

/*!
    Returns true if the current filesystem is protected from writing; false otherwise.
*/
bool QVolumeInfo::isReadOnly() const
{
    QVolumeInfoPrivate::ensureCached(this, QVolumeInfoPrivate::CachedReadOnlyFlag);
    return d->readOnly;
}

/*!
    Returns true if current filesystem is ready to work; false otherwise. For example, false is
    returned if CD volume is not inserted.

    Note that fileSystemName(), name(), bytesTotal(), bytesFree(), and bytesAvailable() will return
    invalid data until the volume is ready.

    \sa isValid()
*/
bool QVolumeInfo::isReady() const
{
    QVolumeInfoPrivate::ensureCached(this, QVolumeInfoPrivate::CachedReadyFlag);
    return d->ready;
}

/*!
    Returns true if the QVolumeInfo specified by rootPath exists and is mounted correctly.

    \sa isReady()
*/
bool QVolumeInfo::isValid() const
{
    QVolumeInfoPrivate::ensureCached(this, QVolumeInfoPrivate::CachedValidFlag);
    return d->valid;
}

/*!
    Returns the type of the filesystem (i.e. remote volume, removable and so on).

    The following example prints the type of a volume.

    \snippet code/src_corelib_io_qvolumeinfo.cpp 3

    \sa QVolumeInfo::VolumeType
*/
QVolumeInfo::VolumeType QVolumeInfo::type() const
{
    QVolumeInfoPrivate::ensureCached(this, QVolumeInfoPrivate::CachedTypeFlag);
    return QVolumeInfo::VolumeType(d->type);
}

/*!
    Returns the flags supported by volume's filesystem.
*/
QVolumeInfo::Capabilities QVolumeInfo::capabilities() const
{
    QVolumeInfoPrivate::ensureCached(this, QVolumeInfoPrivate::CachedCapabilitiesFlag);
    return d->capabilities;
}

/*!
    \fn bool QVolumeInfo::hasCapability(QVolumeInfo::Capability capability) const

    Returns true if volume's filesystem supports specified \a capability.
*/

/*!
    Resets QVolumeInfo's internal cache.

    QVolumeInfo caches information about volumes to speed up performance. Some information can be
    retrieved by only one native function call (for example, if you call bytesTotal(),
    QVolumeInfo will also cache information for bytesAvailable() and bytesFree()).
    Also, QVolumeInfo won't update information for future calls and you have to manually
    reset the cache when needed.
*/
void QVolumeInfo::refresh()
{
    d->clear();
}

/*!
    Returns list of QVolumeInfo's that corresponds to the list of currently mounted filesystems.

    On Windows, this returns volumes presented in 'My Computer' folder. On Unix operation systems,
    returns list of all mounted filesystems (except for Mac, where devfs is ignored).

    The example shows how to retrieve all volumes present in the system and skip read-only volumes.

    \snippet code/src_corelib_io_qvolumeinfo.cpp 1

    \sa rootVolume()
*/
QList<QVolumeInfo> QVolumeInfo::volumes()
{
    return QVolumeInfoPrivate::volumes();
}

Q_GLOBAL_STATIC_WITH_ARGS(QVolumeInfo, theRootVolume, (QVolumeInfoPrivate::rootVolume()))

/*!
    Returns a QVolumeInfo object that represents the system root volume.

    On Unix systems this call returns '/' volume, on Windows the volume where operating
    system is installed is returned.

    \sa isRoot()
*/
QVolumeInfo QVolumeInfo::rootVolume()
{
    return *theRootVolume();
}

QT_END_NAMESPACE