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
#include <chrono>
#include <string>

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
#include <concurrency/ThreadPool2.h>

// Ours
#include <tests/TestHelpers.h>
#include "ExtAsyncTestCommon.h"
#include <concurrency/ExtFuture.h>
#include <tests/IResultsSequenceMock.h>

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

	TCOUT << "delayed_string_func() returning" << tostdstr(retval);

	return ExtFuture<QString>(retval);
}


//
// TESTS
//

static std::string create(const char *s)
{
	using namespace std::chrono_literals;

	TCOUT << "3s CREATE \"" << s << "\"\n";
	std::this_thread::sleep_for(3ms);
	return {s};
}

static std::string concat(const std::string &a, const std::string &b)
{
	using namespace std::chrono_literals;

	TCOUT << "5s CONCAT "
		  << "\"" << a << "\" "
		  << "\"" << b << "\"\n";
	std::this_thread::sleep_for(5ms);
	return a + b;
}

static std::string twice(const std::string &s)
{
	using namespace std::chrono_literals;

	TCOUT << "3s TWICE \"" << s << "\"\n";
	std::this_thread::sleep_for(3ms);
	return s + s;
}

#if 0 // This stuff has been moved to attic, probably obsolete.
TEST_F(ExtAsyncTestsSuiteFixture, AsynchronizeBasic)
{
	TC_ENTER();

	auto pcreate (ExtAsync::asynchronize(create));
	auto pconcat (ExtAsync::async_adapter(concat));
	auto ptwice (ExtAsync::async_adapter(twice));

	/// @note result is not a std::string here, it is a callable returning a std::future<std::string>.
	auto result (
				pconcat(
					ptwice(
						pconcat(
							pcreate("foo "),
							pcreate("bar "))),
					pconcat(
						pcreate("this "),
						pcreate("that "))));
	TCOUT << "Setup done. Nothing executed yet.\n";

	auto retval = std::string(result().get());

	TCOUT << "Calculated retval:" << retval << "\n";

	const std::string expected_str {"foo bar foo bar this that "};
	AMLMTEST_EXPECT_EQ(expected_str, retval);

	TC_EXIT();
}
#endif

/// Concurrent C++
template<typename T>
struct sorter
{
	thread_pool pool;

	std::list<T> do_sort(std::list<T>& chunk_data)
	{
		if(chunk_data.empty())
		{
			return chunk_data;
		}

		std::list<T> result;
		result.splice(result.begin(),chunk_data,chunk_data.begin());
		T const& partition_val=*result.begin();

		typename std::list<T>::iterator divide_point=
			std::partition(
				chunk_data.begin(),chunk_data.end(),
				[&](T const& val){return val<partition_val;});

		std::list<T> new_lower_chunk;
		new_lower_chunk.splice(
			new_lower_chunk.end(),
			chunk_data,chunk_data.begin(),
			divide_point);

		thread_pool::task_handle<std::list<T> > new_lower=
			pool.submit(
				std::bind(
					&sorter::do_sort,this,
					std::move(new_lower_chunk)));

		std::list<T> new_higher(do_sort(chunk_data));

		result.splice(result.end(),new_higher);
		while(!new_lower.is_ready())
		{
			pool.run_pending_task();
		}

		result.splice(result.begin(),new_lower.get());
		return result;
	}
};
template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
	if(input.empty())
	{
		return input;
	}
	sorter<T> s;

	return s.do_sort(input);
}

TEST_F(ExtAsyncTestsSuiteFixture, CCPPBasic)
{
	TC_ENTER();
/// @todo
//	parallel_quick_sort();

	TC_EXIT();
}

/// END Concurrent C++

INSTANTIATE_TEST_SUITE_P(ExtAsyncParameterizedTests,
						ExtAsyncTestsParameterized,
						::testing::Bool());

TEST_P(ExtAsyncTestsParameterized, ExtAsyncQthreadAsyncException)
{
	TC_ENTER();

	ExtFuture<int> f0 = ExtAsync::qthread_async([=]() -> int {
		/*TCOUT*/qDebug() << "THROWING";
		TC_Sleep(1000);
//		throw ExtAsyncCancelException();
		throw QException();
//		throw std::exception();
		TCOUT << "ABOUT TO LEAVE THREAD AND RETURN 5";
		return 5;
		;});

	TC_Wait(500);
	TCOUT << "ABOUT TO TRY";

	try
	{
		f0.wait();
		ADD_FAILURE() << "Didn't throw";
	}
	catch(ExtAsyncCancelException& e)
	{
		TCOUT << "CAUGHT CANCEL EXCEPTION";
		SUCCEED();
	}
	catch(QException& e)
	{
		TCOUT << "CAUGHT CANCEL EXCEPTION";
		SUCCEED();
	}
	catch(...)
	{
		ADD_FAILURE() << "Threw unexpected exception.";
	}

	TCOUT << "ABOUT TO LEAVE TEST";

	TC_EXIT();
}

