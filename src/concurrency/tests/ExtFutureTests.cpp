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

#include "ExtFutureTests.h"

// Std C++
#include <type_traits>
#include <atomic>
#include <functional>
#include <cstdint>

// Future Std C++
#include <future/future_type_traits.hpp>
#include <future/function_traits.hpp>

// Qt5
#include <QString>
#include <QTest>
#include <QFutureInterfaceBase> // shhh, we're not supposed to use this.  For calling .reportFinished() on QFuture<>s inside a run().

// Google Test
#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>

// Ours
#include <tests/TestHelpers.h>
#include "ExtAsyncTestCommon.h"

#include "../ExtAsync.h"

#include "../ExtFuture.h"
#include <concurrency/ReportingRunner.h>


/// Types for gtest's "Typed Test" support.
//using FutureTypes = ::testing::Types<ExtFuture<int>, QFuture<int>>;
//TYPED_TEST_CASE(ExtFutureTest, FutureTypes);

//
// TESTS
//

//TEST_F(ExtFutureTest, NestedQTestWrapper)
//{
//	// Trying out https://stackoverflow.com/questions/39032462/can-i-check-the-gtest-filter-from-inside-a-non-gtest-test
////	tst_QString test;
////	ASSERT_NE(QTEST_FAILED, QTest::exec(&test, 0, 0));
//}

TEST_F(ExtFutureTest, ReadyFutureCompletion)
{
    TC_ENTER();

    /// @note Important safety tip: nL and nLL are different sizes on Windows vs. Linux.
    /// <cstdint> to the rescue.
    ExtFuture<int64_t> ef = make_ready_future(INT64_C(25));

    QList<int64_t> results = ef.get();

    EXPECT_EQ(ef.state(), ExtFutureState::Started | ExtFutureState::Finished);

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], 25L);

    TC_DONE_WITH_STACK();
    TC_EXIT();
}

TEST_F(ExtFutureTest, FutureSingleThread)
{
    TC_ENTER();

    ExtFuture<int> ef;

    EXPECT_EQ(ef.state(), ExtFutureState::Started | ExtFutureState::Running);

    ef.reportResult(1);

    EXPECT_EQ(ef.resultCount(), 1);
//    EXPECT_EQ(ef.get()[0], 1);
    EXPECT_EQ(ef.result(), 1);

    ef.reportResult(2);

    EXPECT_EQ(ef.resultCount(), 2);
//    EXPECT_EQ(ef.get()[1], 2);
    EXPECT_EQ(ef.resultAt(1), 2);

    TC_DONE_WITH_STACK();
    TC_EXIT();
}

template <template <typename T> class FutureType>
void CopyAssign()
{
    /// Default constructors.
    /// QFuture<> default constructs to Started|Canceled|Finished.
    FutureType<int> extfuture_int;

    FutureType<QString> extfuture_string;

    FutureType<Unit> ef_unit;

    FutureType<Unit> ef_unit2;


    // copy constructor
    FutureType<int> ef_int2(extfuture_int);
    FutureType<Unit> ef_unit3(ef_unit2);

    // Assignment operator
    ef_int2 = FutureType<int>();
    ef_unit3 = FutureType<Unit>();

    // state
    ASSERT_EQ(ef_int2.isStarted(), true);
    /// @note This is a difference between QFuture<> and ExtFuture<>, there's no reason this future should be finished here.
//    ASSERT_EQ(ef_int2.isFinished(), true);
}

TEST_F(ExtFutureTest, CopyAssignTests)
{
    SCOPED_TRACE("CopyAssignTests");

    TC_ENTER();

    CopyAssign<QFuture>();

    CopyAssign<ExtFuture>();

    TC_DONE_WITH_STACK();
    TC_EXIT();
}

/**
 * Test basic cancel properties.
 */
