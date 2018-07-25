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

#include "AMLMJobTests.h"

// Std C++
#include <type_traits>
#include <atomic>

#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>


// Qt5
#include <QString>
#include <QTest>
#include <QSignalSpy>

// Ours
#include <future/future_type_traits.hpp>
#include <future/function_traits.hpp>
#include <tests/TestHelpers.h>
#include "../AMLMJob.h"
#include <src/concurrency/DirectoryScanJob.h>


void AMLMJobTests::SetUp()
{
	GTEST_COUT << "SetUp()" << std::endl;
}

void AMLMJobTests::TearDown()
{
	GTEST_COUT << "TearDown()" << std::endl;
}



class TestAMLMJob1;
using TestAMLMJob1Ptr = QPointer<TestAMLMJob1>;
/**
 * Test job derived from AMLMJob.
 */
class TestAMLMJob1 : public AMLMJob, public UniqueIDMixin<TestAMLMJob1>
{
	Q_OBJECT

	using BASE_CLASS = AMLMJob;

	/**
	 * @note CRTP: Still need this to avoid ambiguous name resolution.
	 * @see https://stackoverflow.com/a/46916924
	 */
	using UniqueIDMixin<TestAMLMJob1>::uniqueQObjectName;

protected:
	explicit TestAMLMJob1(QObject* parent) : AMLMJob(parent)
	{
		// Set our object name.
		setObjectName(uniqueQObjectName());
        setCapabilities(KJob::Capability::Killable);
	}

public:
    /// @name Public types
    /// @{
    using ExtFutureType = ExtFuture<int>;
    /// @}

    ~TestAMLMJob1() override = default;

    static TestAMLMJob1Ptr make_job(QObject *parent)
    {
        TestAMLMJob1Ptr retval = new TestAMLMJob1(parent);
        return retval;
    }

    ExtFutureType& get_extfuture_ref() override { return asDerivedTypePtr()->m_ext_future; }
    TestAMLMJob1* asDerivedTypePtr() override { return this; }

    std::atomic_int m_counter {0};

protected:

    void runFunctor() override
    {
        SCOPED_TRACE("InFunctor");

        // Do some work...
        for(int i =0; i<10; i++)
        {
            GTEST_COUT << "Sleeping for 1 second\n";
            QTest::qSleep(1000);

            GTEST_COUT << "Incementing counter\n";
            m_counter++;

            GTEST_COUT << "Reporting counter result\n";
            m_ext_future.reportResult(m_counter);

            if(functorHandlePauseResumeAndCancel())
            {
                // We've been cancelled.
                qIno() << "CANCELLED";
                m_ext_future.reportCanceled();
                break;
            }
        }

        setSuccessFlag(!wasCancelRequested());
    }

private:
    ExtFuture<int> m_ext_future;
};

///
/// Test Cases
///


TEST_F(AMLMJobTests, ThisShouldPass)
{
	ASSERT_FALSE(has_finished(__PRETTY_FUNCTION__));
	ASSERT_TRUE(true);
	finished(__PRETTY_FUNCTION__);
	ASSERT_TRUE(has_finished(__PRETTY_FUNCTION__));
}

TEST_F(AMLMJobTests, SynchronousExecTest)
{
    TestAMLMJob1Ptr j = TestAMLMJob1::make_job(qApp);
    j->setAutoDelete(false);

    QSignalSpy kjob_finished_spy(j, &KJob::finished);
    ASSERT_TRUE(kjob_finished_spy.isValid());
    QSignalSpy kjob_result_spy(j, &KJob::result);
    ASSERT_TRUE(kjob_result_spy.isValid());

    bool status = j->exec();

    EXPECT_EQ(status, true);

    GTEST_COUT << "FINISHED CT:" << kjob_finished_spy.count();
    GTEST_COUT << "RESULT CT:" << kjob_result_spy.count();

    EXPECT_EQ(kjob_finished_spy.count(), 1);
    EXPECT_EQ(kjob_result_spy.count(), 1);
}

TEST_F(AMLMJobTests, ThenTest)
{
    QObject* ctx = new QObject(qApp);
    TestAMLMJob1Ptr j = TestAMLMJob1::make_job(qApp);
    j->setAutoDelete(false);

    QSignalSpy kjob_finished_spy(j, &KJob::finished);
    ASSERT_TRUE(kjob_finished_spy.isValid());
    QSignalSpy kjob_result_spy(j, &KJob::result);
    ASSERT_TRUE(kjob_result_spy.isValid());

    j->then(ctx, [=](TestAMLMJob1* j_kjob) -> void {
        if(j->error())
        {
            // Error.
            GTEST_COUT << "ASYNC JOB FAILED:\r\n"; // << j_kjob->error() << ":" << j_kjob->errorText() << ":" << j_kjob->errorString();
        }
        else
        {
            // Succeeded, update the model.
            GTEST_COUT << "ASYNC JOB COMPLETE:\r\n";// << j_kjob;
            int new_val = j_kjob->get_extfuture_ref().qtget_first();
        }
    });
    // Start the job.
    j->start();

    // Wait for job to finish.
    EXPECT_TRUE(QTest::qWaitFor([&](){return kjob_result_spy.count() == 1; }, 15000));
//    while(!j->isFinished())
//    {
//    }
    j->get_extfuture_ref().waitForFinished();

    GTEST_COUT << "JOB EXTFUTURE:" << j->get_extfuture_ref().state() << "\n";
    QList<int> extf_int = j->get_extfuture_ref().get();

    EXPECT_EQ(extf_int.size(), 10);
}


