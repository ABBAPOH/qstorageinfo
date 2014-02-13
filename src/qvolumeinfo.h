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

#ifndef QVOLUMEINFO_H
#define QVOLUMEINFO_H

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QSharedDataPointer>

QT_BEGIN_NAMESPACE

class QVolumeInfoPrivate;
class QVolumeInfo
{
public:
    enum VolumeTypeFlag {
        UnknownVolume = 0,

        InternalVolume = 0x01,
        RemovableVolume = 0x02,
        RemoteVolume = 0x04,

        OpticalVolume = 0x10,
        RamVolume = 0x20
    };
    Q_DECLARE_FLAGS(VolumeTypeFlags, VolumeTypeFlag)

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

    QVolumeInfo();
    explicit QVolumeInfo(const QString &path);
    QVolumeInfo(const QVolumeInfo &other);
    ~QVolumeInfo();

    QVolumeInfo &operator=(const QVolumeInfo &other);

    bool operator==(const QVolumeInfo &other) const;
    inline bool operator!=(const QVolumeInfo &other) const;

    void setPath(const QString &path);

    QString rootPath() const;
    QByteArray device() const;
    QByteArray fileSystemName() const;
    QString name() const;
    QString displayName() const;

    qint64 bytesTotal() const;
    qint64 bytesFree() const;
    qint64 bytesAvailable() const;

    inline bool isRoot() const;
    bool isReadOnly() const;
    bool isReady() const;
    bool isValid() const;

    VolumeTypeFlags typeFlags() const;

    Capabilities capabilities() const;
    inline bool hasCapability(Capability capability) const;

    void refresh();

    static QList<QVolumeInfo> volumes();
    static QVolumeInfo rootVolume();

protected:
    explicit QVolumeInfo(QVolumeInfoPrivate &dd);

private:
    friend class QVolumeInfoPrivate;
    QSharedDataPointer<QVolumeInfoPrivate> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QVolumeInfo::Capabilities)

inline bool QVolumeInfo::operator!=(const QVolumeInfo &other) const
{ return !(operator==(other)); }

inline bool QVolumeInfo::isRoot() const
{ return *this == QVolumeInfo::rootVolume(); }

inline bool QVolumeInfo::hasCapability(QVolumeInfo::Capability capability) const
{ return (capabilities() & capability) != 0; }

QT_END_NAMESPACE

#endif // QVOLUMEINFO_H
