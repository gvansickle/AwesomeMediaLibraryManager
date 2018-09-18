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

#include "ExtAsyncTests.h"


// Std C++
#include <type_traits>
#include <atomic>
#include <functional>

// Future Std C++
#include <future/function_traits.hpp>
#include <future/future_type_traits.hpp>

// Qt5
#include <QString>
#include <QTest>
#include <QFutureInterfaceBase> // shhh, we're not supposed to use this.  For calling .reportFinished() on QFuture<>s inside a run().

// Google Test
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Ours
#include <tests/TestHelpers.h>
#include "ExtAsyncTestCommon.h"

// Mocks
#include <tests/TestLifecycleManager.h>


// Classes Under Test.
#include "../ExtAsync.h"



///
/// Test Cases
///

#if 0
TEST_F(ExtAsyncTestsSuiteFixture, ThisShouldFail)
{
	ASSERT_TRUE(false);
}
#endif

TEST_F(ExtAsyncTestsSuiteFixture, ThisShouldPass)
{
	TC_ENTER();

	TC_EXPECT_NOT_EXIT();

	ASSERT_TRUE(true);

	TC_DONE_WITH_STACK();

	TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, QStringPrintTest)
{
	QString test = "Test";
	ASSERT_EQ(test, "Test");
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
        TC_Sleep(1000);
		qDb() << "SLEEP COMPLETE, returning HELLO";
		return QString("HELLO");
	});

	static_assert(std::is_same_v<decltype(retval), QFuture<QString>>, "");

	EXPECT_TRUE(retval.isStarted());
	EXPECT_FALSE(retval.isFinished());

	AMLMTEST_COUT << "delayed_string_func() returning" << tostdstr(retval);

	return retval;
}


//
// TESTS
//

TEST_F(ExtAsyncTestsSuiteFixture, QtConcurrentSanityTest)
{
    SCOPED_TRACE("A");
	TC_ENTER();

    std::atomic_int counter {0};

	AMLMTEST_COUT << "CALLING ::RUN";

    /// @note When Qt says the returned QFuture can't be canceled, they mean it.
	/// If you do, things get totally screwed up and this will segfault.
	QFuture<int> f = QtConcurrent::run([&]() -> int {
		AMLMTEST_COUT << "Entered callback";
        TC_Sleep(1000);
        counter = 1;
		AMLMTEST_COUT << "T+1 secs";
        TC_Sleep(1000);
        counter = 2;
        return 5;
        ;});

	EXPECT_TRUE(f.isStarted());
	/// @note QFuture<> f's state here is Running|Started.  EXPECT is here to see if Running is always the case,
	/// I don't think it necessarily is.
//	EXPECT_TRUE(f.isRunning()) << "QtConcurrent::run() doesn't always return a Running future.";
	AMLMTEST_COUT << "CALLED ::RUN, FUTURE STATE:" << ExtFutureState::state(f);

    EXPECT_TRUE(f.isStarted());
    TC_Sleep(500);
    EXPECT_TRUE(f.isRunning()); // In the first TC_Sleep(1000);
    TC_Sleep(1000);
	AMLMTEST_COUT << "CHECKING COUNTER FOR 1";//; // In the second TC_Sleep(1000);
    EXPECT_EQ(counter, 1);

	AMLMTEST_COUT << "WAITING FOR FINISHED";
    f.waitForFinished();
	/// @note This QFuture<>, which was returned by QtConcurrent::run(), is Started|Finished here.
	/// So, that's the natural state of a successfully finished QFuture<>.
	AMLMTEST_COUT << "QFUTURE FINISHED, state:" << ExtFutureState::state(f);
    EXPECT_FALSE(f.isCanceled());
    EXPECT_TRUE(f.isFinished());

    // Can't cancel a QtConcurrent::run() future, so the callback should have run to completion.
    EXPECT_EQ(counter, 2);
	AMLMTEST_COUT << "CHECKING QFUTURE RESULT";
    EXPECT_EQ(f.resultCount(), 1);
    int res = f.result();
    EXPECT_EQ(res, 5);
	AMLMTEST_COUT << "CHECKED QFUTURE RESULT";

    f.waitForFinished();
    EXPECT_TRUE(f.isStarted());
    EXPECT_TRUE(f.isFinished());
	AMLMTEST_COUT << "RETURNING";

	TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, QFutureSanityGetFinishedWhenAlreadyCanceled)
{
	TC_ENTER();

	std::atomic_int finished_counter {0};

	QFuture<int> f = make_finished_QFuture(5);

	AMLMTEST_EXPECT_TRUE(f.isStarted()) << state(f);
	AMLMTEST_EXPECT_TRUE(f.isFinished());
	AMLMTEST_EXPECT_FALSE(f.isCanceled());

	// Hook up a watcher and make sure we get the finished signal.
	QFutureWatcher<int> fw(qApp);

	connect_or_die(&fw, &QFutureWatcher<int>::finished, qApp, [&](){
		qDb() << "Got Finished signal";
		finished_counter++;
	});

	// Now we set the future, and we should immediately get the finsihed signal.
	fw.setFuture(f);

	// Spin the event loop for a bit to allow the signal to actually propagate.
	TC_Wait(1000);

	AMLMTEST_ASSERT_EQ(finished_counter, 1);

	// Block on the future.  Should be finished, so shouldn't block.
	f.waitForFinished();

	TC_EXIT();
}


