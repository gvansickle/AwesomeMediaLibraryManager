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

#include "ExtAsyncTestCommon.h"

#include <logic/jobs/DirectoryScanJob.h>

/**
 * From a lambda passed to ExtAsync::run(), iterates @a num_iteration times,
 * QTest::qSleep()ing for 1 sec, then returns the the next value in the sequence to the returned ExtFuture<>.
 *
 * @todo Doesn't handle cancellation or progress reporting.
 */
template<class ReturnFutureT>
ReturnFutureT async_int_generator(int start_val, int num_iterations/*, ExtAsyncTestsSuiteFixtureBase *fixture*/)
{
//    SCOPED_TRACE("In async_int_generator");

//    auto tgb = new trackable_generator_base(fixture);
//    fixture->register_generator(tgb);

	auto lambda = [=](ReturnFutureT future) -> void {
        int current_val = start_val;
//        SCOPED_TRACE("In async_int_generator callback");
        for(int i=0; i<num_iterations; i++)
        {
			/// @todo Not sure if we want this to work or not.
			// AMLMTEST_EXPECT_FALSE(ExtFutureState::state(future) & ExtFutureState::Canceled);

            // Sleep for a second.
			AMLMTEST_COUT << "SLEEPING FOR 1 SEC";

            TC_Sleep(1000);
            AMLMTEST_COUT << "SLEEP COMPLETE, sending value to future:" << current_val;

			reportResult(&future, current_val);
            current_val++;
        }

        // We're done.
        AMLMTEST_COUT << "REPORTING FINISHED";
//        fixture->unregister_generator(tgb);
//        delete tgb;

		reportFinished(&future);
    };

    ReturnFutureT retval;
    if constexpr (std::is_same_v<ReturnFutureT, QFuture<int>>)
    {
        // QFuture() creates an empty, cancelled future (Start|Canceled|Finished).
    	AMLMTEST_COUT << "QFuture<>, clearing state";
        retval = make_startedNotCanceled_QFuture<int>();
    }

    // AMLMTEST_COUT << "ReturnFuture initial state:" << ExtFutureState::state(retval);

//	AMLMTEST_EXPECT_TRUE(retval.isStarted());
//	AMLMTEST_EXPECT_FALSE(retval.isFinished());

    if constexpr (std::is_same_v<ReturnFutureT, QFuture<int>>)
    {
    	AMLMTEST_COUT << "QtConcurrent::run()";
        auto qrunfuture = QtConcurrent::run(lambda, retval);
    }
    else
    {
    	AMLMTEST_COUT << "ExtAsync::run_efarg()";
		retval = QtConcurrent::run(std::move(lambda));
    }

    // AMLMTEST_COUT << "RETURNING future:" << ExtFutureState::state(retval);

//    AMLMTEST_EXPECT_TRUE(retval.isStarted());
//    if constexpr (std::is_same_v<ReturnFutureT, QFuture<int>>)
//    {
//        // QFuture starts out Start|Canceled|Finished.
//        EXPECT_TRUE(retval.isCanceled());
//        EXPECT_TRUE(retval.isFinished());
//    }
//    else
//    {
//        EXPECT_FALSE(retval.isCanceled());
//        EXPECT_FALSE(retval.isFinished());
//    }

    return retval;
}

//////////


class tst_QString: public ExtAsyncTestsSuiteFixtureBase //public QObject
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

//	void ExtFutureStreamingTap();

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

	QTest::newRow("KJob is not autodelete") << false;
    QTest::newRow("KJob is autodelete") << true;
}