TEST_P(ExtAsyncTestsParameterized, ExtAsyncQthreadAsyncThenCancelExceptionFromTopOrBottom)
{
	TC_ENTER();

	bool cancel_from_top = GetParam();

	ExtFuture<int> f0 = ExtAsync::qthread_async([=]() -> int {
		TC_Sleep(1000);
		if(cancel_from_top)
		{
			/*TCOUT*/qDebug() << "THROWING CANCEL FROM TOP";
			throw ExtAsyncCancelException();
			ADD_FAILURE() << "Didn't throw out of thread to future.";
		}

		TCOUT << "ABOUT TO LEAVE THREAD AND RETURN 5";
		return 5;
		});
	ExtFuture<int> f1 = f0
		.then_qthread_async([=](ExtFuture<int> f0){

			qDb() << "In then(), triggering via .wait() for cancel exception.";
			// This should throw on a cancel.
			f0.wait();

//			int f0val = f0.get()[0];
			return 1;
		});

	TC_Wait(500);
	TCOUT << "ABOUT TO TRY";

	try
	{
		if(!cancel_from_top)
		{
			/*TCOUT*/qDebug() << "THROWING CANCEL FROM BOTTOM THEN'S RETURNED FUTURE";
			Q_ASSERT(!f1.isCanceled());
			Q_ASSERT(!f1.isFinished());
			if(0)
			{
				f1.reportException(ExtAsyncCancelException());
				f1.reportFinished();
			}
			else
			{
				f1.cancel();
			}
		}
		f0.wait();
		ADD_FAILURE() << ".wait() Didn't throw, f1:" << f1;
	}
	catch(ExtAsyncCancelException& e)
	{
		qDb() << "CAUGHT CANCEL EXCEPTION:" << e.what();
		SUCCEED();
		EXPECT_TRUE(f0.isCanceled()) << ExtFutureState::state(f0);
	}
	catch(QException& e)
	{
		ADD_FAILURE() << "CAUGHT NON-CANCEL EXCEPTION";
	}
	catch(...)
	{
		std::exception_ptr eptr = std::current_exception();
		try { std::rethrow_exception(eptr); }
		catch (const std::exception& e)
		{
			ADD_FAILURE() << "Threw unexpected exception." << e.what();
		}
	}

	TCOUT << "ABOUT TO LEAVE TEST";

	TC_EXIT();
}

TEST_P(ExtAsyncTestsParameterized, ExtAsyncQthreadAsyncMultiThenCancelExceptionFromTopOrBottom)
{
	TC_ENTER();

	bool cancel_from_top = GetParam();

	ExtFuture<int> f1 = ExtAsync::qthread_async([=]() -> int {
		TC_Sleep(1000);
		if(cancel_from_top)
		{
			/*TCOUT*/qDebug() << "THROWING CANCEL FROM TOP";
			throw ExtAsyncCancelException();
			ADD_FAILURE() << "Didn't throw out of thread to future.";
		}

		TCOUT << "ABOUT TO LEAVE THREAD AND RETURN 5";
		return 5;
	});
	auto fend = f1
	.then_qthread_async([=](ExtFuture<int> f0){
		qDb() << "Waiting in then() for cancel exception, f0:" << f0;
		f0.wait();

		ADD_FAILURE() << "f0.wait() didn't throw:" << f0;

		return 1;
	})
	.then_qthread_async([=](ExtFuture<int> f2){
		qDb() << "Waiting in then() for cancel exception, f2:" << f2;
		f2.wait();
		ADD_FAILURE() << "f2.wait() didn't throw:" << f2;

		return 1;
	});
	f1.setName("f1");
	fend.setName("fend");

	TC_Wait(500);

	if(!cancel_from_top)
	{
		/*TCOUT*/qDebug() << "THROWING CANCEL FROM BOTTOM THEN";
		fend.cancel();
	}

	TCOUT << "ABOUT TO TRY";

	try
	{
		f1.wait();
		ADD_FAILURE() << ".wait() Didn't throw";
	}
	catch(ExtAsyncCancelException& e)
	{
		TCOUT << "CAUGHT CANCEL EXCEPTION";
		SUCCEED();
		EXPECT_TRUE(f1.isCanceled());
	}
	catch(QException& e)
	{
		ADD_FAILURE() << "CAUGHT NON-CANCEL EXCEPTION";
	}
	catch(...)
	{
		ADD_FAILURE() << "Threw unexpected exception.";
	}

	TCOUT << "ABOUT TO LEAVE TEST";

	TC_EXIT();
}

constexpr auto c_started_running = QFutureInterfaceBase::State(QFutureInterfaceBase::Started
		| QFutureInterfaceBase::Running);

constexpr auto c_started_finished_canceled = QFutureInterfaceBase::State(QFutureInterfaceBase::Started
		| QFutureInterfaceBase::Finished
		| QFutureInterfaceBase::Canceled);


