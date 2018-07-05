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

#include "AsyncTests.h"

#include <type_traits>
#include <atomic>

#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>
#include <tests/TestHelpers.h>

#include <QString>
#include <QTest>

#include "../future_type_traits.hpp"
#include "../function_traits.hpp"


#include "../ExtAsync.h"


void AsyncTestsSuiteFixture::SetUp()
{
	GTEST_COUT << "SetUp()" << std::endl;
}

void AsyncTestsSuiteFixture::TearDown()
{
	GTEST_COUT << "TearDown()" << std::endl;
}



///
/// Test Cases
///

#if 0
TEST_F(AsyncTestsSuiteFixture, ThisShouldFail)
{
	ASSERT_TRUE(false);
}
#endif

TEST_F(AsyncTestsSuiteFixture, ThisShouldPass)
{
	TC_ENTER();

	TC_EXPECT_NOT_EXIT();

	ASSERT_FALSE(has_finished(__PRETTY_FUNCTION__));
	ASSERT_TRUE(true);
	finished(__PRETTY_FUNCTION__);
	ASSERT_TRUE(has_finished(__PRETTY_FUNCTION__));

	TC_DONE_WITH_STACK();

	TC_EXIT();
}

TEST_F(AsyncTestsSuiteFixture, QStringPrintTest)
{
	QString test = "Test";
	ASSERT_EQ(test, "Test");
	finished(__PRETTY_FUNCTION__);
}

/**
 * From a lambda passed to QtConcurrent::run(), sleeps for 1 sec and then returns a single QString.
 * @return
 */
static QString delayed_string_func_1()
{
	auto retval = QtConcurrent::run([](){
		// Sleep for a second.
		qDb() << "ENTER, SLEEPING FOR 1 SEC";
//		QThread::sleep(1);
		QTest::qWait(1000);
		qDb() << "SLEEP COMPLETE";
		return QString("delayed_string_func_1() output");
	});

	EXPECT_TRUE(retval.isStarted());
	EXPECT_FALSE(retval.isFinished());

	GTEST_COUT << "delayed_string_func_1() returning" << tostdstr(retval) << std::endl;

	return retval.result();
}

/**
 * From a lambda passed to QtConcurrent::run(), sleeps for 1 sec and then returns a single QString.
 * @return
 */
static ExtFuture<QString> delayed_string_func()
{
    QFuture<QString> retval = QtConcurrent::run([](){
		// Sleep for a second.
		qDb() << "ENTER, SLEEPING FOR 1 SEC";
		QThread::sleep(1);
		qDb() << "SLEEP COMPLETE, returning HELLO";
		return QString("HELLO");
	});

	static_assert(std::is_same_v<decltype(retval), QFuture<QString>>, "");

	EXPECT_TRUE(retval.isStarted());
	EXPECT_FALSE(retval.isFinished());

	GTEST_COUT << "delayed_string_func() returning" << tostdstr(retval) << std::endl;

	return retval;
}

/**
 * From a lambda passed to ExtAsync::run(), iterates @a num_iteration times,
 * sleep()ing for 1 sec, then returns the the next value in the sequence to the returned ExtFuture<>.
 *
 * @todo Doesn't handle cancellation or progress reporting.
 */
static ExtFuture<int> async_int_generator(int start_val, int num_iterations)
{
	/** @todo
	 * This run is:
	[ "" ] EXTASYNC::RUN: IN auto run(F&& function, Args&&... args): auto ExtAsync::run(F &&, Args &&...) [F = (lambda at ../utils/concurrency/tests/AsyncTests.cpp:165:40), Args = <>]
	[ "" ] EXTASYNC::RUN: IN ExtFutureR run_helper_struct::run(F&& function, Args&&... args): ExtFutureR ExtAsync::detail::run_helper_struct<ExtFuture<int> >::run(F &&, Args &&...) [ExtFutureR = ExtFuture<int>, F = (lambda at ../utils/concurrency/tests/AsyncTests.cpp:165:40), Args = <>]
	*/
	ExtFuture<int> retval = ExtAsync::run([=](ExtFuture<int>& future) {
		int current_val = start_val;
		for(int i=0; i<num_iterations; i++)
		{
			// Sleep for a second.
			qWr() << "SLEEPING FOR 1 SEC";

			QThread::sleep(1);
			qWr() << "SLEEP COMPLETE, sending value to future:" << current_val;

			future.reportResult(current_val);
			current_val++;
		}
		// We're done.
		qWr() << "REPORTING FINISHED";
		future.reportFinished();
	});

	static_assert(std::is_same_v<decltype(retval), ExtFuture<int>>, "");

	qWr() << "RETURNING:" << retval;

	return retval;
}

