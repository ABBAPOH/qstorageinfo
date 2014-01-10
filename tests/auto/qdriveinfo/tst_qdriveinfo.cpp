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

#include <QDriveInfo>

class tst_QDriveInfo : public QObject
{
    Q_OBJECT
public:
    tst_QDriveInfo() {}

private slots:
    void testInvalidDrive();
    void testOperatorEqual();
    void testOperatorNotEqual();
    void testRootDrive();
    void testCurrentDrive();
    void testDriveList();
    void testTempFile();
    void testCaching();
};

void tst_QDriveInfo::testInvalidDrive()
{
    QDriveInfo drive;

    QVERIFY(!drive.isValid());
    QVERIFY(!drive.isReady());
    QVERIFY(drive.rootPath().isEmpty());
    QVERIFY(!drive.isRoot());
    QVERIFY(drive.type() == QDriveInfo::UnknownDrive);
    QVERIFY(drive.device().isEmpty());
    QVERIFY(drive.fileSystemName().isEmpty());
    QVERIFY(drive.bytesTotal() == 0);
    QVERIFY(drive.bytesFree() == 0);
    QVERIFY(drive.bytesAvailable() == 0);
}

void tst_QDriveInfo::testOperatorEqual()
{
    {
        QDriveInfo drive1 = QDriveInfo::rootDrive();
        QDriveInfo drive2(QDir::rootPath());
        QVERIFY(drive1 == drive2);
    }

    {
        QDriveInfo drive1(QCoreApplication::applicationDirPath());
        QDriveInfo drive2(QCoreApplication::applicationFilePath());
        QVERIFY(drive1 == drive2);
    }

    {
        QDriveInfo drive1;
        QDriveInfo drive2;
        QVERIFY(drive1 == drive2);
    }
}

void tst_QDriveInfo::testOperatorNotEqual()
{
    QDriveInfo drive1 = QDriveInfo::rootDrive();
    QDriveInfo drive2;
    QVERIFY(drive1 != drive2);
}

void tst_QDriveInfo::testRootDrive()
{
    QDriveInfo drive = QDriveInfo::rootDrive();

    QVERIFY(drive.isValid());
    QVERIFY(drive.isReady());
    QCOMPARE(drive.rootPath(), QDir::rootPath());
    QVERIFY(drive.isRoot());
    QVERIFY(drive.type() != QDriveInfo::UnknownDrive);
    QVERIFY(!drive.device().isEmpty());
    QVERIFY(!drive.fileSystemName().isEmpty());
    QVERIFY(drive.bytesTotal() > 0);
    QVERIFY(drive.bytesFree() > 0);
    QVERIFY(drive.bytesAvailable() > 0);
}

void tst_QDriveInfo::testCurrentDrive()
{
    QString appPath = QCoreApplication::applicationFilePath();
    QDriveInfo drive(appPath);
    QVERIFY(drive.isValid());
    QVERIFY(drive.isReady());
    QVERIFY(appPath.startsWith(drive.rootPath(), Qt::CaseInsensitive));
    QVERIFY(drive.type() != QDriveInfo::UnknownDrive);
#ifndef Q_OS_WIN
    QVERIFY(!drive.device().isEmpty());
#else
    // remote drives have no device on Windows
    if (drive.type() != QDriveInfo::RemoteDrive)
        QVERIFY(!drive.device().isEmpty());
#endif
    QVERIFY(!drive.fileSystemName().isEmpty());
    QVERIFY(drive.bytesTotal() > 0);
    QVERIFY(drive.bytesFree() > 0);
    QVERIFY(drive.bytesAvailable() > 0);
}

void tst_QDriveInfo::testDriveList()
{
    QDriveInfo rootDrive = QDriveInfo::rootDrive();

    QList<QDriveInfo> drives = QDriveInfo::drives();

    // at least, root drive should be present
    QVERIFY(drives.contains(rootDrive));
    drives.removeOne(rootDrive);
    QVERIFY(!drives.contains(rootDrive));

    foreach (const QDriveInfo &drive, drives) {
        if (!drive.isReady())
            continue;

        QVERIFY(drive.isValid());
        QVERIFY(!drive.isRoot());
#ifndef Q_OS_WIN
        QVERIFY(!drive.device().isEmpty());
        QVERIFY(!drive.fileSystemName().isEmpty());
#else
        if (drive.type() != QDriveInfo::CdromDrive || drive.type() != QDriveInfo::RemovableDrive)
            QVERIFY(!drive.fileSystemName().isEmpty());
#endif
    }
}

void tst_QDriveInfo::testTempFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());

    QDriveInfo drive1(file.fileName());
    qint64 free = drive1.bytesFree();

    file.write(QByteArray(1024*1024, '1'));
    file.flush();
    file.close();

    QDriveInfo drive2(file.fileName());
    QVERIFY(free != drive2.bytesFree());
}

void tst_QDriveInfo::testCaching()
{
    QTemporaryFile file;
    QVERIFY(file.open());

    QDriveInfo drive1(file.fileName());
    qint64 free = drive1.bytesFree();
    QDriveInfo drive2(drive1);
    QVERIFY(free == drive2.bytesFree());

    file.write(QByteArray(1024*1024, '\0'));
    file.flush();

    QVERIFY(free == drive1.bytesFree());
    QVERIFY(free == drive2.bytesFree());
    drive2.refresh();
    QVERIFY(drive1 == drive2);
    QVERIFY(free != drive2.bytesFree());
}

QTEST_MAIN(tst_QDriveInfo)

#include "tst_qdriveinfo.moc"