TEST_F(ExtFutureTest, ExtFutureBasicCancel)
{
    TC_ENTER();

    ExtFuture<Unit> f;

    qDb() << "Starting extfuture:" << f;

    ASSERT_TRUE(f.isStarted());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_FALSE(f.isFinished());

    f.cancel();

    qDb() << "Cancelled extfuture:" << f;

    ASSERT_TRUE(f.isStarted());
    ASSERT_TRUE(f.isCanceled());

    // Canceling alone won't finish the extfuture.
    ASSERT_FALSE(f.isFinished());

    f.reportFinished();
    f.waitForFinished();

    ASSERT_TRUE(f.isFinished());

    qDb() << "Cancelled and finished extfuture:" << f;

    TC_DONE_WITH_STACK();
    TC_EXIT();
}


/**
 * Cancel the Promise side, see if the Future side detects it.
 */
TEST_F(ExtFutureTest, ExtFutureCancelPromise)
{
    TC_ENTER();

    ExtFuture<Unit> f;
    ExtFuture<Unit> result;

    result.reportStarted();
    f = result;
    ASSERT_FALSE(f.isCanceled());
    result.reportCanceled();
    ASSERT_TRUE(f.isCanceled());
    result.reportFinished();
    ASSERT_TRUE(f.isCanceled());
    result.waitForFinished();
    ASSERT_TRUE(f.isCanceled());

    TC_DONE_WITH_STACK();
    TC_EXIT();
}

/**
 * Cancel the Future side, see if the promise side detects it.
 */
TEST_F(ExtFutureTest, ExtFutureCancelFuture)
{
    TC_ENTER();

    ExtFuture<Unit> promise_side;
    ExtFuture<Unit> future_side;

    ASSERT_TRUE(future_side.isStarted());

    promise_side.reportStarted();
    future_side = promise_side;

    ASSERT_TRUE(future_side.isStarted());

    ASSERT_FALSE(promise_side.isCanceled());
    future_side.cancel();

    ASSERT_TRUE(promise_side.isCanceled());

    promise_side.reportFinished();

    TC_DONE_WITH_STACK();
    TC_EXIT();
}


template <class FutureType, class TestFixtureType>
QList<int> results_test(int startval, int iterations, TestFixtureType* fixture)
{
    SCOPED_TRACE("In results_test");

    GTEST_COUT_qDB << "START GENERATOR";

    FutureType f = async_int_generator<FutureType>(startval, iterations, fixture);
//    EXPECT_TRUE(f.isStarted());
//    EXPECT_FALSE(f.isFinished());

    GTEST_COUT_qDB << "START RESULTS()" << ExtFutureState::state(f);
    QList<int> retval = f.results();
    GTEST_COUT_qDB << "END RESULTS():" << ExtFutureState::state(f);

    // .results() should block until future is finished.
    EXPECT_TRUE(f.isStarted());
    EXPECT_TRUE(f.isFinished());

    return retval;
}

/**
 * Test QFuture results().
 */
TEST_F(ExtFutureTest, QFutureResults)
{
    SCOPED_TRACE("QFutureResults");

    TC_ENTER();

    QList<int> expected_results {2,3,4,5,6};
    QList<int> results = results_test<QFuture<int>>(2, 5, this);

    EXPECT_EQ(results, expected_results);

    TC_EXIT();
}

/**
 * Test ExtFuture<> results().
 */
TEST_F(ExtFutureTest, Results)
{
    SCOPED_TRACE("Results");

    TC_ENTER();

    QList<int> expected_results {2,3,4,5,6};
    QList<int> results = results_test<ExtFuture<int>>(2, 5, this);

    EXPECT_EQ(results, expected_results);

    TC_EXIT();
}