//
// TESTS
//

TEST_F(AsyncTestsSuiteFixture, QtConcurrentSanityTest)
{
    int counter = 0;

    QFuture<int> f = QtConcurrent::run([&]() mutable -> int {
        GTEST_COUT << "Entered callback\n";
        sleep(1);
        counter = 1;
        GTEST_COUT << "T+1 secs\n";
        sleep(1);
        counter = 2;
        return 5;
        ;});

    EXPECT_TRUE(f.isStarted());
    QTest::qSleep(500);
    EXPECT_TRUE(f.isRunning()); // In sleep(1)
    QTest::qSleep(1000);
    GTEST_COUT << "CHECKING COUNTER FOR 1\n";
    EXPECT_EQ(counter, 1);

    GTEST_COUT << "CANCELING\n";
    f.cancel();
    EXPECT_TRUE(f.isCanceled());
    f.waitForFinished();
    EXPECT_TRUE(f.isCanceled());
    EXPECT_TRUE(f.isFinished());

    f.waitForFinished();
    EXPECT_TRUE(f.isStarted());
    EXPECT_TRUE(f.isFinished());
}

TEST_F(AsyncTestsSuiteFixture, ExtFuture_copy_assign_tests)
{
    SCOPED_TRACE("START");
    TC_ENTER();

    // default constructors
    ExtFuture<int> intFuture;
    intFuture.waitForFinished();
    ExtFuture<QString> stringFuture;
    stringFuture.waitForFinished();
    ExtFuture<Unit> unitFuture;
    unitFuture.waitForFinished();
    ExtFuture<Unit> defaultUnitFuture;
    defaultUnitFuture.waitForFinished();

    // copy constructor
    ExtFuture<int> intFuture2(intFuture);
    ExtFuture<Unit> UnitFuture2(defaultUnitFuture);

    // assigmnent operator
    intFuture2 = ExtFuture<int>();
    UnitFuture2 = ExtFuture<Unit>();

    // state
    ASSERT_EQ(intFuture2.isStarted(), true);
    /// @note This is a difference between QFuture<> and ExtFuture<>, there's no reason this future should be finished here.
//    ASSERT_EQ(intFuture2.isFinished(), true);

    TC_DONE_WITH_STACK();
    TC_EXIT();
}

TEST_F(AsyncTestsSuiteFixture, ExtFutureThenChainingTest_ExtFutures)
{
	SCOPED_TRACE("START");
//	qIn() << "START";

	std::atomic_bool ran1 {false};
	std::atomic_bool ran2 {false};
	std::atomic_bool ran3 {false};

	TC_ENTER();

	ExtFuture<QString> future = ExtAsync::run(delayed_string_func_1);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	future
	.then([&](ExtFuture<QString> extfuture) -> QString {

		TC_EXPECT_NOT_EXIT();

		// Check if .get() would block.  In the continuation, it shouldn't, since it shouldn't
		// run until extfuture is finished.
		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then1, got extfuture:" << extfuture;
		qDb() << "Then1, extfuture val:" << extfuture.get();
		EXPECT_EQ(ran1, false);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran1 = true;
		QString the_str = extfuture.get();
		EXPECT_EQ(the_str, QString("delayed_string_func_1() output"));
		return QString("Then1 OUTPUT");
	})
	.then([&](ExtFuture<QString> extfuture) -> QString {

		TC_EXPECT_NOT_EXIT();

		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then2, got extfuture:" << extfuture;
		qDb() << "Then2, extfuture val:" << extfuture.get();
		EXPECT_EQ(ran1, true);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran2 = true;
		auto the_str = extfuture.get();
		EXPECT_EQ(the_str, QString("Then1 OUTPUT"));
		return QString("Then2 OUTPUT");
	})
	.then([&](ExtFuture<QString> extfuture) -> QString {

		TC_EXPECT_NOT_EXIT();

		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then3, got extfuture:" << extfuture;
		qDb() << "Then3, extfuture val:" << extfuture.get();
		EXPECT_EQ(ran1, true);
		EXPECT_EQ(ran2, true);
		EXPECT_EQ(ran3, false);
		ran3 = true;
		TC_DONE_WITH_STACK();
		return QString("Then3 OUTPUT");
    }).wait();

    ASSERT_TRUE(future.isFinished());

	qDb() << "STARING WAIT";
	/// @todo This doesn't wait here, but the attached wait() above does. Which maybe makes sense.
	future.wait();
	qDb() << "ENDING WAIT";

//    qDb() << "STARTING waitForFinished()";
//    future.future().waitForFinished();
//    qDb() << "ENDING waitForFinished()";

    ASSERT_TRUE(future.isFinished());

	ASSERT_TRUE(ran1);
	ASSERT_TRUE(ran2);
	ASSERT_TRUE(ran3);

    GTEST_COUT << __PRETTY_FUNCTION__ << "returning" << future << std::endl;

	TC_EXIT();
}

