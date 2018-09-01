/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
 *
 * This file is part of AwesomeMediaLibraryManager.
 *
 * AwesomeMediaLibraryManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AwesomeMediaLibraryManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AwesomeMediaLibraryManager.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtTest>
#include <QSignalSpy>

#include <QString>

//#include "ExtAsyncTestCommon.h"

#include <concurrency/DirectoryScanJob.h>

///////

#define AMLMTEST_COUT qDbo()

#define AMLMTEST_EXPECT_TRUE(arg) QVERIFY(arg)
#define AMLMTEST_ASSERT_TRUE(arg) QVERIFY(arg)
#define AMLMTEST_EXPECT_EQ(arg1, arg2) QCOMPARE(arg1, arg2)
#define AMLMTEST_ASSERT_EQ(arg1, arg2) QCOMPARE(arg1, arg2)
#define AMLMTEST_ASSERT_NE(arg1, arg2) QVERIFY((arg1) != (arg2))

/// Macros for making sure a KJob gets destroyed before the TEST_F() returns.
#define M_QSIGNALSPIES_SET(kjobptr) \
    QSignalSpy kjob_finished_spy(kjobptr, &KJob::finished); \
    AMLMTEST_EXPECT_TRUE(kjob_finished_spy.isValid()); \
    QSignalSpy kjob_result_spy(kjobptr, &KJob::result); \
    AMLMTEST_EXPECT_TRUE(kjob_result_spy.isValid()); \
    QSignalSpy kjob_destroyed_spy(kjobptr, SIGNAL(destroyed(QObject*))); \
    AMLMTEST_EXPECT_TRUE(kjob_destroyed_spy.isValid()); \
    QSignalSpy kjob_destroyed_spy2(kjobptr, SIGNAL(destroyed())); \
    AMLMTEST_EXPECT_TRUE(kjob_destroyed_spy2.isValid());

#define M_QSIGNALSPIES_EXPECT_IF_DESTROY_TIMEOUT() \
    AMLMTEST_EXPECT_TRUE(kjob_destroyed_spy.wait() || kjob_destroyed_spy2.wait());

/// Divisor for ms delays/timeouts in the tests.
constexpr long TC_MS_DIV = 10;

static inline void TC_Sleep(int ms)
{
    QTest::qSleep(ms / TC_MS_DIV);
}

static inline void TC_Wait(int ms)
{
    QTest::qWait(ms / TC_MS_DIV);
}

//////////


class tst_QString: public QObject
{
    Q_OBJECT

private Q_SLOTS:

	/// @name QTest framework slots.
	/// @{
	void initTestCase()
    { qDebug("called before everything else"); }
    void init()
    { qDebug("called before each test function is executed"); }
    void cleanup()
    { qDebug("called after every test function"); }
    void cleanupTestCase()
    { qDebug("alled after the last test function was executed."); }
    /// @}

    /// @name Tests
    /// @{

    void toUpper();

    void DirScanCancelTestPAutodelete_data();
    void DirScanCancelTestPAutodelete();

    /// @}
};

void tst_QString::toUpper()
{
    QString str = "Hello";
    QCOMPARE(str.toUpper(), QString("HELLO"));
}

void tst_QString::DirScanCancelTestPAutodelete_data()
{
    QTest::addColumn<bool>("autodelete");

    QTest::newRow("KJob is autodelete") << true;
    QTest::newRow("KJob is not autodelete") << false;
}

//TEST_P(AMLMJobTestsParameterized, DirScanCancelTestPAutodelete)
void tst_QString::DirScanCancelTestPAutodelete()
{
//	TC_ENTER();

    QFETCH(bool, autodelete);
//    bool autodelete = GetParam();

	// Dummy dir so the dir scanner job has something to chew on.
    QUrl dir_url = QUrl::fromLocalFile("/");
//    RecordProperty("dirscanned", tostdstr(dir_url.toString()));
    DirectoryScannerAMLMJobPtr dsj = DirectoryScannerAMLMJob::make_job(qApp, dir_url,
	                                    QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
	                                    QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    M_QSIGNALSPIES_SET(dsj);

    AMLMTEST_ASSERT_TRUE(dsj->isAutoDelete());

    // Set autodelete or not.
    AMLMTEST_COUT << "Setting Autodelete to:" << autodelete;
    dsj->setAutoDelete(autodelete);

    // Connect signals and slots.
    connect_or_die(dsj, &DirectoryScannerAMLMJob::finished, qApp, [=](KJob* kjob){
        qIn() << "GOT FINISHED";
        AMLMTEST_EXPECT_EQ(kjob, dsj);
        AMLMTEST_EXPECT_EQ(kjob->error(), KJob::KilledJobError);

        ;});
    connect_or_die(dsj, &DirectoryScannerAMLMJob::result, qApp, [=](KJob* kjob){
        qIn() << "GOT RESULT";
        AMLMTEST_EXPECT_EQ(kjob, dsj);

        AMLMTEST_EXPECT_EQ(kjob->error(), KJob::KilledJobError);

        ;});
    connect_or_die(dsj, &QObject::destroyed, qApp, [=](QObject* obj){
        AMLMTEST_COUT << "GOT DESTROYED SIGNAL:" << obj;
    });

    // Dump some info.
    dsj->dumpObjectInfo();

    // Start the job.
    dsj->start();

    // Cancel the job after 1 sec of scanning.
    TC_Wait(1000);

    AMLMTEST_ASSERT_NE(dsj, nullptr);
    dsj->kill();
    AMLMTEST_ASSERT_NE(dsj, nullptr);

    // Wait for the cancel to finish.
    AMLMTEST_ASSERT_TRUE(kjob_result_spy.wait());
    // We make sure it was a cancel in the result handler above.

    if(!autodelete)
    {
        dsj->deleteLater();
    }

    AMLMTEST_COUT << "Waiting for destroyed signal";
    TC_Wait(100);
    // Wait for the (auto-)delete to happen.
    M_QSIGNALSPIES_EXPECT_IF_DESTROY_TIMEOUT();
//    bool didnt_time_out = kjob_destroyed_spy.wait();
    AMLMTEST_COUT << "Done Waiting for destroyed signal";
//    EXPECT_TRUE(didnt_time_out);

//    TC_EXIT();
}

QTEST_MAIN(tst_QString)

#include "QTestAMLMJobTests.moc"