TEST_F(ExtAsyncTestsSuiteFixture, ExceptionsWhatDoesQtCRunDo)
{
	TC_ENTER();

	///
	/// QtConcurrent::run():
	///
	qDb() << "############# START QtConcurrent::run()";
	QFuture<int> qf0 = QtConcurrent::run([&](){
		qDb() << "QFUTURE:" << ExtFutureState::state(qf0);
		EXPECT_EQ(ExtFutureState::state(qf0), c_started_running);
		throw QException(); return 1; });

	while(!qf0.isCanceled() && !qf0.isFinished())
	{
		TC_Wait(1000);
	}

	qDb() << "QFUTURE:" << ExtFutureState::state(qf0);
	EXPECT_TRUE(ExtFutureState::state(qf0) == c_started_finished_canceled);

	// Does it throw?
	try
	{
		qf0.waitForFinished();
		ADD_FAILURE() << "QFuture didn't throw";
	}
	catch(QException& e)
	{
		qDb() << "QFUTURE/caught exception:" << ExtFutureState::state(qf0);
		qDb() << "Caught:" << e.what();
		EXPECT_EQ(ExtFutureState::state(qf0), c_started_finished_canceled);
	}
	catch(...)
	{
		ADD_FAILURE() << "Caught the wrong exception type";
	}

	///
	/// ExtAsync::qthread_async():
	///
	qDb() << "############# START ExtAsync::qthread_async()";
	ExtFuture<int> exf0 = ExtAsync::qthread_async_with_cnr_future([](ExtFuture<int> exf0){
		EXPECT_EQ(ExtFutureState::state(exf0), c_started_running);
		qDb() << "EXTFUTURE:" << ExtFutureState::state(exf0);
		throw QException(); return 1; });

	while(!exf0.isCanceled() && !exf0.isFinished())
	{
		TC_Wait(1000);
	}
	qDb() << "EXTFUTURE:" << ExtFutureState::state(exf0);
	EXPECT_TRUE(ExtFutureState::state(exf0) == c_started_finished_canceled);

	// Does it throw?
	try
	{
		exf0.waitForFinished();
		ADD_FAILURE() << "ExtFuture didn't throw";
	}
	catch(QException& e)
	{
		qDb() << "EXTFUTURE/caught exception:" << ExtFutureState::state(exf0);
		qDb() << "Caught:" << e.what();
		EXPECT_EQ(ExtFutureState::state(exf0), c_started_finished_canceled);
	}
	catch(...)
	{
		ADD_FAILURE() << "Caught the wrong exception type";
	}

	///
	/// ExtAsync::qthread_async_with_cnr_future():
	///
	qDb() << "############# START ExtAsync::qthread_async_with_cnr_future()";
	ExtFuture<int> cnrf0 = ExtAsync::qthread_async_with_cnr_future([](ExtFuture<int> cnr_future){
		EXPECT_EQ(ExtFutureState::state(cnr_future), c_started_running);
		qDb() << "EXTFUTURE:" << ExtFutureState::state(cnr_future);
		throw QException(); return 1; });

	while(!cnrf0.isCanceled() && !cnrf0.isFinished())
	{
		TC_Wait(1000);
	}
	qDb() << "EXTFUTURE:" << ExtFutureState::state(cnrf0);
	EXPECT_TRUE(ExtFutureState::state(cnrf0) == c_started_finished_canceled) << cnrf0;

	// Does it throw?
	try
	{
		cnrf0.waitForFinished();
		ADD_FAILURE() << "ExtFuture didn't throw";
	}
	catch(QException& e)
	{
		qDb() << "EXTFUTURE/caught exception:" << ExtFutureState::state(cnrf0);
		qDb() << "Caught:" << e.what();
		EXPECT_EQ(ExtFutureState::state(cnrf0), c_started_finished_canceled);
	}
	catch(...)
	{
		ADD_FAILURE() << "Caught the wrong exception type";
	}

	TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, ExceptionsSingleThenish)
{
	TC_ENTER();



	TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, ExtAsyncQthreadAsyncThenCancelExceptionFromBottom)
{
	TC_ENTER();

	ExtFuture<int> f1 = ExtAsync::qthread_async_with_cnr_future([=](ExtFuture<int> in_fut) -> int {
		for(int i = 0; i<10; i++)
		{
			TCOUT << "qthread_async_with_cnr_future() iteration:" << i;
			// Do nothing for a sec.
			TC_Sleep(1000);

			if(in_fut.HandlePauseResumeShouldICancel())
			{
				// We're being canceled.
				if(in_fut.isCanceled())
				{
					qDb() << "IN_FUT is already canceled";
				}
				if(in_fut.isFinished())
				{
					TCOUT << "IN_FUT is already finished";
				}
				qDb() << "LEAVING TOP LOOP DUE TO CANCEL";
				in_fut.reportException(ExtAsyncCancelException());
				in_fut.reportFinished();
				return 0;
			}
		}

		ADD_FAILURE() << "Finished thread function not due to cancel.";
		return 5;
		})
		.then_qthread_async([=](ExtFuture<int> f0){
		qDb() << "Waiting in then() for cancel exception.";
		EXPECT_TRUE(f0.is_ready());

			QList<int> result;

			result = f0.get();
			ADD_FAILURE() << "get() DIDN'T THROW";

			EXPECT_TRUE(f0.isCanceled());
//			EXPECT_TRUE(f0.hasException());
			TCOUT << "WAIT START" << f0;
			f0.wait();
			TCOUT << "WAIT END" << f0;
			ADD_FAILURE() << ".wait() didn't throw";
//			int f0val = f0.get()[0];
			EXPECT_TRUE(f0.isCanceled());
			return 1;//f0val;
		})
		.then_qthread_async([=](ExtFuture<int> f2){
			qDb() << "Waiting in then() for cancel exception.";
			EXPECT_TRUE(f2.is_ready());

				QList<int> result;

				result = f2.get();
				ADD_FAILURE() << "get() DIDN'T THROW";

				EXPECT_TRUE(f2.isCanceled());
	//			EXPECT_TRUE(f2.hasException());
				TCOUT << "WAIT START" << f2;
				f2.wait();
				TCOUT << "WAIT END" << f2;
				ADD_FAILURE() << ".wait() didn't throw";
	//			int f0val = f0.get()[0];
				EXPECT_TRUE(f2.isCanceled());
				return 2;//f0val;
			});

	f1.setName("f1");

	TC_Wait(500);

	qDb() << "ABOUT TO TRY TO QUIT FROM THE BOTTOM";

	try
	{
		// Cancel the last future in the chain.
		qDb() << "ABOUT TO CANCEL";
		EXPECT_FALSE(f1.isFinished());
		EXPECT_FALSE(f1.isCanceled());
//		f1.cancel();
		f1.reportException(ExtAsyncCancelException());
		f1.wait();

		ADD_FAILURE() << "Wait after cancel didn't throw";
	}
	catch(ExtAsyncCancelException& e)
	{
		TCOUT << "CAUGHT CANCEL EXCEPTION";
		SUCCEED();
		EXPECT_TRUE(f1.isCanceled());
	}
	catch(QException& e)
	{
		ADD_FAILURE() << "CAUGHT NON-CANCEL EXCEPTION";
	}
	catch(...)
	{
		ADD_FAILURE() << "Threw unexpected exception.";
	}

	TCOUT << "ABOUT TO LEAVE TEST";

	TC_EXIT();
}

