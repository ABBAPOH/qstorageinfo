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

#ifndef QDRIVEINFO_P_H
#define QDRIVEINFO_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

//#include <qplatformdefs.h>

#include "qdriveinfo.h"

#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QFile>

QT_BEGIN_NAMESPACE

class QDriveInfoPrivate : public QSharedData
{
public:

    inline QDriveInfoPrivate() : QSharedData(),
        bytesTotal(0), bytesFree(0), bytesAvailable(0),
        type(QDriveInfo::UnknownDrive),
        readOnly(false), ready(false), valid(false),
        cachedFlags(0)
    {}

    enum CachedFlags {
        CachedRootPathFlag = 0x001,
        CachedDeviceFlag = 0x002,
        CachedFileSystemNameFlag = 0x004,
        CachedNameFlag = 0x008,
        CachedBytesTotalFlag = 0x010,
        CachedBytesFreeFlag = 0x020,
        CachedBytesAvailableFlag = 0x040,
        CachedTypeFlag = 0x100,
        CachedReadOnlyFlag = 0x200,
        CachedReadyFlag = 0x400,
        CachedValidFlag = 0x800,
        CachedCapabilitiesFlag = 0x1000
    };

    inline void clear()
    {
        device.clear();
        fileSystemName.clear();
        name.clear();

        bytesTotal = 0;
        bytesFree = 0;
        bytesAvailable = 0;

        type = QDriveInfo::UnknownDrive;
        readOnly = false;
        ready = false;
        valid = false;

        cachedFlags = 0;
    }

    inline bool getCachedFlag(uint c) const
    { return !((cachedFlags & c) ^ c); }
    inline void setCachedFlag(uint c)
    { cachedFlags |= c; }

    void initRootPath();
    void doStat(uint requiredFlags);

    static QList<QDriveInfo> drives();
    static QDriveInfo rootDrive();

    static inline void ensureCached(const QDriveInfo *q, uint flags);

protected:
#if defined(Q_OS_WIN)
    void getVolumeInfo();
    void getDiskFreeSpace();
#elif defined(Q_OS_MAC)
    void getPosixInfo();
    void getUrlProperties(bool initRootPath = false);
    void getLabel();
#elif defined(Q_OS_UNIX)
    void getVolumeInfo();
    void getCapabilities();
#endif

public:
    QString rootPath;
    QByteArray device;
    QByteArray fileSystemName;
    QString name;

    qint64 bytesTotal;
    qint64 bytesFree;
    qint64 bytesAvailable;

    QDriveInfo::Capabilities capabilities;

    ushort type : 8;
    ushort readOnly : 1;
    ushort ready : 1;
    ushort valid : 1;
    ushort reserved : 5;

    uint cachedFlags;
};

QT_END_NAMESPACE

#endif // QDRIVEINFO_P_H
