/****************************************************************************
**
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

#include <QStorageInfo>

class tst_QStorageInfo : public QObject
{
    Q_OBJECT
private slots:
    void defaultValues();
    void operatorEqual();
#ifndef Q_OS_WINRT
    void operatorNotEqual();
    void rootVolume();
    void currentVolume();
    void volumeList();
    void tempFile();
    void caching();
#endif
};

void tst_QStorageInfo::defaultValues()
{
    QStorageInfo volume;

    QVERIFY(!volume.isValid());
    QVERIFY(!volume.isReady());
    QVERIFY(volume.rootPath().isEmpty());
    QVERIFY(!volume.isRoot());
    QVERIFY(volume.device().isEmpty());
    QVERIFY(volume.fileSystemType().isEmpty());
    QVERIFY(volume.bytesTotal() == 0);
    QVERIFY(volume.bytesFree() == 0);
    QVERIFY(volume.bytesAvailable() == 0);
}

void tst_QStorageInfo::operatorEqual()
{
    {
        QStorageInfo volume1 = QStorageInfo::rootVolume();
        QStorageInfo volume2(QDir::rootPath());
        QVERIFY(volume1 == volume2);
    }

    {
        QStorageInfo volume1(QCoreApplication::applicationDirPath());
        QStorageInfo volume2(QCoreApplication::applicationFilePath());
        QVERIFY(volume1 == volume2);
    }

    {
        QStorageInfo volume1;
        QStorageInfo volume2;
        QVERIFY(volume1 == volume2);
    }
}

#ifndef Q_OS_WINRT
void tst_QStorageInfo::operatorNotEqual()
{
    QStorageInfo volume1 = QStorageInfo::rootVolume();
    QStorageInfo volume2;
    QVERIFY(volume1 != volume2);
}

void tst_QStorageInfo::rootVolume()
{
    QStorageInfo volume = QStorageInfo::rootVolume();

    QVERIFY(volume.isValid());
    QVERIFY(volume.isReady());
    QCOMPARE(volume.rootPath(), QDir::rootPath());
    QVERIFY(volume.isRoot());
    QVERIFY(!volume.device().isEmpty());
    QVERIFY(!volume.fileSystemType().isEmpty());
    QVERIFY(volume.bytesTotal() > 0);
    QVERIFY(volume.bytesFree() > 0);
    QVERIFY(volume.bytesAvailable() > 0);
}

void tst_QStorageInfo::currentVolume()
{
    QString appPath = QCoreApplication::applicationFilePath();
    QStorageInfo volume(appPath);
    QVERIFY(volume.isValid());
    QVERIFY(volume.isReady());
    QVERIFY(appPath.startsWith(volume.rootPath(), Qt::CaseInsensitive));
    QVERIFY(!volume.device().isEmpty());
    QVERIFY(!volume.fileSystemType().isEmpty());
    QVERIFY(volume.bytesTotal() > 0);
    QVERIFY(volume.bytesFree() > 0);
    QVERIFY(volume.bytesAvailable() > 0);
}

void tst_QStorageInfo::volumeList()
{
    QStorageInfo rootVolume = QStorageInfo::rootVolume();

    QList<QStorageInfo> volumes = QStorageInfo::volumes();

    // at least, root volume should be present
    QVERIFY(volumes.contains(rootVolume));
    volumes.removeOne(rootVolume);
    QVERIFY(!volumes.contains(rootVolume));

    foreach (const QStorageInfo &volume, volumes) {
        if (!volume.isReady())
            continue;

        QVERIFY(volume.isValid());
        QVERIFY(!volume.isRoot());
#ifndef Q_OS_WIN
        QVERIFY(!volume.device().isEmpty());
        QVERIFY(!volume.fileSystemType().isEmpty());
#endif
    }
}

void tst_QStorageInfo::tempFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());

    QStorageInfo volume1(file.fileName());
    qint64 free = volume1.bytesFree();

    file.write(QByteArray(1024*1024, '1'));
    file.flush();
    file.close();

    QStorageInfo volume2(file.fileName());
    QVERIFY(free != volume2.bytesFree());
}

void tst_QStorageInfo::caching()
{
    QTemporaryFile file;
    QVERIFY(file.open());

    QStorageInfo volume1(file.fileName());
    qint64 free = volume1.bytesFree();
    QStorageInfo volume2(volume1);
    QVERIFY(free == volume2.bytesFree());

    file.write(QByteArray(1024*1024, '\0'));
    file.flush();

    QVERIFY(free == volume1.bytesFree());
    QVERIFY(free == volume2.bytesFree());
    volume2.refresh();
    QVERIFY(volume1 == volume2);
    QVERIFY(free != volume2.bytesFree());
}
#endif

QTEST_MAIN(tst_QStorageInfo)

#include "tst_qstorageinfo.moc"