static int use1(int a) {return a*a;}
static int use2(int a) {return a/a;}
static int use3(int a) {return a+a;}

TEST_F(ExtAsyncTestsSuiteFixture, ExtAsyncQthreadAsyncMultipleGet)
{
	TC_ENTER();

	int arg = 5;

	ExtFuture<int> ftr = ExtAsync::qthread_async([=]{ return arg*arg; } );

	if ( rand() > RAND_MAX/2 )
	{
		use1( ftr.get_first() );
	} else
	{
		use2(  ftr.get_first() );
	}
	use3( ftr.get_first() );

	TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, QtConcurrentSanityTest)
{
    SCOPED_TRACE("A");
	TC_ENTER();

    std::atomic_int counter {0};

	TCOUT << "CALLING ::RUN";

    /// @note When Qt says the returned QFuture can't be canceled, they mean it.
	/// If you do, things get totally screwed up and this will segfault.
	QFuture<int> f = QtConcurrent::run([&]() -> int {
		TCOUT << "Entered callback";
        TC_Sleep(1000);
        counter = 1;
		TCOUT << "T+1 secs";
        TC_Sleep(1000);
        counter = 2;
        return 5;
        ;});

	EXPECT_TRUE(f.isStarted());
	/// @note QFuture<> f's state here is Running|Started.  EXPECT is here to see if Running is always the case,
	/// I don't think it necessarily is.
//	EXPECT_TRUE(f.isRunning()) << "QtConcurrent::run() doesn't always return a Running future.";
	TCOUT << "CALLED ::RUN, FUTURE STATE:" << ExtFutureState::state(f);

    EXPECT_TRUE(f.isStarted());
    TC_Sleep(500);
    EXPECT_TRUE(f.isRunning()); // In the first TC_Sleep(1000);
    TC_Sleep(1000);
	TCOUT << "CHECKING COUNTER FOR 1";//; // In the second TC_Sleep(1000);
    EXPECT_EQ(counter, 1);

	TCOUT << "WAITING FOR FINISHED";
    f.waitForFinished();
	/// @note This QFuture<>, which was returned by QtConcurrent::run(), is Started|Finished here.
	/// So, that's the natural state of a successfully finished QFuture<>.
	TCOUT << "QFUTURE FINISHED, state:" << ExtFutureState::state(f);
    EXPECT_FALSE(f.isCanceled());
    EXPECT_TRUE(f.isFinished());

    // Can't cancel a QtConcurrent::run() future, so the callback should have run to completion.
    EXPECT_EQ(counter, 2);
	TCOUT << "CHECKING QFUTURE RESULT";
    EXPECT_EQ(f.resultCount(), 1);
    int res = f.result();
    EXPECT_EQ(res, 5);
	TCOUT << "CHECKED QFUTURE RESULT";

    f.waitForFinished();
    EXPECT_TRUE(f.isStarted());
    EXPECT_TRUE(f.isFinished());
	TCOUT << "RETURNING";

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
	else
	{
		the_future = ExtAsync::make_started_only_future<int>();
	}

    ASSERT_TRUE(the_future.isStarted());
    ASSERT_FALSE(the_future.isCanceled());
    ASSERT_FALSE(the_future.isFinished());

	TCOUT << "CALLING QTC::run()";

    /**
     * Per docs:
     * "Note that function may not run immediately; function will only be run once a thread becomes available."
     * @link http://doc.qt.io/qt-5/qtconcurrent.html#mappedReduced-1
     */
	auto f = QtConcurrent::run([=,&counter](FutureTypeT the_passed_future)  {
		TCOUT << "Entered callback, passed future state:" << ExtFutureState::state(the_passed_future);

            EXPECT_TRUE(the_passed_future.isStarted());
            EXPECT_FALSE(the_passed_future.isCanceled());

            while(!the_passed_future.isCanceled())
            {
				TCOUT << "LOOP COUNTER: " << counter;

                // Pretend to do 1 second of work.
                TC_Sleep(1000);
                counter++;
				TCOUT << "+1 secs, counter = " << counter;
            }

            // Only way to exit the while loop above is by external cancellation.
            EXPECT_TRUE(the_passed_future.isCanceled());

            /// @note For both QFuture<> and ExtFuture<>, we need to finish the future ourselves in this case.
            ///       Not sure if the waitForFinished() should be here or rely on caller to do it.
			reportFinished(&the_passed_future);
//            the_passed_future.waitForFinished();

		 TCOUT << "Exiting callback, passed future state:" << ExtFutureState::state(the_passed_future);
		 ;}, the_future);

	TCOUT << "Passed the run() call, got the future:" << ExtFutureState::state(the_future);

    EXPECT_TRUE(the_future.isStarted());
    EXPECT_TRUE(f.isStarted());

	TCOUT << "WAITING FOR 5 SECS...";
    // qWait() doesn't block the event loop, qSleep() does.
    TC_Wait(5000);

	TCOUT << "CANCELING THE FUTURE:" << ExtFutureState::state(the_future);
    the_future.cancel();
	TCOUT << "CANCELED FUTURE STATE:" << ExtFutureState::state(the_future);

//    TC_Sleep(1000);

    // We don't really care about the future returned from QtConcurrent::run(), but it should be finished by now.
    /// @todo Sometimes f not finished here, problem?
//	f.waitForFinished();
//	EXPECT_TRUE(f.isFinished());// << ExtFutureState::state(ExtFuture<Unit>(f));

    EXPECT_TRUE(the_future.isStarted());
    EXPECT_TRUE(the_future.isCanceled());

    the_future.waitForFinished();
//    the_future.result();

	TCOUT << "FUTURE IS FINISHED:" << ExtFutureState::state(the_future);

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
	try
	{
	AMLMTEST_SCOPED_TRACE("IN QtConcurrentMappedFutureStateOnCancel template function");

	std::atomic_int map_callback_run_counter {0};

	QVector<int> test_input_vector{0,1,2,3,4,5,6,7,8,9};
	QVector<int> test_output_vector{1,2,3,4,5,6,7,8,9,10};


	// The lambda which does the mapping.
	std::function<int(const int&)> lambda = [&](const int& the_passed_value) -> int {

//		AMLMTEST_SCOPED_TRACE("IN LAMBDA");

//		TCOUT << "Entered callback, passed value:" << the_passed_value;

		if(dont_let_jobs_complete)
		{
			// None of the jobs should complete before the future is canceled.
//			TCOUT << "SLEEPING FOR 2 secs";
			TC_Sleep(2000);
		}
		else
		{
			// Let them all complete.
//			TCOUT << "SLEEPING FOR 0.5 secs";
			TC_Sleep(500);
		}

		// In the run counter.
		map_callback_run_counter++;

//		AMLMTEST_EXPECT_LE(map_callback_run_counter, test_input_vector.size());

		return the_passed_value + 1;
	}; // End lambda


	/**
	 * Per docs:
	 * "Note that function may not run immediately; function will only be run once a thread becomes available."
	 * @link http://doc.qt.io/qt-5/qtconcurrent.html#mappedReduced-1
	 */
	TCOUT << "Calling QtConcurrent::mapped(), dont_let_jobs_complete:" << dont_let_jobs_complete;
	FutureTypeT mapped_results_future = static_cast<FutureTypeT>(QtConcurrent::mapped(test_input_vector, lambda));
	/// @note If/when the lambdas are called, they will sleep for either 2 secs or 0.5 secs.

	// mapped_results_future state should be Started, maybe Running.
	TCOUT << "After the QtConcurrent::mapped() call, got the future:" << ExtFutureState::state(mapped_results_future);

	AMLMTEST_EXPECT_TRUE(mapped_results_future.isStarted());
	AMLMTEST_EXPECT_TRUE(mapped_results_future.isRunning()) << "State is:" << state(mapped_results_future);
	AMLMTEST_EXPECT_FALSE(mapped_results_future.isCanceled());
	AMLMTEST_EXPECT_FALSE(mapped_results_future.isFinished());

	TCOUT << "SLEEPING FOR 1000";
	/// @note So this will wake back up and either all the lambdas will have been called and finished, or none will have.
	///       However, the mapped() call/future may not be finished yet.
    TC_Sleep(1000);

	// f state: QFuture == if(dont_let_jobs_complete):(Running|Started)==still running
	//                     else: (Started|Finished) == finished.
	TCOUT << "CANCELING:" << ExtFutureState::state(mapped_results_future);
	if(dont_let_jobs_complete)
	{
		/// @note Jobs take 2 sec to complete case.  We should always still be Running here.
		AMLMTEST_EXPECT_TRUE(mapped_results_future.isRunning() && mapped_results_future.isStarted()) << state(mapped_results_future);
	}
	else
	{
		/// @note Jobs take 0.5 sec to complete case, we should not still be Running here, and should be Finished.
		/// @note However, it sporadically is coming back as not finished.
		EXPECT_TRUE(mapped_results_future.isStarted());
		AMLMTEST_EXPECT_TRUE(mapped_results_future.isFinished()) << mapped_results_future;
		EXPECT_TRUE(!mapped_results_future.isRunning()) << mapped_results_future;
		TCOUT << "WARNING: Canceling already-finished future";
	}

	// Cancel the async mapping operation.
	mapped_results_future.cancel();

	// mapped_results_future state: QFuture == Just adds the Cancel flag.  cancel() code brings it out of Paused if it was paused.
	//                                         dont_let...: (Running|Started|Canceled)
	//                                         else:        (Started|Finished|Canceled)
	/// @note Expect Running to sometimes be cleared.
	/// @note CANCELED QFUTURES ARE NOT NECESSARILY FINISHED.
	TCOUT << "CANCELED:" << ExtFutureState::state(mapped_results_future);
	if(dont_let_jobs_complete)
	{
		AMLMTEST_EXPECT_TRUE(mapped_results_future.isRunning() && mapped_results_future.isStarted() && mapped_results_future.isCanceled()) << ExtFutureState::state(mapped_results_future);
	}
	else
	{
		AMLMTEST_EXPECT_TRUE(mapped_results_future.isFinished() && mapped_results_future.isStarted() && mapped_results_future.isCanceled()) << ExtFutureState::state(mapped_results_future);
	}

	TCOUT << "START SLEEP FOR 1000";
	TC_Sleep(1000);
	TCOUT << "END FOR 1000 COMPLETE";

	///
	/// @wth Google test is completing here with an "OK" result.  ???
	///

	/// @note CANCELED QFUTURES ARE NOT IMMEDIATELY FINISHED, but if you wait a while they will be?
//	TCOUT << "STATE AFTER TC_WAIT:" << ExtFutureState::state(mapped_results_future);


	if(dont_let_jobs_complete)
	{
		/// @note 2 sec case.  We should not be finished here, but:
		/// @note Something's wrong here. If we expect isRunning, we get Finished, if we expect isFinished, we get Running.
		/// Both is{Finished,Running}() just query the QIFB state.
		/// If we expect either, it seems like things work fine.  W. T. H.
		TCOUT << "DONT";
		AMLMTEST_EXPECT_TRUE((mapped_results_future.isRunning() || mapped_results_future.isFinished()));
		AMLMTEST_EXPECT_TRUE(mapped_results_future.isStarted());
		AMLMTEST_EXPECT_TRUE(mapped_results_future.isCanceled());
	}
	else
	{
		/// @note 0.5 sec case,
		TCOUT << "DO";
//		TCOUT << ExtFutureState::state(mapped_results_future);
		AMLMTEST_EXPECT_TRUE(mapped_results_future.isFinished() && mapped_results_future.isStarted() && mapped_results_future.isCanceled());// << state(f);
	}

	// Check if the operation is still running.  If it is, we have to waitForFinished().
	/// @todo I think we need to do a check for (Canceled|Finished) here regardless, then call waitForFinished().

	TCOUT << "CALLING waitForFinished()";// << ExtFutureState::state(mapped_results_future);

	try
	{
		mapped_results_future.waitForFinished();
//		mapped_results_future.result();
	}
	catch(...)
	{
		FAIL() << "wait threw an exception.";
	}


//	TCOUT << "RETURNED FROM waitForFinished():" << ExtFutureState::state(mapped_results_future);
	/// @note QFuture is always (Started|Finished|Canceled) here.
	AMLMTEST_EXPECT_TRUE(mapped_results_future.isFinished() && mapped_results_future.isStarted() && mapped_results_future.isCanceled() && !mapped_results_future.isRunning());

	// They should either all complete or none should.
    if(dont_let_jobs_complete)
    {
		AMLMTEST_EXPECT_EQ(mapped_results_future.resultCount(), 0);
    }
    else
    {
		AMLMTEST_EXPECT_EQ(mapped_results_future.resultCount(), 10);
    }

	TCOUT << "FUTURE IS FINISHED:" << ExtFutureState::state(mapped_results_future);

	}
	catch(...)
	{
		FAIL() << "Test function threw when it shouldn't have";
	}
}