template <class FutureType, class TestFixtureType>
QList<int> streaming_tap_test(int startval, int iterations, TestFixtureType* fixture)
{
	AMLMTEST_SCOPED_TRACE("IN STT");

	static std::atomic_int num_tap_completions {0};
	QList<int> async_results_from_tap, async_results_from_get;


	FutureType ef = async_int_generator<FutureType>(startval, iterations, fixture);

	GTEST_COUT_qDB << "Starting ef state:" << ExtFutureState::state(ef);
	EXPECT_TRUE(ef.isStarted());
	EXPECT_FALSE(ef.isCanceled());
	EXPECT_FALSE(ef.isFinished());

	QList<int> expected_results {1,2,3,4,5,6};


	GTEST_COUT_qDB << "Attaching tap and get()";

//    async_results_from_get =
M_WARNING("TODO: This is still spinning when the test exits.");
	auto f2 = ef.tap(qApp, [=, &async_results_from_tap](FutureType ef, int begin, int end) mutable {
			GTEST_COUT_qDB << "IN TAP, begin:" << begin << ", end:" << end;
		for(int i = begin; i<end; i++)
		{
			GTEST_COUT_qDB << "Pushing" << ef.resultAt(i) << "to tap list.";
			async_results_from_tap.push_back(ef.resultAt(i));
			num_tap_completions++;
		}
	});

	GTEST_COUT_qDB << "BEFORE WAITING FOR GET()" << f2;

	f2.wait();

	GTEST_COUT_qDB << "AFTER WAITING FOR GET()" << f2;
	async_results_from_get = ef.results();

	EXPECT_TRUE(ef.isFinished());
	EXPECT_EQ(num_tap_completions, 6);

	// .get() above should block.
	EXPECT_TRUE(ef.isFinished());

	// This shouldn't do anything, should already be finished.
	ef.waitForFinished();

	GTEST_COUT_qDB << "Post .tap().get(), extfuture:" << ExtFutureState::state(ef);

	EXPECT_TRUE(ef.isStarted());
	EXPECT_FALSE(ef.isCanceled());
	EXPECT_TRUE(ef.isFinished());

	EXPECT_EQ(async_results_from_get.size(), 6);
	EXPECT_EQ(async_results_from_get, expected_results);
	EXPECT_EQ(async_results_from_tap.size(), 6);
	EXPECT_EQ(async_results_from_tap, expected_results);

	EXPECT_TRUE(ef.isFinished());
}

/**
 * Test1 "streaming" tap().
 * @todo Currently crashes.
 */
TEST_F(ExtFutureTest, ExtFutureStreamingTap1)
{
	TC_ENTER();

	streaming_tap_test<ExtFuture<int>>(1, 6, this);

	TC_EXIT();
}

class TestReportingRunner : public ControllableTask<int>
{
public:
	~TestReportingRunner() override = default;

	void run(QFutureInterface<int>& fi) override
	{
		AMLMTEST_SCOPED_TRACE("In TestReportingRunner::run()");

		QFutureInterface<int>& future_iface = fi;
		AMLMTEST_COUT << "Reporting started, was:" << ExtFutureState::state(future_iface);
//		future_iface.reportStarted();
//		AMLMTEST_COUT << "Reported started, is:" << ExtFutureState::state(future_iface);
		AMLMTEST_COUT << "Waiting...";
		QTest::qSleep(1000);
		AMLMTEST_COUT << "Waiting done, sending value 1 to future...";
		future_iface.reportResult(8675309);
		AMLMTEST_COUT << "Waiting...";
		QTest::qSleep(1000);
		AMLMTEST_COUT << "Waiting done, sending value 2 to future...";
		future_iface.reportResult(8675310);
		AMLMTEST_COUT << "run() finished, returning.";
//		future_iface.reportFinished();
//		AMLMTEST_COUT << "Reported finished, is:" << ExtFutureState::state(future_iface);
	}
};

