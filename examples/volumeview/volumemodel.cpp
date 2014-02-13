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

#include "volumemodel.h"

#include <QDir>
#include <qmath.h>

static QString sizeToString(qint64 size)
{
    static const char *const strings[] = { "b", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };

    if (size <= 0)
        return VolumeModel::tr("0 b");

    double power = log((double)size)/log(1024.0);
    int intPower = (int)power;
    intPower = intPower >= 8 ? 8 - 1 : intPower;

    double normSize = size / pow(1024.0, intPower);
    //: this should expand to "1.23 GB"
    return VolumeModel::tr("%1 %2").arg(normSize, 0, 'f', intPower > 0 ? 2 : 0).arg(strings[intPower]);
}

static QString capabilityToString(QVolumeInfo::Capability capability)
{
    switch (capability) {
    case QVolumeInfo::SupportsSymbolicLinks:
        return VolumeModel::tr("SupportsSymbolicLinks");
    case QVolumeInfo::SupportsHardLinks:
        return VolumeModel::tr("SupportsHardLinks");
    case QVolumeInfo::SupportsCaseSensitiveNames:
        return VolumeModel::tr("SupportsCaseSensitiveNames");
    case QVolumeInfo::SupportsCasePreservedNames:
        return VolumeModel::tr("SupportsCasePreservedNames");
    case QVolumeInfo::SupportsJournaling:
        return VolumeModel::tr("SupportsJournaling");
    case QVolumeInfo::SupportsSparseFiles:
        return VolumeModel::tr("SupportsSparseFiles");
    case QVolumeInfo::SupportsPersistentIDs:
        return VolumeModel::tr("SupportsPersistentIDs");
    }
    return QString();
}

static QString typeToString(QVolumeInfo::VolumeTypeFlag type)
{
    switch (type) {
    case QVolumeInfo::UnknownVolume:
        return VolumeModel::tr("Unknown");
    case QVolumeInfo::InternalVolume:
        return VolumeModel::tr("Internal");
    case QVolumeInfo::RemovableVolume:
        return VolumeModel::tr("Removable");
    case QVolumeInfo::RemoteVolume:
        return VolumeModel::tr("Remote");
    case QVolumeInfo::OpticalVolume:
        return VolumeModel::tr("Cdrom");
    case QVolumeInfo::RamVolume:
        return VolumeModel::tr("RAM flash");
    default:
        break;
    }
    return QString();
}

static QString capabilitiesToString(QVolumeInfo::Capabilities capabilities)
{
    QStringList result;
    for (int i = 1; i != QVolumeInfo::SupportsPersistentIDs << 1; i = i << 1) {
        if (capabilities & i)
            result.append(capabilityToString(QVolumeInfo::Capability(i)));
    }
    return result.join(" | ");
}

static QString typesToString(QVolumeInfo::VolumeTypeFlags typeFlags)
{
    QStringList result;
    for (int i = 1; i != QVolumeInfo::RamVolume << 1; i = i << 1) {
        if (typeFlags & i)
            result.append(typeToString(QVolumeInfo::VolumeTypeFlag(i)));
    }
    return result.join(" | ");
}

VolumeModel::VolumeModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_volumes(QVolumeInfo::volumes())
{
}

int VolumeModel::columnCount(const QModelIndex &/*parent*/) const
{
    return ColumnCount;
}

int VolumeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_volumes.count();
}

QVariant VolumeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        const QVolumeInfo &volume = m_volumes.at(index.row());
        switch (index.column()) {
        case ColumnRootPath:
            return QDir::toNativeSeparators(volume.rootPath());
        case ColumnName:
            return volume.name();
        case ColumnDevice:
            return volume.device();
        case ColumnFileSystemName:
            return volume.fileSystemName();
        case ColumnType:
            return typesToString(volume.typeFlags());
        case ColumnCapabilities:
            return capabilitiesToString(volume.capabilities());
        case ColumnTotal:
            return sizeToString(volume.bytesTotal());
        case ColumnFree:
            return sizeToString(volume.bytesFree());
        case ColumnAvailable:
            return sizeToString(volume.bytesAvailable());
        case CoulmnIsReady:
            return volume.isReady();
        default:
            break;
        }
    } else if (role == Qt::ToolTipRole) {
        const QVolumeInfo &volume = m_volumes.at(index.row());
        return tr("Root path : %1\n"
                  "Name: %2\n"
                  "Device: %3\n"
                  "FileSystem: %4\n"
                  "Type: %5\n"
                  "Capabilities: %6\n"
                  "Total size: %7\n"
                  "Free size: %8\n"
                  "Available size: %9\n"
                  "Is Ready: %10"
                  ).
                arg(QDir::toNativeSeparators(volume.rootPath())).
                arg(volume.name()).
                arg(QString::fromUtf8(volume.device())).
                arg(QString::fromUtf8(volume.fileSystemName())).
                arg(typesToString(volume.typeFlags())).
                arg(capabilitiesToString(volume.capabilities())).
                arg(sizeToString(volume.bytesTotal())).
                arg(sizeToString(volume.bytesFree())).
                arg(sizeToString(volume.bytesAvailable())).
                arg(volume.isReady() ? tr("true") : tr("false"));
    }

    return QVariant();
}

QVariant VolumeModel::headerData(int section, Qt::Orientation orientation, int role) const
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