template <typename FutureTypeT>
void QtConcurrentRunFutureStateOnCancelGuts()
{
    SCOPED_TRACE("QtConcurrentRunFutureStateOnCancelGuts");

    std::atomic_int counter {0};

    FutureTypeT the_future;
    if constexpr (std::is_same_v<FutureTypeT, QFuture<int>>)
    {
        the_future = make_startedNotCanceled_QFuture<int>();
    }

    ASSERT_TRUE(the_future.isStarted());
    ASSERT_FALSE(the_future.isCanceled());
    ASSERT_FALSE(the_future.isFinished());

	AMLMTEST_COUT << "CALLING QTC::run()";

    /**
     * Per docs:
     * "Note that function may not run immediately; function will only be run once a thread becomes available."
     * @link http://doc.qt.io/qt-5/qtconcurrent.html#mappedReduced-1
     */
	auto f = QtConcurrent::run([=,&counter](FutureTypeT the_passed_future)  {
		AMLMTEST_COUT << "Entered callback, passed future state:" << ExtFutureState::state(the_passed_future);

            EXPECT_TRUE(the_passed_future.isStarted());
            EXPECT_FALSE(the_passed_future.isCanceled());

            while(!the_passed_future.isCanceled())
            {
				AMLMTEST_COUT << "LOOP COUNTER: " << counter;

                // Pretend to do 1 second of work.
                TC_Sleep(1000);
                counter++;
				AMLMTEST_COUT << "+1 secs, counter = " << counter;
            }

            // Only way to exit the while loop above is by external cancellation.
            EXPECT_TRUE(the_passed_future.isCanceled());

            /// @note For both QFuture<> and ExtFuture<>, we need to finish the future ourselves in this case.
            ///       Not sure if the waitForFinished() should be here or rely on caller to do it.
			reportFinished(&the_passed_future);
//            the_passed_future.waitForFinished();

		 AMLMTEST_COUT << "Exiting callback, passed future state:" << ExtFutureState::state(the_passed_future);
		 ;}, the_future);

	AMLMTEST_COUT << "Passed the run() call, got the future:" << ExtFutureState::state(the_future);

    EXPECT_TRUE(the_future.isStarted());
    EXPECT_TRUE(f.isStarted());

	AMLMTEST_COUT << "WAITING FOR 5 SECS...";
    // qWait() doesn't block the event loop, qSleep() does.
    TC_Wait(5000);

	AMLMTEST_COUT << "CANCELING THE FUTURE:" << ExtFutureState::state(the_future);
    the_future.cancel();
	AMLMTEST_COUT << "CANCELED FUTURE STATE:" << ExtFutureState::state(the_future);

//    TC_Sleep(1000);

    // We don't really care about the future returned from QtConcurrent::run(), but it should be finished by now.
    /// @todo Sometimes f not finished here, problem?
//	f.waitForFinished();
//	EXPECT_TRUE(f.isFinished());// << ExtFutureState::state(ExtFuture<Unit>(f));

    EXPECT_TRUE(the_future.isStarted());
    EXPECT_TRUE(the_future.isCanceled());

    /// @todo This is never finished.
    the_future.waitForFinished();
//    the_future.result();

	AMLMTEST_COUT << "FUTURE IS FINISHED:" << ExtFutureState::state(the_future);

	AMLMTEST_ASSERT_TRUE(the_future.isStarted());
	AMLMTEST_ASSERT_TRUE(the_future.isCanceled());
	AMLMTEST_ASSERT_FALSE(the_future.isRunning());
	AMLMTEST_ASSERT_TRUE(the_future.isFinished());
}