TEST_F(ExtFutureTest, ReportingRunnerBasicStreamingTap)
{
	TC_ENTER();

	QFuture<int> the_future = make_default_future<QFuture<int>, int>();
//	QFuture<int> tap_finished_future = make_default_future<QFuture<int>, int>();
	QFutureWatcher<int> the_watcher(qApp);
//	QFutureSynchronizer tap_future_sync(tap_finished_future);
//	QFutureWatcher<int> the_tap_finished_watcher(qApp);

	AMLMTEST_EXPECT_FALSE(the_future.isCanceled());
//	AMLMTEST_EXPECT_FALSE(tap_finished_future.isCanceled());

	TestReportingRunner* async_task = new TestReportingRunner();

	AMLMTEST_COUT << "START running async_task.";
	the_future = ReportingRunner::run(async_task);
	AMLMTEST_COUT << "STARTED running async_task, the_future state:" << ExtFutureState::state(the_future);

	/// Tap control/results watcher.
	connect_or_die(&the_watcher, &QFutureWatcher<int>::resultsReadyAt, qApp, [=, &the_watcher](int begin, int end) mutable {
		// The tap.
		qDb() << "RECEIVED Tap Results:" << begin << end;
		for(auto i = begin; i < end; ++i)
		{
			qDb() << "Result" << i << ":" << the_watcher.future().resultAt(i);
		}
		;});
	/// Tap finished watcher.
	connect_or_die(&the_watcher, &QFutureWatcher<int>::finished, qApp, [=, &the_watcher/*, &the_tap_finished_watcher*/]() mutable {
		qDb() << "tap reported finished";
//		tap_finished_future = the_watcher.future();
//		tap_finished_future.d.reportFinished();
M_WARNING("First reports Started|Finished, second reports Running|Started");
//		qDb() << "Post-tap reported finished:" << ExtFutureState::state(tap_finished_future);
//		qDb() << "Post-tap reported finished:" << ExtFutureState::state(tap_finished_future.d);
//		QTest::qSleep(1000);
//		qDb() << "Post-tap reported finished:" << ExtFutureState::state(the_tap_finished_watcher.future());
//		run_in_event_loop(qApp, [=](){ tap_finished_future.d.reportFinished(); return 2; });
		;});
	the_watcher.setFuture(the_future);

//	connect_or_die(&the_tap_finished_watcher, &QFutureWatcher<int>::finished, qApp, [=](){
//		qDb() << "tap finished watcher finished";
//		;});
//	the_tap_finished_watcher.setFuture(tap_finished_future);

	AMLMTEST_COUT << "STARTING WAIT ON the_future results" << ExtFutureState::state(the_future); // << ExtFutureState::state(tap_finished_future);
	QTest::qWait(1000);
	QList<int> the_results = the_future.results();
	AMLMTEST_COUT << "WAIT ON the_future DONE" << ExtFutureState::state(the_future); // << ExtFutureState::state(tap_finished_future);

	AMLMTEST_ASSERT_EQ(the_results.at(0), 8675309);
	AMLMTEST_ASSERT_EQ(the_results.at(1), 8675310);

//	{
//		AMLMTEST_COUT << "STARTING WAIT ON tap_finished_future:" << ExtFutureState::state(tap_finished_future);
//		QTest::qWait(1000);

//		the_tap_finished_watcher.waitForFinished();
//		QEventLoop loop(qApp);
//		connect_or_die(&the_tap_finished_watcher, &QFutureWatcherBase::finished, &loop, &QEventLoop::quit);
//		AMLMTEST_ASSERT_FALSE(tap_future_sync.cancelOnWait());
//		tap_future_sync.waitForFinished();
//		bool didnt_time_out = QTest::qWaitFor([&](){return tap_finished_future.isFinished();}, 5000);
//		loop.exec();
//		AMLMTEST_COUT << "WAIT ON tap_finished_future DONE" << ExtFutureState::state(tap_finished_future);
//		AMLMTEST_ASSERT_TRUE(didnt_time_out);
//	}


//	the_future.waitForFinished();

	AMLMTEST_ASSERT_TRUE(the_future.isFinished());
//	AMLMTEST_ASSERT_TRUE(tap_finished_future.isFinished());

	AMLMTEST_COUT << "EXITING";

	TC_EXIT();
}