TEST_F(ExtAsyncTestsSuiteFixture, QtConcurrentMappedQFutureStateOnCancelNoCompletions)
{
    TC_ENTER();

//	AMLMTEST_ASSERT_NO_FATAL_FAILURE(
    QtConcurrentMappedFutureStateOnCancel<QFuture<int>>(true);
//									 );
	if(HasFailure())
	{
		FAIL();
	}

    TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, QtConcurrentMappedExtFutureStateOnCancelNoCompletions)
{
	TC_ENTER();

//	AMLMTEST_ASSERT_NO_FATAL_FAILURE(
    QtConcurrentMappedFutureStateOnCancel<ExtFuture<int>>(true);
//									 );
	if(HasFailure())
	{
		FAIL();
	}

    TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, QtConcurrentMappedQFutureStateOnCancelAllCompletions)
{
    TC_ENTER();

//	AMLMTEST_ASSERT_NO_FATAL_FAILURE(
    QtConcurrentMappedFutureStateOnCancel<QFuture<int>>(false);
//									 );
	if(HasFailure())
	{
		FAIL();
	}

    TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, QtConcurrentMappedExtFutureStateOnCancelAllCompletions)
{
    TC_ENTER();

//	AMLMTEST_ASSERT_NO_FATAL_FAILURE(
    QtConcurrentMappedFutureStateOnCancel<ExtFuture<int>>(false);
//									 );
	if(HasFailure())
	{
		FAIL();
	}

    TC_EXIT();
}