//TEST_P(AMLMJobTestsParameterized, DirScanCancelTestPAutodelete)
void tst_QString::DirScanCancelTestPAutodelete()
{
//	TC_ENTER();

	QSKIP("");

    QFETCH(bool, autodelete);
//    bool autodelete = GetParam();

	// Dummy dir so the dir scanner job has something to chew on.
    QUrl dir_url = QUrl::fromLocalFile("/");
//    RecordProperty("dirscanned", tostdstr(dir_url.toString()));
#if 0 // OBSOLETE
    DirectoryScannerAMLMJobPtr dsj = DirectoryScannerAMLMJob::make_job(qApp, dir_url,
	                                    QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
	                                    QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
#endif
	// Run the directory scan in another thread.
	ExtFuture<DirScanResult> dirresults_future = QtConcurrent::run(DirScanFunction,
																						 dir_url,
																						 QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
																						 QDir::Filters(QDir::Files |
																									   QDir::AllDirs |
																									   QDir::NoDotAndDotDot),
																						 QDirIterator::Subdirectories);
	auto dsj = make_async_AMLMJobT(dirresults_future);


    M_QSIGNALSPIES_SET(dsj);

    AMLMTEST_ASSERT_TRUE(dsj->isAutoDelete());

    // Set autodelete or not.
    AMLMTEST_COUT << "Setting Autodelete to:" << autodelete;
    dsj->setAutoDelete(autodelete);

#if 0 /// DirectoryScannerAMLMJob is gone
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
#endif
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

/**
 * Test "streaming" tap().
 * @todo Currently crashes.
 */
//TEST_F(ExtFutureTest, ExtFutureStreamingTap)
#if 0
void tst_QString::ExtFutureStreamingTap()
{
	TC_ENTER();

	static std::atomic_int num_tap_completions {0};

	QList<int> expected_results {1,2,3,4,5,6};
	ExtFuture<int> ef = async_int_generator<ExtFuture<int>>(1, 6, nullptr/*, this*/);

	AMLMTEST_COUT << "Starting ef state:" << ef.state();

	AMLMTEST_ASSERT_TRUE(ef.isStarted());
	AMLMTEST_ASSERT_FALSE(ef.isCanceled());
	AMLMTEST_ASSERT_FALSE(ef.isFinished());

	QList<int> async_results_from_tap, async_results_from_get;

	AMLMTEST_COUT << "Attaching tap and get()";
M_WARNING("TODO");
//ExtFuture<int> f2;
#if 1
//    async_results_from_get =
	auto f2 = ef.tap(qApp->instance(), [=, &async_results_from_tap](ExtFuture<int> ef, int begin, int end)  {
			AMLMTEST_COUT << "IN TAP, begin:" << begin << ", end:" << end;
		for(int i = begin; i<end; i++)
		{
			AMLMTEST_COUT << "Pushing" << ef.resultAt(i) << "to tap list.";
			async_results_from_tap.push_back(ef.resultAt(i));
			num_tap_completions++;
		}
	});
#endif
	AMLMTEST_COUT << "BEFORE WAITING FOR GET()" << f2;

	f2.waitForFinished();

	AMLMTEST_COUT << "AFTER WAITING FOR GET()" << f2;
	async_results_from_get = ef.results();

	AMLMTEST_EXPECT_TRUE(ef.isFinished());
	AMLMTEST_EXPECT_EQ(num_tap_completions, 6);

	// .get() above should block.
	AMLMTEST_EXPECT_TRUE(ef.isFinished());

	// This shouldn't do anything, should already be finished.
	ef.waitForFinished();

	AMLMTEST_COUT << "Post .tap().get(), extfuture:" << ef.state();

	AMLMTEST_EXPECT_TRUE(ef.isStarted());
	AMLMTEST_EXPECT_FALSE(ef.isCanceled());
	AMLMTEST_EXPECT_TRUE(ef.isFinished());

	AMLMTEST_EXPECT_EQ(async_results_from_get.size(), 6);
	AMLMTEST_EXPECT_EQ(async_results_from_get, expected_results);
	AMLMTEST_EXPECT_EQ(async_results_from_tap.size(), 6);
	AMLMTEST_EXPECT_EQ(async_results_from_tap, expected_results);

	AMLMTEST_ASSERT_TRUE(ef.isFinished());

//	TC_EXIT();
}
#endif

QTEST_MAIN(tst_QString)

#include "QTestAMLMJobTests.moc"