TEST_F(ExtFutureTest, QFutureBasicStreamingTap)
{
	TC_ENTER();

	QFuture<int> the_future = make_default_future<QFuture<int>, int>();
	QFuture<int> tap_finished_future = make_default_future<QFuture<int>, int>();
	QFutureWatcher<int> the_watcher(qApp);
//	QFutureSynchronizer tap_future_sync(tap_finished_future);
	QFutureWatcher<int> the_tap_finished_watcher(qApp);

	AMLMTEST_EXPECT_FALSE(the_future.isCanceled());
	AMLMTEST_EXPECT_FALSE(tap_finished_future.isCanceled());

	auto dontcare_f2 = QtConcurrent::run([=](/*std::reference_wrapper<QFutureInterface<int>>*/QFutureInterface<int>* future_iface_ref) mutable {
		QFutureInterface<int>& future_iface = *future_iface_ref;
		AMLMTEST_COUT << "Reporting started, was:" << ExtFutureState::state(future_iface);
		future_iface.reportStarted();
		AMLMTEST_COUT << "Reported started, is:" << ExtFutureState::state(future_iface);
		AMLMTEST_COUT << "Waiting...";
		TC_Sleep(1000);
		AMLMTEST_COUT << "Waiting done, sending value 1 to future...";
		future_iface.reportResult(8675309);
		AMLMTEST_COUT << "Waiting...";
		TC_Sleep(1000);
		AMLMTEST_COUT << "Waiting done, sending value 2 to future...";
		future_iface.reportResult(8675310);
		AMLMTEST_COUT << "Reporting finished";
		future_iface.reportFinished();
		AMLMTEST_COUT << "Reported finished, is:" << ExtFutureState::state(future_iface);
		;}, &(the_future.d));//std::ref(the_future.d));

	/// Tap
	connect_or_die(&the_watcher, &QFutureWatcher<int>::resultsReadyAt, qApp, [=, &the_watcher](int begin, int end) mutable {
		// The tap.
		qDb() << "Tap Results:" << begin << end;
		for(auto i = begin; i < end; ++i)
		{
			qDb() << "Result" << i << ":" << the_watcher.future().resultAt(i);
		}
		;});
	connect_or_die(&the_watcher, &QFutureWatcher<int>::finished, qApp, [=, &the_watcher, &the_tap_finished_watcher]() mutable {
		qDb() << "tap reported finished";
		tap_finished_future = the_watcher.future();
		tap_finished_future.d.reportFinished();
M_WARNING("First reports Started|Finished, second reports Running|Started");
		qDb() << "Post-tap reported finished:" << ExtFutureState::state(tap_finished_future);
		qDb() << "Post-tap reported finished:" << ExtFutureState::state(tap_finished_future.d);
		QTest::qSleep(1000);
		qDb() << "Post-tap reported finished:" << ExtFutureState::state(the_tap_finished_watcher.future());
//		run_in_event_loop(qApp, [=](){ tap_finished_future.d.reportFinished(); return 2; });
		;});
	the_watcher.setFuture(the_future);

	connect_or_die(&the_tap_finished_watcher, &QFutureWatcher<int>::finished, qApp, [=](){
		qDb() << "tap finished watcher finished";
		;});
	the_tap_finished_watcher.setFuture(tap_finished_future);

	AMLMTEST_COUT << "STARTING WAIT ON the_future" << ExtFutureState::state(the_future) << ExtFutureState::state(tap_finished_future);
//	QTest::qWait(1000);
	int the_result = the_watcher.result();
	AMLMTEST_COUT << "WAIT ON the_future DONE" << ExtFutureState::state(the_future) << ExtFutureState::state(tap_finished_future);

	AMLMTEST_ASSERT_EQ(the_result, 8675309);

//	{
		AMLMTEST_COUT << "STARTING WAIT ON tap_finished_future:" << ExtFutureState::state(tap_finished_future);
//		QTest::qWait(1000);

//		the_tap_finished_watcher.waitForFinished();
		QEventLoop loop(qApp);
		connect_or_die(&the_tap_finished_watcher, &QFutureWatcherBase::finished, &loop, &QEventLoop::quit);
//		AMLMTEST_ASSERT_FALSE(tap_future_sync.cancelOnWait());
//		tap_future_sync.waitForFinished();
		bool didnt_time_out = QTest::qWaitFor([&](){return tap_finished_future.isFinished();}, 5000);
		loop.exec();
		AMLMTEST_COUT << "WAIT ON tap_finished_future DONE" << ExtFutureState::state(tap_finished_future);
		AMLMTEST_ASSERT_TRUE(didnt_time_out);
//	}


//	the_future.waitForFinished();

	AMLMTEST_ASSERT_TRUE(the_future.isFinished());
	AMLMTEST_ASSERT_TRUE(tap_finished_future.isFinished());

	TC_EXIT();
}