TEST_F(AsyncTestsSuiteFixture, ExtFutureThenChainingTest_MixedTypes)
{
//	qIn() << "START";

	bool ran1 = false;
	bool ran2 = false;
	bool ran3 = false;

	TC_ENTER();

	ExtFuture<QString> future = ExtAsync::run(delayed_string_func_1);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	future
	.then([&](ExtFuture<QString> extfuture) -> int {

		TC_EXPECT_NOT_EXIT();

		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then1, got val:" << extfuture.get();
		EXPECT_EQ(ran1, false);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran1 = true;
		QString the_str = extfuture.get();
		EXPECT_EQ(the_str, QString("delayed_string_func_1() output"));
		return 2;
	})
	.then([&](ExtFuture<int> extfuture) -> int {

		TC_EXPECT_NOT_EXIT();

		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then2, got val:" << extfuture.get();
		EXPECT_EQ(ran1, true);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran2 = true;
		EXPECT_EQ(extfuture.get(), 2);
		return 3;
	})
	.then([&](ExtFuture<int> extfuture) -> double {
		TC_EXPECT_NOT_EXIT();

		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then3, got val:" << extfuture.get();
		EXPECT_EQ(ran1, true);
		EXPECT_EQ(ran2, true);
		EXPECT_EQ(ran3, false);
		ran3 = true;
		EXPECT_EQ(extfuture.get(), 3);

		TC_DONE_WITH_STACK();

		return 3.1415;
	}).wait();

	ASSERT_TRUE(future.isFinished());

	qDb() << "STARING WAIT";
	/// @todo This doesn't wait here, but the attached wait() above does. Which maybe makes sense.
	future.wait();
	qDb() << "ENDING WAIT";

	ASSERT_TRUE(ran1);
	ASSERT_TRUE(ran2);
	ASSERT_TRUE(ran3);

    GTEST_COUT << __PRETTY_FUNCTION__ << "returning" << future << std::endl;

	TC_EXIT();
}

