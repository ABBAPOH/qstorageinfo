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

#ifndef QDRIVEINFO_H
#define QDRIVEINFO_H

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QSharedDataPointer>

QT_BEGIN_NAMESPACE

class QDriveInfoPrivate;
class QDriveInfo
{
public:
    enum DriveType {
        UnknownDrive = 0,
        InternalDrive,
        RemovableDrive,
        RemoteDrive,
        CdromDrive,
        InternalFlashDrive,
        RamDrive
    };

    enum Capability {
        SupportsSymbolicLinks = 0x01,
        SupportsHardLinks = 0x02,
        SupportsCaseSensitiveNames = 0x04,
        SupportsCasePreservedNames = 0x08,
        SupportsJournaling = 0x10,
        SupportsSparseFiles = 0x20,
        SupportsPersistentIDs = 0x40
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)

    QDriveInfo();
    explicit QDriveInfo(const QString &rootPath);
    QDriveInfo(const QDriveInfo &other);
    ~QDriveInfo();

    QDriveInfo &operator=(const QDriveInfo &other);

    bool operator==(const QDriveInfo &other) const;
    inline bool operator!=(const QDriveInfo &other) const;

    QString rootPath() const;
    void setRootPath(const QString &rootPath);

    QByteArray device() const;
    QByteArray fileSystemName() const;
    QString name() const;

    qint64 bytesTotal() const;
    qint64 bytesFree() const;
    qint64 bytesAvailable() const;

    inline bool isRoot() const;
    bool isReadOnly() const;
    bool isReady() const;
    bool isValid() const;

    DriveType type() const;

    Capabilities capabilities() const;
    inline bool hasCapability(Capability capability) const;

    void refresh();

    static QList<QDriveInfo> drives();
    static QDriveInfo rootDrive();

protected:
    explicit QDriveInfo(QDriveInfoPrivate &dd);

private:
    friend class QDriveInfoPrivate;
    QSharedDataPointer<QDriveInfoPrivate> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QDriveInfo::Capabilities)

inline bool QDriveInfo::operator!=(const QDriveInfo &other) const
{ return !(operator==(other)); }

inline bool QDriveInfo::isRoot() const
{ return *this == QDriveInfo::rootDrive(); }

inline bool QDriveInfo::hasCapability(QDriveInfo::Capability capability) const
{ return (capabilities() & capability) != 0; }

QT_END_NAMESPACE

#endif // QDRIVEINFO_H