TEST_F(ExtAsyncTestsSuiteFixture, QtConcurrentRunQFutureStateOnCancel)
{
	TC_ENTER();

	AMLMTEST_ASSERT_NO_FATAL_FAILURE({
		QtConcurrentRunFutureStateOnCancelGuts<QFuture<int>>();
									 });
	TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, QtConcurrentRunExtFutureStateOnCancel)
{
	TC_ENTER();

	AMLMTEST_ASSERT_NO_FATAL_FAILURE({
		QtConcurrentRunFutureStateOnCancelGuts<ExtFuture<int>>();
									 });

	TC_EXIT();
}

template <typename FutureTypeT>
void QtConcurrentMappedFutureStateOnCancel(bool dont_let_jobs_complete)
{
	AMLMTEST_SCOPED_TRACE("IN QtConcurrentMappedFutureStateOnCancel template function");
    std::atomic_int counter{0};

    QVector<int> dummy_vector{0,1,2,3,4,5,6,7,8,9};

	AMLMTEST_COUT << "CALLING QTC::mapped()";

    /**
     * Per docs:
     * "Note that function may not run immediately; function will only be run once a thread becomes available."
     * @link http://doc.qt.io/qt-5/qtconcurrent.html#mappedReduced-1
     *
     * Since a QFuture<> starts out canceled, we will get into the callback with the future Started | Canceled.
     */

    FutureTypeT f;
    if constexpr (std::is_same_v<FutureTypeT, QFuture<int>>)
    {
		f = make_startedNotCanceled_QFuture<int>();
    }

	std::function<int(const int&)> lambda = [&](const int& the_passed_value) -> int {
		AMLMTEST_COUT << "Entered callback, passed value:" << the_passed_value;

		AMLMTEST_COUT << "FUTURE:" << ExtFutureState::state(f);
        EXPECT_TRUE(f.isStarted());
        EXPECT_TRUE(f.isRunning());
        EXPECT_FALSE(f.isCanceled());
        EXPECT_FALSE(f.isFinished());

            if(!f.isCanceled())
            {
                if(dont_let_jobs_complete)
                {
                    // None of the jobs should complete before the future is canceled.
					AMLMTEST_COUT << "SLEEPING FOR 2 secs";
                    TC_Sleep(2000);
                }
                else
                {
                    // Let them all complete.
					AMLMTEST_COUT << "SLEEPING FOR 0.5 secs";
                    TC_Sleep(500);
                }

                if(f.isCanceled())
                {
					AMLMTEST_COUT << "CANCELLED";
                    return 0;
                }

				AMLMTEST_COUT << "PROCESSING VALUE: " << the_passed_value;

                counter++;
				AMLMTEST_COUT << "+1 secs, counter = " << counter;
                if(counter == 10)
                {
					AMLMTEST_COUT << "FAIL counter = " << counter;
                }
            }
		 AMLMTEST_COUT << "Exiting callback";

         return the_passed_value + 1;
         };

	AMLMTEST_COUT << "Calling QtConcurrent::mapped()";
    f = QtConcurrent::mapped(dummy_vector, lambda);

	AMLMTEST_COUT << "Passed the QtConcurrent::mapped() call, got the future:" << ExtFutureState::state(f);

    EXPECT_TRUE(f.isStarted());
    EXPECT_FALSE(f.isFinished());
    EXPECT_TRUE(f.isRunning());

	AMLMTEST_COUT << "WAITING FOR 1 SECS";//;
    // qWait() doesn't block the event loop, qSleep() does.
    TC_Sleep(1000);


	AMLMTEST_COUT << "CANCELING:" << ExtFutureState::state(f);
    f.cancel();
	AMLMTEST_COUT << "CANCELED:" << ExtFutureState::state(f);

//    TC_Wait(1000);

    EXPECT_TRUE(f.isStarted());
    EXPECT_TRUE(f.isCanceled());

    f.waitForFinished();

    if(dont_let_jobs_complete)
    {
		AMLMTEST_EXPECT_EQ(f.resultCount(), 0);
    }
    else
    {
		AMLMTEST_EXPECT_EQ(f.resultCount(), 10);
    }

//	AMLMTEST_COUT << "POST-wait RESULT:" << f.result();
	AMLMTEST_COUT << "FUTURE IS FINISHED:" << ExtFutureState::state(f);

	AMLMTEST_ASSERT_TRUE(f.isStarted());
	AMLMTEST_ASSERT_TRUE(f.isCanceled());
	AMLMTEST_ASSERT_TRUE(f.isFinished());
	AMLMTEST_ASSERT_FALSE(f.isRunning());

}