int mapper(const int &i)
{
	QTest::qWait(1);
	return i;
}

TEST_F(ExtAsyncTestsSuiteFixture, MappedIncrementalResults)
{
	TC_ENTER();

	const int count = 200;
	QList<int> ints;
	for (int i=0; i < count; ++i)
	{
		ints << i;
	}

	ExtFuture<int> future = QtConcurrent::mapped(ints, mapper);

	QList<int> results;

	while (future.isFinished() == false)
	{
		for (int i = 0; i < future.resultCount(); ++i)
		{
			results += future.resultAt(i);
		}

		QTest::qWait(1);
	}

	AMLMTEST_ASSERT_EQ(future.isFinished(), true);
	AMLMTEST_ASSERT_EQ(future.resultCount(), count);
	AMLMTEST_ASSERT_EQ(future.results().count(), count);

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
	/// @todo NOT CANCELABLE
	ExtFuture<QString> future(QtConcurrent::run(delayed_string_func_1, this));

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
		qDb() << "Then1, extfuture val:" << extfuture.get_first();
		EXPECT_EQ(ran1, false);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran1 = true;
		QString the_str = extfuture.get_first();
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
		qDb() << "Then2, extfuture val:" << extfuture.get_first();
		EXPECT_EQ(ran1, true);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran2 = true;
		auto the_str = extfuture.get_first();
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
		qDb() << "Then3, extfuture val:" << extfuture.get_first();
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

	ExtFuture<QString> future = ExtAsync::run(delayed_string_func_1, this);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	ExtFuture<double> last_future = future
	.then([&](ExtFuture<QString> extfuture) -> int {

		TC_EXPECT_NOT_EXIT();

		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then1, got val:" << extfuture.get_first();
		EXPECT_EQ(ran1, false);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran1 = true;
		QString the_str = extfuture.get_first();
		EXPECT_EQ(the_str, QString("delayed_string_func_1() output"));
		return 2;
	})
	.then([&](ExtFuture<int> extfuture) -> int {

		TC_EXPECT_NOT_EXIT();

		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then2, got val:" << extfuture.get_first();
		EXPECT_EQ(ran1, true);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran2 = true;
		EXPECT_EQ(extfuture.get_first(), 2);
		return 3;
	})
	.then([&](ExtFuture<int> extfuture) -> double {
		TC_EXPECT_NOT_EXIT();

		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		EXPECT_FALSE(extfuture.isRunning());

		qDb() << "Then3, got val:" << extfuture.get_first();
		EXPECT_EQ(ran1, true);
		EXPECT_EQ(ran2, true);
		EXPECT_EQ(ran3, false);
		ran3 = true;
		EXPECT_EQ(extfuture.get_first(), 3);

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

		TCOUT << "testname: " << static_test_id_string;
		TCOUT << "num_tap_calls:" << num_tap_calls;

		EXPECT_EQ(start_val, 5);
		EXPECT_EQ(num_iterations, 3);

		if(num_tap_calls == 0)
		{
			EXPECT_EQ(start_val, 5);
			EXPECT_EQ(last_seen_result, 0);
		}

		int expected_future_val = start_val + num_tap_calls;
		TCOUT << "expected_future_val: " << expected_future_val;
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

	ExtFuture<int> future = ExtAsync::make_ready_future(45);
	ASSERT_TRUE(future.isStarted());
	ASSERT_TRUE(future.isFinished());
	ASSERT_EQ(future.get_first(), 45);

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

using ::testing::InSequence;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::_;
using ::testing::Eq;

TEST_F(ExtAsyncTestsSuiteFixture, TapAndThenOneResult)
{
	TC_ENTER();

	TC_START_RSM(rsm);

	// default behavior of ReportResult.
	ON_CALL(rsm, ReportResult(_))
			.WillByDefault(ReturnArg<0>());
	{
		InSequence s;

		TC_RSM_EXPECT_CALL(rsm, 0);
		TC_RSM_EXPECT_CALL(rsm, 1);
//		EXPECT_CALL(rsm, ReportResult(0))
//			.WillOnce(ReturnFromAsyncCall(0, &semDone));
//		EXPECT_CALL(rsm, ReportResult(1))
//			.WillOnce(ReturnFromAsyncCall(1, &semDone));
		EXPECT_CALL(rsm, ReportResult(2))
			.WillRepeatedly(ReturnFromAsyncCall(2, &rsm_sem_done));
		TC_RSM_EXPECT_CALL(rsm, 3);
		TC_RSM_EXPECT_CALL(rsm, 5);

//		EXPECT_CALL(rsm, ReportResult(3))
//			.WillOnce(ReturnFromAsyncCall(3, &semDone));
//		EXPECT_CALL(rsm, ReportResult(5))
//			.WillOnce(ReturnFromAsyncCall(5, &semDone));
	}

	SCOPED_TRACE("TapThenOneResult");

	rsm.ReportResult(0);

	std::atomic_bool ran_tap {false};
	std::atomic_bool ran_then {false};

    using FutureType = ExtFuture<QString>;

	TCOUT << "STARTING FUTURE";

	ExtFuture<QString> future = ExtAsync::run(delayed_string_func_1, this);

	rsm.ReportResult(1);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	TCOUT << "Future created:" << future;

	future
		.tap([&](QString result){
			SCOPED_TRACE("In tap");

			// Called multiple times.
			rsm.ReportResult(2);

			TCOUT << "in tap(), result:" << tostdstr(result);
			EXPECT_EQ(result, QString("delayed_string_func_1() output"));
			ran_tap = true;
			EXPECT_FALSE(ran_then);
		;})
		.then([&](ExtFuture<QString> extfuture) {
			SCOPED_TRACE("In then");

			rsm.ReportResult(3);

			EXPECT_THAT(ran_tap, ::testing::Eq(true));

			EXPECT_TRUE(extfuture.isStarted());
			EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
			EXPECT_FALSE(extfuture.isRunning());

			TCOUT << "in then(), extfuture:" << tostdstr(extfuture.get_first());
			EXPECT_EQ(extfuture.get_first(), QString("delayed_string_func_1() output"));
			EXPECT_FALSE(ran_then);
			ran_then = true;
			return QString("Then Called");
    })/*.test_tap([&](auto ef){
		TCOUT << "IN TEST_TAP";
        wait_result = ef.result();
        EXPECT_TRUE(wait_result[0] == QString("Then Called"));
    })*/.wait();

//    TCOUT << "after wait(): " << future.state().toString();
//    ASSERT_EQ(wait_result, QString("Then Called"));

//    future.wait();
	AMLMTEST_EXPECT_FUTURE_FINISHED(future);

	rsm.ReportResult(5);

    EXPECT_TRUE(ran_tap);
    EXPECT_TRUE(ran_then);
	Q_ASSERT(ran_then);

	TC_END_RSM(rsm);

	TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, TapAndThenMultipleResults)
{
	TC_ENTER();

	std::atomic_int tap_call_counter {0};

	ExtFuture<int> future = ExtAsync::run([&](ExtFuture<int> extfuture) {
			SCOPED_TRACE("Main callback");

			TC_EXPECT_NOT_EXIT();
			TC_EXPECT_STACK();
            TC_EXPECT_THIS_TC();

			TCOUT << "TEST: Running from main run lambda.";
			// Sleep for a second to make sure then() doesn't run before we get to the Q_ASSERT() after this chain.
			TCOUT << "SLEEP 1";
            TC_Sleep(1000);
			extfuture.reportResult(867);
			TCOUT << "SLEEP 1";
            TC_Sleep(1000);
			extfuture.reportResult(5309);
			TCOUT << "SLEEP 1";
            TC_Sleep(1000);
			TCOUT << "FINISHED";
			extfuture.reportFinished();
			TCOUT << "TEST: Finished from main run lambda.";
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
/// @todo Pretty broken if we have to directly use QtConcurrent::run().
//	ExtFuture<int> extfuture = static_cast<ExtFuture<int>>(/*ExtAsync*/QtConcurrent::run([=](){ return 4;}));
	ExtFuture<int> extfuture = ExtAsync::run_again([=](ExtFuture<int> f) -> int { int val = 4; f.reportFinished(&val); return val; });

	QList<int> retval = extfuture.get();

    ASSERT_EQ(retval.size(), 1);
    ASSERT_EQ(retval[0], 4);

    TC_DONE_WITH_STACK();
    TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, RunInQThreadTest)
{
	TC_ENTER();

	int val = 0;
	std::set<int> seen_tap_values;

	ExtFuture<int> f0 = ExtAsync::qthread_async_with_cnr_future([&](ExtFuture<int> ef, int testval1) {
		EXPECT_EQ(f0, ef);
		TCOUT << M_ID_VAL(testval1);
		while(val < 10)
		{
			TCOUT << "val:" << val;
			ef.reportResult(val);
			val++;
			TC_Sleep(100);
		}
		ef.reportFinished();;
	}, 7);

	TCOUT << "POST run(), f0:" << f0;

	auto flast = f0.stap([&](ExtFuture<int> in_future, int begin, int end){
		for(int i = begin; i < end; i++)
		{
			seen_tap_values.insert(in_future.resultAt(i));
		}
	}).then([&](ExtFuture<int> fut_from_upstream){
		EXPECT_TRUE(fut_from_upstream.is_ready());
		QList<int> then_in_val = fut_from_upstream.get();

		TCOUT << M_ID_VAL(then_in_val);

		// Check that we got the right values.
		int exp_i = 0;
		for(const int i : then_in_val)
		{
			EXPECT_EQ(i, exp_i);
			exp_i++;
		}
	;});

	flast.wait();

//	flast.waitForFinished();

	EXPECT_EQ(val, 10);
	EXPECT_EQ(seen_tap_values.size(), 10);

	TC_EXIT();
}

TEST_F(ExtAsyncTestsSuiteFixture, RunFreeFuncInQThreadWithEventLoop)
{
	TC_ENTER();

	std::atomic_bool ran_callback = false;

	ExtFuture<int> f0 = ExtAsync::run_in_qthread_with_event_loop([&](ExtFuture<int> f, int passed_in_val){
			TCOUT << "ENTERED CALLBACK";

			TC_Sleep(1000);

			ran_callback = true;

			f.reportResult(passed_in_val);
	}, 4);

	EXPECT_FALSE(f0.isFinished());
	EXPECT_FALSE(ran_callback);

	f0.wait();

	EXPECT_TRUE(f0.isFinished());
	EXPECT_TRUE(ran_callback);
	EXPECT_EQ(f0.result(), 4);

	TC_EXIT();
}

/// Static checks
TEST_F(ExtAsyncTestsSuiteFixture, StaticChecks)
{

	static_assert(std::is_default_constructible<QString>::value);

	// From http://en.cppreference.com/w/cpp/experimental/make_ready_future:
	// "If std::decay_t<T> is std::reference_wrapper<X>, then the type V is X&, otherwise, V is std::decay_t<T>."
	static_assert(std::is_same_v<decltype(ExtAsync::make_ready_future(4)), ExtFuture<int> >);
	int v;
	static_assert(!std::is_same_v<decltype(ExtAsync::make_ready_future(std::ref(v))), ExtFuture<int&> >);
	/// @todo
//	static_assert(std::is_same_v<decltype(ExtAsync::make_ready_future()), ExtFuture<void> >);

}