TEST_F(AMLMJobTests, DirScanCancelTest)
{
    ExtFutureWatcher<DirScanResult> watcher;

	// Dummy dir so the dir scanner job has something to chew on.
    QUrl dir_url = QUrl::fromLocalFile("/");
    RecordProperty("dirscanned", tostdstr(dir_url.toString()));
    DirectoryScannerAMLMJobPtr dsj = DirectoryScannerAMLMJob::make_job(nullptr, dir_url,
	                                    QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
	                                    QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    // Connect signals and slots.
    connect_or_die(dsj, &DirectoryScannerAMLMJob::finished, [=](KJob* kjob){
        qIn() << "GOT FINISHED";
        ASSERT_EQ(kjob, dsj);
        EXPECT_EQ(kjob->error(), KJob::KilledJobError);

        ;});
    connect_or_die(dsj, &DirectoryScannerAMLMJob::result, [=](KJob* kjob){
        qIn() << "GOT RESULT";
        ASSERT_EQ(kjob, dsj);

        EXPECT_EQ(kjob->error(), KJob::KilledJobError);

        ;});

    watcher.setFuture(dsj->get_extfuture_ref());

    // Start the job.
    dsj->start();

//    // Spin waiting for the job to complete.
//    /// Don't think this is waiting.
//    while(watcher.isRunning())
//    {
//        qDb() << "isRunning()";
//        qApp->processEvents();
//    }
//    watcher.waitForFinished();

    // Cancel the job after 2 secs.
    QTest::qSleep(2000);

    dsj->kill();

//    ASSERT_EQ(dsj->get_extfuture_ref(), );
}

TEST_F(AMLMJobTests, CancelTest)
{
    TestAMLMJob1Ptr j = TestAMLMJob1::make_job(nullptr);
    j->setAutoDelete(false);

    QSignalSpy kjob_finished_spy(j, &KJob::finished);
    ASSERT_TRUE(kjob_finished_spy.isValid());
    QSignalSpy kjob_result_spy(j, &KJob::result);
    ASSERT_TRUE(kjob_result_spy.isValid());

    connect_or_die(j, &KJob::finished, qApp, [&](KJob* kjob){
        qDb() << "GOT SIGNAL FINISHED:" << kjob;
                });

    ExtFuture<int> ef = j->get_extfuture_ref();

    // Start the job.
    GTEST_COUT << "START\n";
    j->start();

    ASSERT_TRUE(ef.isStarted()) << ef.state();

    // Let it run for a while.
    QTest::qSleep(500);
    EXPECT_EQ(j->m_counter, 0);
    QTest::qSleep(700);
    EXPECT_EQ(j->m_counter, 1);
    QTest::qSleep(500);

    // Should not be canceled or finished yet.
    ASSERT_FALSE(ef.isCanceled()) << ef;
    ASSERT_FALSE(ef.isFinished()) << ef;

    // Cancel the running job.
    GTEST_COUT << "CANCELING JOB\n";
    bool kill_succeeded = j->kill(KJob::KillVerbosity::Quietly);

    // j is now probably going to be deleteLater()'ed.

    ASSERT_TRUE(kill_succeeded) << ef;
    ASSERT_TRUE(ef.isCanceled()) << ef;
    EXPECT_EQ(j->error(), KJob::KilledJobError);

    // Wait for the KJob to signal that it's finished.
    // Won't get a result() signal here because it's kill()'ed Quietly.
    EXPECT_TRUE(kjob_finished_spy.wait());
    EXPECT_FALSE(kjob_result_spy.wait(500));
}

TEST_F(AMLMJobTests, CancelBeforeStartTest)
{
    RecordProperty("amlmtestproperty", "Test of the RecordProperty() system");

    TestAMLMJob1Ptr j = TestAMLMJob1::make_job(nullptr);
//    j->setAutoDelete(false);

    QSignalSpy kjob_finished_spy(j, &KJob::finished);
    ASSERT_TRUE(kjob_finished_spy.isValid());
    QSignalSpy kjob_result_spy(j, &KJob::result);
    ASSERT_TRUE(kjob_result_spy.isValid());

    connect_or_die(j, &KJob::finished, qApp, [&](KJob* kjob){
        qDb() << "GOT SIGNAL FINISHED:" << kjob;
                });

    ExtFuture<int> ef = j->get_extfuture_ref();

    ASSERT_TRUE(ef.isStarted()) << ef;
    ASSERT_FALSE(ef.isRunning()) << ef;

    // Job hasn't started yet, kill it.
    bool kill_succeeded = j->kill();

    // j is now probably going to be deleteLater()'ed.

    EXPECT_TRUE(kill_succeeded) << ef;
    EXPECT_TRUE(ef.isCanceled()) << ef;
    EXPECT_EQ(j->error(), KJob::KilledJobError);

//    j->start();

//    QTest::qSleep(500);
//    EXPECT_EQ(j->m_counter, 0);
//    QTest::qSleep(700);
//    EXPECT_EQ(j->m_counter, 1);
//    QTest::qSleep(500);

    // Wait for the KJob to signal that it's finished.
    // Won't get a result() signal here because it's kill()'ed Quietly.
    EXPECT_EQ(kjob_finished_spy.count(), 1);
    EXPECT_EQ(kjob_result_spy.count(), 0);
}

#include "AMLMJobTests.moc"