TEST_F(ExtAsyncTestsSuiteFixture, QtConcurrentMappedQFutureStateOnCancelNoCompletions)
{
    TC_ENTER();

	AMLMTEST_ASSERT_NO_FATAL_FAILURE({
    QtConcurrentMappedFutureStateOnCancel<QFuture<int>>(true);
									 });

    TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, QtConcurrentMappedExtFutureStateOnCancelNoCompletions)
{
	TC_ENTER();

	AMLMTEST_ASSERT_NO_FATAL_FAILURE({
    QtConcurrentMappedFutureStateOnCancel<ExtFuture<int>>(true);
									 });

    TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, QtConcurrentMappedQFutureStateOnCancelAllCompletions)
{
    TC_ENTER();

	AMLMTEST_ASSERT_NO_FATAL_FAILURE({
    QtConcurrentMappedFutureStateOnCancel<QFuture<int>>(false);
									 });

    TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, QtConcurrentMappedExtFutureStateOnCancelAllCompletions)
{
    TC_ENTER();

	AMLMTEST_ASSERT_NO_FATAL_FAILURE({
    QtConcurrentMappedFutureStateOnCancel<ExtFuture<int>>(false);
									 });

    TC_EXIT();
}



TEST_F(ExtAsyncTestsSuiteFixture, ExtFutureThenChainingTestExtFutures)
{
	TC_ENTER();

	AMLMTEST_SCOPED_TRACE("START");

	std::atomic_bool ran1 {false};
	std::atomic_bool ran2 {false};
	std::atomic_bool ran3 {false};

//    ExtFuture<QString> future = ExtAsync::run_1param(delayed_string_func_1, std::ref(generator_complete));
    ExtFuture<QString> future = QtConcurrent::run(delayed_string_func_1, this);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	future
    .then([&](ExtFuture<QString> extfuture) -> QString {
		AMLMTEST_SCOPED_TRACE("First then");
		TC_EXPECT_NOT_EXIT();
        TC_EXPECT_THIS_TC();

        // Check if .get() would block.  In this continuation, it shouldn't, since it shouldn't
		// run until extfuture is finished.
		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then1, got extfuture:" << extfuture;
		qDb() << "Then1, extfuture val:" << extfuture.qtget_first();
		EXPECT_EQ(ran1, false);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran1 = true;
		QString the_str = extfuture.qtget_first();
		EXPECT_EQ(the_str, QString("delayed_string_func_1() output"));
		return QString("Then1 OUTPUT");
	})
    .then([&](ExtFuture<QString> extfuture) -> QString {
		AMLMTEST_SCOPED_TRACE("Second then");
		TC_EXPECT_NOT_EXIT();
        TC_EXPECT_THIS_TC();

		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then2, got extfuture:" << extfuture;
		qDb() << "Then2, extfuture val:" << extfuture.qtget_first();
		EXPECT_EQ(ran1, true);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran2 = true;
		auto the_str = extfuture.qtget_first();
		EXPECT_EQ(the_str, QString("Then1 OUTPUT"));
		return QString("Then2 OUTPUT");
	})
    .then([&](ExtFuture<QString> extfuture) -> QString {
		AMLMTEST_SCOPED_TRACE("Third then");
		TC_EXPECT_NOT_EXIT();
        TC_EXPECT_THIS_TC();

		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then3, got extfuture:" << extfuture;
		qDb() << "Then3, extfuture val:" << extfuture.qtget_first();
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

	TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, ExtFutureThenChainingTestMixedTypes)
{
	TC_ENTER();

	bool ran1 = false;
	bool ran2 = false;
	bool ran3 = false;

    ExtFuture<QString> future = ExtAsync::run_1param(delayed_string_func_1, this);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	ExtFuture<QString> last_future = future
	.then([&](ExtFuture<QString> extfuture) -> int {

		TC_EXPECT_NOT_EXIT();

		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then1, got val:" << extfuture.qtget_first();
		EXPECT_EQ(ran1, false);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran1 = true;
		QString the_str = extfuture.qtget_first();
		EXPECT_EQ(the_str, QString("delayed_string_func_1() output"));
		return 2;
	})
	.then([&](ExtFuture<int> extfuture) -> int {

		TC_EXPECT_NOT_EXIT();

		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then2, got val:" << extfuture.qtget_first();
		EXPECT_EQ(ran1, true);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran2 = true;
		EXPECT_EQ(extfuture.qtget_first(), 2);
		return 3;
	})
	.then([&](ExtFuture<int> extfuture) -> double {
		TC_EXPECT_NOT_EXIT();

		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then3, got val:" << extfuture.qtget_first();
		EXPECT_EQ(ran1, true);
		EXPECT_EQ(ran2, true);
		EXPECT_EQ(ran3, false);
		ran3 = true;
		EXPECT_EQ(extfuture.qtget_first(), 3);

		TC_DONE_WITH_STACK();

		return 3.1415;
	});//.wait();

	last_future.wait();

	AMLMTEST_ASSERT_TRUE(last_future.isFinished());
	ASSERT_TRUE(future.isFinished());

	qDb() << "STARING WAIT";
	/// @todo This doesn't wait here, but the attached wait() above does. Which maybe makes sense.
	future.wait();
	qDb() << "ENDING WAIT";

	ASSERT_TRUE(ran1);
	ASSERT_TRUE(ran2);
	ASSERT_TRUE(ran3);

	AMLMTEST_ASSERT_TRUE(future.isFinished());

	TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, ExtFutureExtAsyncRunMultiResultTest)
{
	TC_ENTER();

	std::atomic_int start_val {5};
	std::atomic_int num_iterations {3};
	std::atomic_bool tap_complete {false};
	std::atomic_bool then_started {false};

	int last_seen_result = 0;
	int num_tap_calls = 0;

	// Start generating a sequence of results.
    ExtFuture<int> future = async_int_generator<ExtFuture<int>>(start_val, num_iterations, this);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	// Separated .then() connect.
	auto then_future = future.tap([&](int future_value) {
		AMLMTEST_SCOPED_TRACE("In first tap");
		TC_EXPECT_NOT_EXIT();
		TC_EXPECT_STACK();
        TC_EXPECT_THIS_TC();

		AMLMTEST_COUT << "testname: " << static_test_id_string;
		AMLMTEST_COUT << "num_tap_calls:" << num_tap_calls;

		EXPECT_EQ(start_val, 5);
		EXPECT_EQ(num_iterations, 3);

		if(num_tap_calls == 0)
		{
			EXPECT_EQ(start_val, 5);
			EXPECT_EQ(last_seen_result, 0);
		}

		int expected_future_val = start_val + num_tap_calls;
		AMLMTEST_COUT << "expected_future_val: " << expected_future_val;
		EXPECT_EQ(expected_future_val, future_value) << "FAIL in ExtFuture_ExtAsyncRun_multi_result_test()";
		last_seen_result = future_value;
		num_tap_calls++;
		EXPECT_LE(num_tap_calls, num_iterations);
		if(num_tap_calls == num_iterations)
		{
//			TC_DONE_WITH_STACK();
			EXPECT_FALSE(then_started);
			tap_complete = true;
		}
        })
        .then([&](ExtFuture<int> extfuture) -> int {
			then_started = true;

			AMLMTEST_SCOPED_TRACE("In then");
            TC_EXPECT_THIS_TC();

			AMLMTEST_EXPECT_TRUE(tap_complete);
			AMLMTEST_EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";

			EXPECT_FALSE(extfuture.isRunning());

			qWr() << "IN THEN:" << extfuture;
			return 1;
		;});
		then_future.wait();
//M_WARNING("THE ABOVE .wait() doesn't wait");
#if 0
		.finally([&]() -> void {
			AMLMTEST_SCOPED_TRACE("In finally");

            TC_EXPECT_THIS_TC();
			TC_EXPECT_NOT_EXIT();

			EXPECT_EQ(tap_complete, true);

			TC_DONE_WITH_STACK();
		;}).wait();
#endif

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isRunning());
	ASSERT_TRUE(future.isFinished());

	ASSERT_TRUE(then_future.isStarted());
	ASSERT_TRUE(then_future.isFinished());

	TC_EXIT();
}


TEST_F(ExtAsyncTestsSuiteFixture, TestMakeReadyFutures)
{
	TC_ENTER();

	ExtFuture<int> future = make_ready_future(45);
	ASSERT_TRUE(future.isStarted());
	ASSERT_TRUE(future.isFinished());
	ASSERT_EQ(future.qtget_first(), 45);

	TC_EXIT();
}

//TEST_F(ExtAsyncTestsSuiteFixture, ExtAsyncRun_NoExtFutureInterface)
//{
//    auto f = ExtAsync::run([=]() -> int { return 1;});
//    EXPECT_EQ(f.result(), 1);
//}

//TEST_F(ExtAsyncTestsSuiteFixture, DISABLED_UnwrapTest)
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

/// @todo EXPERIMENTAL
// Specialize an action that synchronizes with the calling thread.
// @link https://stackoverflow.com/questions/10767131/expecting-googlemock-calls-from-another-thread
ACTION_P2(ReturnFromAsyncCall, RetVal, SemDone)
{
	SemDone->release();
	return RetVal;
}

TEST_F(ExtAsyncTestsSuiteFixture, TapAndThenOneResult)
{
	TC_ENTER();

//	QSemaphore semDone;
//	using ::testing::Sequence;
//	using ::testing::Return;
//	using ::testing::ReturnArg;
//	Sequence s_outer, s_inner;

//	ON_CALL(tlm, Checkpoint(::testing::_))
//			.WillByDefault(ReturnArg<0>());
//	EXPECT_CALL(tlm, Checkpoint(1))
//			.InSequence(s_outer)
//			.WillOnce(Return(1));
//	EXPECT_CALL(tlm, Checkpoint(2))
//			.InSequence(s_outer, s_inner)
//			.WillOnce(ReturnFromAsyncCall(2, &semDone));
//	EXPECT_CALL(tlm, Checkpoint(3))
//			.InSequence(s_inner)
//			.WillOnce(ReturnFromAsyncCall(3, &semDone));


	SCOPED_TRACE("TapThenOneResult");

	bool ran_tap {false};
	bool ran_then {false};

    using FutureType = ExtFuture<QString>;

	AMLMTEST_COUT << "STARTING FUTURE";
	tlm.Checkpoint(1);
    ExtFuture<QString> future = ExtAsync::run_1param(delayed_string_func_1, this);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	AMLMTEST_COUT << "Future created:" << future;

	future/*.test_tap([=, &ran_tap](FutureType ef){
		SCOPED_TRACE("in test_tap");
        TC_EXPECT_THIS_TC();
        ExtAsync::name_qthread();
        qDb() << "Future: " << &ef;
		})*/
		.tap([=, &ran_tap, &ran_then, &tlm](QString result){
			SCOPED_TRACE("In tap");
#warning "Called multiple times"
//			tlm.Checkpoint(2);
//            ExtAsync::name_qthread();
//			TC_EXPECT_NOT_EXIT();
//			TC_EXPECT_STACK();
            TC_EXPECT_THIS_TC();

			AMLMTEST_COUT << "in tap(), result:" << tostdstr(result);
			EXPECT_EQ(result, QString("delayed_string_func_1() output"));
			ran_tap = true;
			EXPECT_FALSE(ran_then);
//			tlm.Checkpoint(3);
		;})
		.then([=, &ran_tap, &ran_then, &tlm](ExtFuture<QString> extfuture) {
			SCOPED_TRACE("In then");
//            ExtAsync::name_qthread();
//			TC_EXPECT_NOT_EXIT();
//			TC_EXPECT_STACK();
            TC_EXPECT_THIS_TC();

			EXPECT_THAT(ran_tap, ::testing::Eq(true));

			EXPECT_TRUE(extfuture.isStarted());
			EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
			EXPECT_FALSE(extfuture.isRunning());

			AMLMTEST_COUT << "in then(), extfuture:" << tostdstr(extfuture.qtget_first());
			EXPECT_EQ(extfuture.qtget_first(), QString("delayed_string_func_1() output"));
			EXPECT_FALSE(ran_then);
			ran_then = true;
//			TC_DONE_WITH_STACK();
			return QString("Then Called");
    })/*.test_tap([&](auto ef){
		AMLMTEST_COUT << "IN TEST_TAP";
        wait_result = ef.result();
        EXPECT_TRUE(wait_result[0] == QString("Then Called"));
    })*/.wait();

//    AMLMTEST_COUT << "after wait(): " << future.state().toString();
//    ASSERT_EQ(wait_result, QString("Then Called"));

//    future.wait();
    EXPECT_TRUE(future.isStarted());
    EXPECT_FALSE(future.isRunning());
    EXPECT_TRUE(future.isFinished());

    EXPECT_TRUE(ran_tap);
    EXPECT_TRUE(ran_then);
	Q_ASSERT(ran_then);

	TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, TapAndThenMultipleResults)
{
	TC_ENTER();

	std::atomic_int tap_call_counter {0};

	ExtFuture<int> future = ExtAsync::run_efarg([&](ExtFuture<int> extfuture) {
			SCOPED_TRACE("Main callback");

			TC_EXPECT_NOT_EXIT();
			TC_EXPECT_STACK();
            TC_EXPECT_THIS_TC();

			AMLMTEST_COUT << "TEST: Running from main run lambda.";
			// Sleep for a second to make sure then() doesn't run before we get to the Q_ASSERT() after this chain.
			AMLMTEST_COUT << "SLEEP 1";
            TC_Sleep(1000);
			extfuture.reportResult(867);
			AMLMTEST_COUT << "SLEEP 1";
            TC_Sleep(1000);
			extfuture.reportResult(5309);
			AMLMTEST_COUT << "SLEEP 1";
            TC_Sleep(1000);
			AMLMTEST_COUT << "FINISHED";
			extfuture.reportFinished();
			AMLMTEST_COUT << "TEST: Finished from main run lambda.";
		})
	.tap([&](int value){
		SCOPED_TRACE("value tap");
		TC_EXPECT_NOT_EXIT();
		TC_EXPECT_STACK();
        TC_EXPECT_THIS_TC();

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
        });

	// No wait, shouldn't have finished yet.
    EXPECT_TRUE(future.isStarted());
    EXPECT_FALSE(future.isFinished());

    EXPECT_FALSE(future.isFinished());
	future.wait();
    EXPECT_TRUE(future.isFinished());

    EXPECT_EQ(tap_call_counter, 2);

	TC_EXPECT_NOT_EXIT();
	TC_EXIT();
}




///// ExtAsync<>::run() tests.

TEST_F(ExtAsyncTestsSuiteFixture, ExtAsyncRunFreefunc)
{
    TC_ENTER();

    ExtFuture<int> extfuture = /*ExtAsync*/QtConcurrent::run([=](){ return 4;});

    QList<int> retval = extfuture.get();

    ASSERT_EQ(retval.size(), 1);
    ASSERT_EQ(retval[0], 4);

    TC_DONE_WITH_STACK();
    TC_EXIT();
}

/// Static checks
TEST_F(ExtAsyncTestsSuiteFixture, StaticChecks)
{

	static_assert(std::is_default_constructible<QString>::value, "");

	// From http://en.cppreference.com/w/cpp/experimental/make_ready_future:
	// "If std::decay_t<T> is std::reference_wrapper<X>, then the type V is X&, otherwise, V is std::decay_t<T>."
	static_assert(std::is_same_v<decltype(make_ready_future(4)), ExtFuture<int> >, "");
	int v;
	static_assert(!std::is_same_v<decltype(make_ready_future(std::ref(v))), ExtFuture<int&> >, "");
	/// @todo
//	static_assert(std::is_same_v<decltype(make_ready_future()), ExtFuture<void> >, "");

}