TEST_F(AsyncTestsSuiteFixture, ExtFuture_ExtAsyncRun_multi_result_test)
{
	std::atomic_int start_val {5};
	std::atomic_int num_iterations {3};
	std::atomic_bool tap_complete {false};
	TC_ENTER();

	// Start generating a sequence of results.
	auto future = async_int_generator(start_val, num_iterations);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	// Separated .then() connect.
	future.tap([&](int future_value) {

		TC_EXPECT_NOT_EXIT();
		TC_EXPECT_STACK();

		static int last_seen_result = 0;
		static int num_tap_calls = 0;

		GTEST_COUT << "testname: " << testname << std::endl;
		GTEST_COUT << "num_tap_calls:" << num_tap_calls << std::endl;

		EXPECT_EQ(start_val, 5);
		EXPECT_EQ(num_iterations, 3);

		if(num_tap_calls == 0)
		{
			EXPECT_EQ(start_val, 5);
			EXPECT_EQ(last_seen_result, 0);
		}

		int expected_future_val = start_val + num_tap_calls;
		GTEST_COUT << "expected_future_val: " << expected_future_val << std::endl;
		EXPECT_EQ(expected_future_val, future_value) << "FAIL in ExtFuture_ExtAsyncRun_multi_result_test()";
		last_seen_result = future_value;
		num_tap_calls++;
		EXPECT_LE(num_tap_calls, num_iterations);
		if(num_tap_calls == num_iterations)
		{
//			TC_DONE_WITH_STACK();
			tap_complete = true;
		}
		;}).then([&](ExtFuture<int> extfuture) -> int {
			EXPECT_EQ(tap_complete, true);

			EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
			EXPECT_FALSE(extfuture.isRunning());

			qWr() << "IN THEN:" << extfuture;
			return 1;
		;})
//		.wait();
//M_WARNING("THE ABOVE .wait() doesn't wait");
#if 1
		.finally([&]() {

			TC_EXPECT_NOT_EXIT();

			EXPECT_EQ(tap_complete, true);

			TC_DONE_WITH_STACK();
		;}).wait();
#endif

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isRunning());
	ASSERT_TRUE(future.isFinished());

	TC_EXIT();
}


TEST_F(AsyncTestsSuiteFixture, TestReadyFutures)
{
	TC_ENTER();

	ExtFuture<int> future = make_ready_future(45);
	ASSERT_TRUE(future.isStarted());
	ASSERT_TRUE(future.isFinished());
	ASSERT_EQ(future.get(), 45);

	TC_DONE_WITH_STACK();

	TC_EXIT();
}



//TEST_F(AsyncTestsSuiteFixture, DISABLED_UnwrapTest)
//{

////	auto future = QtConcurrent::run(delayed_string_func);
////	ExtFuture<ExtFuture<QString>> future = ExtAsync::run(delayed_string_func);
////	ExtFuture<QString> future = ExtAsync::run([&](ExtFuture<QString> future) {
////		qDb() << "TEST: Running from main run lambda.";
////		// Sleep for a second to make sure then() doesn't run before we get to the Q_ASSERT() after this chain.
////		QThread::sleep(1);
////		future.reportResult("Hello1");
////		future.reportResult("Hello2");
////		future.reportFinished(new QString("FINISHED"));
////		qDb() << "TEST: Finished from main run lambda.";
////		return ExtFuture<QString>(); //("FINISHED");
////	});

////	ExtFuture<QString> unwrapped_future = future.unwrap();
//}

TEST_F(AsyncTestsSuiteFixture, TapAndThen_OneResult)
{

	static std::atomic_bool ran_tap {false};
	static std::atomic_bool ran_then {false};
	TC_ENTER();

	ExtFuture<QString> future = ExtAsync::run(delayed_string_func_1);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	GTEST_COUT << "Future created" << std::endl;

	future.tap([&](QString result){

			TC_EXPECT_NOT_EXIT();
			TC_EXPECT_STACK();

			GTEST_COUT << "testname: " << testname << std::endl;

			GTEST_COUT << "in tap(), result:" << tostdstr(result) << std::endl;
			EXPECT_EQ(result, QString("delayed_string_func_1() output"));
			ran_tap = true;
			EXPECT_FALSE(ran_then);
		;})
		.then([&](ExtFuture<QString> extfuture) {

			TC_EXPECT_NOT_EXIT();
			TC_EXPECT_STACK();

			EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
			EXPECT_FALSE(extfuture.isRunning());

			GTEST_COUT << "in then(), extfuture:" << tostdstr(extfuture.get()) << std::endl;
			EXPECT_EQ(extfuture.get(), QString("delayed_string_func_1() output"));
			EXPECT_TRUE(ran_tap);
			EXPECT_FALSE(ran_then);
			ran_then = true;
			TC_DONE_WITH_STACK();
			return QString("Then Called");
		;}).wait();

    GTEST_COUT << "after wait()" << future << std::endl;

//	future.wait();

	ASSERT_TRUE(ran_tap);
	ASSERT_TRUE(ran_then);

	TC_EXIT();
}