/**
 * Test "streaming" tap().
 * @todo Currently crashes.
 */
TEST_F(ExtFutureTest, ExtFutureStreamingTap)
{
    TC_ENTER();

	using eftype = ExtFuture<int>;

    static std::atomic_int num_tap_completions {0};
	QList<int> async_results_from_tap, async_results_from_get;


    QList<int> expected_results {1,2,3,4,5,6};
    eftype ef = async_int_generator<eftype>(1, 6, this);

    GTEST_COUT_qDB << "Starting ef state:" << ef.state();
    ASSERT_TRUE(ef.isStarted());
    ASSERT_FALSE(ef.isCanceled());
    ASSERT_FALSE(ef.isFinished());

//	ExtFutureWatcher<int> efw(nullptr, nullptr);
//	efw.then([=](){
//		qDb() << "GOT EFW THEN() CALLBACK / future finished:" << ef.state();
//	});
//	efw.setFuture(ef);


    GTEST_COUT_qDB << "Attaching tap and get()";

//    async_results_from_get =
M_WARNING("TODO: This is still spinning when the test exits.");
	auto f2 = ef.tap(qApp, [=, &async_results_from_tap](eftype ef, int begin, int end) mutable {
            GTEST_COUT_qDB << "IN TAP, begin:" << begin << ", end:" << end;
        for(int i = begin; i<end; i++)
        {
            GTEST_COUT_qDB << "Pushing" << ef.resultAt(i) << "to tap list.";
            async_results_from_tap.push_back(ef.resultAt(i));
            num_tap_completions++;
        }
	});

//	ExtFutureWatcher<int> f2w(nullptr, nullptr);
//	f2w.then([=](){
//		qDb() << "GOT F2W THEN() CALLBACK / future finished:" << f2.state();
//	});
//	f2w.setFuture(f2);


	GTEST_COUT_qDB << "BEFORE WAITING FOR GET()" << f2;

	f2.wait();

	GTEST_COUT_qDB << "AFTER WAITING FOR GET()" << f2;
    async_results_from_get = ef.results();

    EXPECT_TRUE(ef.isFinished());
    EXPECT_EQ(num_tap_completions, 6);

    // .get() above should block.
    EXPECT_TRUE(ef.isFinished());

    // This shouldn't do anything, should already be finished.
    ef.waitForFinished();

    GTEST_COUT_qDB << "Post .tap().get(), extfuture:" << ef.state();

    EXPECT_TRUE(ef.isStarted());
    EXPECT_FALSE(ef.isCanceled());
    EXPECT_TRUE(ef.isFinished());

    EXPECT_EQ(async_results_from_get.size(), 6);
    EXPECT_EQ(async_results_from_get, expected_results);
    EXPECT_EQ(async_results_from_tap.size(), 6);
    EXPECT_EQ(async_results_from_tap, expected_results);

    ASSERT_TRUE(ef.isFinished());

    TC_EXIT();
}


/// Static checks
TEST_F(ExtFutureTest, StaticAsserts)
{

	static_assert(std::is_default_constructible<QString>::value);

    // From http://en.cppreference.com/w/cpp/experimental/make_ready_future:
    // "If std::decay_t<T> is std::reference_wrapper<X>, then the type V is X&, otherwise, V is std::decay_t<T>."
	static_assert(std::is_same_v<decltype(make_ready_future(4)), ExtFuture<int> >);
    int v;
	static_assert(std::is_same_v<decltype(make_ready_future(std::ref(v))), ExtFuture<int&> >);
    /// @todo
//    static_assert(std::is_same_v<decltype(make_ready_future()), ExtFuture<Unit> >, "");

}
