/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2014 Ivan Komissarov
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtTest/QtTest>

#include <QVolumeInfo>

class tst_QVolumeInfo : public QObject
{
    Q_OBJECT
public:
    tst_QVolumeInfo() {}

private slots:
    void testInvalidVolume();
    void testOperatorEqual();
    void testOperatorNotEqual();
    void testRootVolume();
    void testCurrentVolume();
    void testVolumeList();
    void testTempFile();
    void testCaching();
};

void tst_QVolumeInfo::testInvalidVolume()
{
    QVolumeInfo volume;

    QVERIFY(!volume.isValid());
    QVERIFY(!volume.isReady());
    QVERIFY(volume.rootPath().isEmpty());
    QVERIFY(!volume.isRoot());
    QVERIFY(volume.typeFlags() == QVolumeInfo::UnknownVolume);
    QVERIFY(volume.device().isEmpty());
    QVERIFY(volume.fileSystemName().isEmpty());
    QVERIFY(volume.bytesTotal() == 0);
    QVERIFY(volume.bytesFree() == 0);
    QVERIFY(volume.bytesAvailable() == 0);
}

void tst_QVolumeInfo::testOperatorEqual()
{
    {
        QVolumeInfo volume1 = QVolumeInfo::rootVolume();
        QVolumeInfo volume2(QDir::rootPath());
        QVERIFY(volume1 == volume2);
    }

    {
        QVolumeInfo volume1(QCoreApplication::applicationDirPath());
        QVolumeInfo volume2(QCoreApplication::applicationFilePath());
        QVERIFY(volume1 == volume2);
    }

    {
        QVolumeInfo volume1;
        QVolumeInfo volume2;
        QVERIFY(volume1 == volume2);
    }
}

void tst_QVolumeInfo::testOperatorNotEqual()
{
    QVolumeInfo volume1 = QVolumeInfo::rootVolume();
    QVolumeInfo volume2;
    QVERIFY(volume1 != volume2);
}

void tst_QVolumeInfo::testRootVolume()
{
    QVolumeInfo volume = QVolumeInfo::rootVolume();

    QVERIFY(volume.isValid());
    QVERIFY(volume.isReady());
    QCOMPARE(volume.rootPath(), QDir::rootPath());
    QVERIFY(volume.isRoot());
    QVERIFY(volume.typeFlags() != QVolumeInfo::UnknownVolume);
    QVERIFY(!volume.device().isEmpty());
    QVERIFY(!volume.fileSystemName().isEmpty());
    QVERIFY(volume.bytesTotal() > 0);
    QVERIFY(volume.bytesFree() > 0);
    QVERIFY(volume.bytesAvailable() > 0);
}

void tst_QVolumeInfo::testCurrentVolume()
{
    QString appPath = QCoreApplication::applicationFilePath();
    QVolumeInfo volume(appPath);
    QVERIFY(volume.isValid());
    QVERIFY(volume.isReady());
    QVERIFY(appPath.startsWith(volume.rootPath(), Qt::CaseInsensitive));
    QVERIFY(volume.typeFlags() != QVolumeInfo::UnknownVolume);
    QVERIFY(!volume.device().isEmpty());
    QVERIFY(!volume.fileSystemName().isEmpty());
    QVERIFY(volume.bytesTotal() > 0);
    QVERIFY(volume.bytesFree() > 0);
    QVERIFY(volume.bytesAvailable() > 0);
}

void tst_QVolumeInfo::testVolumeList()
{
    QVolumeInfo rootVolume = QVolumeInfo::rootVolume();

    QList<QVolumeInfo> volumes = QVolumeInfo::volumes();

    // at least, root volume should be present
    QVERIFY(volumes.contains(rootVolume));
    volumes.removeOne(rootVolume);
    QVERIFY(!volumes.contains(rootVolume));

    foreach (const QVolumeInfo &volume, volumes) {
        if (!volume.isReady())
            continue;

        QVERIFY(volume.isValid());
        QVERIFY(!volume.isRoot());
#ifndef Q_OS_WIN
        QVERIFY(!volume.device().isEmpty());
        QVERIFY(!volume.fileSystemName().isEmpty());
#else
        if (!(volume.typeFlags() & QVolumeInfo::RemovableVolume))
            QVERIFY(!volume.fileSystemName().isEmpty());
#endif
    }
}

void tst_QVolumeInfo::testTempFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());

    QVolumeInfo volume1(file.fileName());
    qint64 free = volume1.bytesFree();

    file.write(QByteArray(1024*1024, '1'));
    file.flush();
    file.close();

    QVolumeInfo volume2(file.fileName());
    QVERIFY(free != volume2.bytesFree());
}

void tst_QVolumeInfo::testCaching()
{
    QTemporaryFile file;
    QVERIFY(file.open());

    QVolumeInfo volume1(file.fileName());
    qint64 free = volume1.bytesFree();
    QVolumeInfo volume2(volume1);
    QVERIFY(free == volume2.bytesFree());

    file.write(QByteArray(1024*1024, '\0'));
    file.flush();

    QVERIFY(free == volume1.bytesFree());
    QVERIFY(free == volume2.bytesFree());
    volume2.refresh();
    QVERIFY(volume1 == volume2);
    QVERIFY(free != volume2.bytesFree());
}

QTEST_MAIN(tst_QVolumeInfo)

#include "tst_qvolumeinfo.moc"