TEST_F(AsyncTestsSuiteFixture, TapAndThen_MultipleResults)
{
	std::atomic_int tap_call_counter {0};
	TC_ENTER();

	ExtFuture<int> future = ExtAsync::run([&](ExtFuture<int>& extfuture) {

			TC_EXPECT_NOT_EXIT();
			TC_EXPECT_STACK();

			GTEST_COUT << "TEST: Running from main run lambda." << std::endl;
			// Sleep for a second to make sure then() doesn't run before we get to the Q_ASSERT() after this chain.
			GTEST_COUT << "SLEEP 1" << std::endl;
			//QThread::sleep(1);
			QTest::qWait(1000);
			extfuture.reportResult(867);
			GTEST_COUT << "SLEEP 1" << std::endl;
			//QThread::sleep(1);
			QTest::qWait(1000);
			extfuture.reportResult(5309);
			GTEST_COUT << "SLEEP 1" << std::endl;
			//QThread::sleep(1);
			QTest::qWait(1000);
			GTEST_COUT << "FINISHED" << std::endl;
			extfuture.reportFinished();
			GTEST_COUT << "TEST: Finished from main run lambda." << std::endl;
		})
	.tap([&](int value){

		TC_EXPECT_NOT_EXIT();
		TC_EXPECT_STACK();

		/// Get a local copy of the counter value atomically.
		int current_tap_call_count = tap_call_counter;
		if(current_tap_call_count == 0)
		{
			EXPECT_EQ(value, 867);
		}
		else if(current_tap_call_count == 1)
		{
			EXPECT_EQ(value, 5309);
			TC_DONE_WITH_STACK();
		}
		else
		{
			EXPECT_EQ(current_tap_call_count, 1);
		}
		current_tap_call_count++;
		// Asign the new value back atomically.
		tap_call_counter = current_tap_call_count;
		;});

	// No wait, shouldn't have finished yet.
	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	ASSERT_FALSE(future.isFinished());
	future.wait();
	ASSERT_TRUE(future.isFinished());

	ASSERT_EQ(tap_call_counter, 2);

	TC_EXPECT_NOT_EXIT();

	TC_EXIT();
}

/**
 * Test basic cancel properties.
 */
TEST_F(AsyncTestsSuiteFixture, ExtFuture_BasicCancel)
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

    // Cancelling alone won't finish the extfuture.
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
TEST_F(AsyncTestsSuiteFixture, ExtFuture_CancelPromise)
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
TEST_F(AsyncTestsSuiteFixture, ExtFuture_CancelFuture)
{
    TC_ENTER();

    ExtFuture<Unit> result;
    ExtFuture<Unit> f;

    ASSERT_TRUE(f.isStarted());

    result.reportStarted();
    f = result.future();

    ASSERT_TRUE(f.isStarted());

    ASSERT_FALSE(result.isCanceled());
    f.cancel();

    ASSERT_TRUE(result.isCanceled());

    result.reportFinished();

    TC_DONE_WITH_STACK();
    TC_EXIT();
}


///// ExtAsync<>::run() tests.

TEST_F(AsyncTestsSuiteFixture, ExtAsync_run_freefunc)
{
    TC_ENTER();

    ExtFuture<int> extfuture = ExtAsync::run([=](){ return 4;});

    int retval = extfuture.get();

    ASSERT_EQ(retval, 4);

    TC_DONE_WITH_STACK();
    TC_EXIT();
}

/// Static checks
void dummy(void)
{

	static_assert(std::is_default_constructible<QString>::value, "");

	// From http://en.cppreference.com/w/cpp/experimental/make_ready_future:
	// "If std::decay_t<T> is std::reference_wrapper<X>, then the type V is X&, otherwise, V is std::decay_t<T>."
	static_assert(std::is_same_v<decltype(make_ready_future(4)), ExtFuture<int> >, "");
	int v;
	static_assert(std::is_same_v<decltype(make_ready_future(std::ref(v))), ExtFuture<int&> >, "");
	/// @todo
//	static_assert(std::is_same_v<decltype(make_ready_future()), ExtFuture<void> >, "");

}

