/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2014 Ivan Komissarov
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "drivemodel.h"

#include <QDir>
#include <qmath.h>

static QString sizeToString(qint64 size)
{
    static const char *const strings[] = { "b", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };

    if (size <= 0)
        return DriveModel::tr("0 b");

    double power = log((double)size)/log(1024.0);
    int intPower = (int)power;
    intPower = intPower >= 8 ? 8 - 1 : intPower;

    double normSize = size / pow(1024.0, intPower);
    //: this should expand to "1.23 GB"
    return DriveModel::tr("%1 %2").arg(normSize, 0, 'f', intPower > 0 ? 2 : 0).arg(strings[intPower]);
}

static QString capabilityToString(QDriveInfo::Capability capability)
{
    switch (capability) {
    case QDriveInfo::SupportsSymbolicLinks:
        return DriveModel::tr("SupportsSymbolicLinks");
    case QDriveInfo::SupportsHardLinks:
        return DriveModel::tr("SupportsHardLinks");
    case QDriveInfo::SupportsCaseSensitiveNames:
        return DriveModel::tr("SupportsCaseSensitiveNames");
    case QDriveInfo::SupportsCasePreservedNames:
        return DriveModel::tr("SupportsCasePreservedNames");
    case QDriveInfo::SupportsJournaling:
        return DriveModel::tr("SupportsJournaling");
    case QDriveInfo::SupportsSparseFiles:
        return DriveModel::tr("SupportsSparseFiles");
    case QDriveInfo::SupportsPersistentIDs:
        return DriveModel::tr("SupportsPersistentIDs");
    }
    return QString();
}

static QString typeToString(QDriveInfo::DriveType type)
{
    switch (type) {
    case QDriveInfo::UnknownDrive:
        return DriveModel::tr("Unknown");
    case QDriveInfo::InternalDrive:
        return DriveModel::tr("Internal");
    case QDriveInfo::RemovableDrive:
        return DriveModel::tr("Removable");
    case QDriveInfo::RemoteDrive:
        return DriveModel::tr("Remote");
    case QDriveInfo::CdromDrive:
        return DriveModel::tr("Cdrom");
    case QDriveInfo::InternalFlashDrive:
        return DriveModel::tr("Internal flash");
    case QDriveInfo::RamDrive:
        return DriveModel::tr("ram flash");
    default:
        break;
    }
    return QString();
}

static QString capabilitiesToString(QDriveInfo::Capabilities capabilities)
{
    QStringList result;
    for (int i = 1; i != QDriveInfo::SupportsPersistentIDs << 1; i = i << 1) {
        if (capabilities & i)
            result.append(capabilityToString(QDriveInfo::Capability(i)));
    }
    return result.join(" | ");
}

DriveModel::DriveModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_drives(QDriveInfo::drives())
{
}

int DriveModel::columnCount(const QModelIndex &/*parent*/) const
{
    return ColumnCount;
}

int DriveModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_drives.count();
}

QVariant DriveModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        const QDriveInfo &drive = m_drives.at(index.row());
        switch (index.column()) {
        case ColumnRootPath:
            return QDir::toNativeSeparators(drive.rootPath());
        case ColumnName:
            return drive.name();
        case ColumnDevice:
            return drive.device();
        case ColumnFileSystemName:
            return drive.fileSystemName();
        case ColumnType:
            return typeToString(drive.type());
        case ColumnCapabilities:
            return capabilitiesToString(drive.capabilities());
        case ColumnTotal:
            return sizeToString(drive.bytesTotal());
        case ColumnFree:
            return sizeToString(drive.bytesFree());
        case ColumnAvailable:
            return sizeToString(drive.bytesAvailable());
        case CoulmnIsReady:
            return drive.isReady();
        default:
            break;
        }
    }

    return QVariant();
}

QVariant DriveModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case ColumnRootPath:
        return tr("Root path");
    case ColumnName:
        return tr("Name");
    case ColumnDevice:
        return tr("Device");
    case ColumnFileSystemName:
        return tr("File system");
    case ColumnType:
        return tr("Type");
    case ColumnCapabilities:
        return tr("ColumnCapabilities");
    case ColumnTotal:
        return tr("Total");
    case ColumnFree:
        return tr("Free");
    case ColumnAvailable:
        return tr("Available");
    case CoulmnIsReady:
        return tr("Ready");
    default:
        break;
    }

    return QVariant();
}
