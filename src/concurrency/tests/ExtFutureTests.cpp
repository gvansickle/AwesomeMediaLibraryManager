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
#include <string>

// Future Std C++
#include <future/future_type_traits.hpp>
#include <future/function_traits.hpp>

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
#include <tests/IResultsSequenceMock.h>

#include "../ExtAsync.h"
#include "../ExtFuture.h"
#warning "TEMP"
//#include "../impl/ExtAsync_impl.h"

//#include <continuable/continuable.hpp>

/// Types for gtest's "Typed Test" support.
using FutureIntTypes = ::testing::Types<QFuture<int>, ExtFuture<int>>;
TYPED_TEST_CASE(ExtFutureTypedTestFixture, FutureIntTypes);

//
// TESTS
//
#if 0 /// Continuable
auto http_request(std::string url) {
  return cti::make_continuable<std::string>(
	[url = std::move(url)](auto&& promise) {
	  // Resolve the promise upon completion of the task.
	  promise.set_value("<html> ... </html>");

	  // Or promise.set_exception(...);
	});
}

TEST_F(ExtFutureTest, ContinuableBasic)
{
	TC_ENTER();

	http_request("github.com")
	  .then([=] (std::string result) -> int {
		// Do something...
		  AMLMTEST_COUT << "IN CONTINUABLE THEN:" << result;
		  return 1;
	  })
			.then([=](){
		AMLMTEST_COUT << "Sleeping";
		TC_Sleep(1000);
		AMLMTEST_COUT << "Not Sleeping";
	});

	TC_EXIT();
}
#endif

//TEST_F(ExtFutureTest, NestedQTestWrapper)
//{
//	// Trying out https://stackoverflow.com/questions/39032462/can-i-check-the-gtest-filter-from-inside-a-non-gtest-test
////	tst_QString test;
////	ASSERT_NE(QTEST_FAILED, QTest::exec(&test, 0, 0));
//}

TEST_F(ExtFutureTest, ReadyFutureCompletion)
{
    TC_ENTER();

	AMLMTEST_SCOPED_TRACE("In ReadyFutureCompletion");

    /// @note Important safety tip: nL and nLL are different sizes on Windows vs. Linux.
    /// <cstdint> to the rescue.
    ExtFuture<int64_t> ef = make_ready_future(INT64_C(25));

	// Make sure it's really ready.
	AMLMTEST_COUT << "ExtFuture state:" << state(ef);
	AMLMTEST_EXPECT_TRUE(ef.isStarted());
	AMLMTEST_EXPECT_TRUE(ef.isFinished());
	AMLMTEST_EXPECT_FALSE(ef.isCanceled());

    QList<int64_t> results = ef.get();

    EXPECT_EQ(ef.state(), ExtFutureState::Started | ExtFutureState::Finished);

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], 25L);

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

    TC_EXIT();
}

#if 0
TEST_F(ExtFutureTest, UnwrappingConstructor)
{
	SCOPED_TRACE("UnwrappingConstructor");

	TC_ENTER();

	ExtFuture<int> f1;
	ExtFuture<ExtFuture<int>> f2(f1);

	f1.reportResult(29)
	f1.reportFinished();

	int val = f2.get();

	QFutureSynchronizer<void> synchronizer;
	synchronizer.addFuture(f2);
	synchronizer.addFuture(f1);
	synchronizer.waitForFinished();

	TC_EXIT();
}
#endif

//TYPED_TEST(ExtFutureTypedTestFixture, CopyAssignP)
//{
//	/// @link https://github.com/google/googletest/blob/master/googletest/docs/advanced.md#typed-tests
//    SCOPED_TRACE("CopyAssignP");

//    TC_ENTER();

//	//TypeParam f = this->value_;
//	using FutureTypeT = TypeParam;

//	FutureTypeT =

//	TC_EXIT();
//}

/**
 * Test basic cancel properties.
 */
TEST_F(ExtFutureTest, CancelBasic)
{
    TC_ENTER();

	QFutureSynchronizer<void> synchronizer;

	using TypeParam = ExtFuture<int>;

	/**
	 * @note QFuture<> behavior.
	 * The QFuture coming out of ::run() here is (Running|Started).
	 * A default construced QFuture is (Started|Canceled|Finished)
	 * I assume "Running" might not always be the case, depending on available threads.
	 */
	TypeParam f = ExtAsync::run([=](TypeParam rc_future) -> void {

		for(int i = 0; i<5; ++i)
		{
			// Do nothing for 1000 ms.
			TC_Sleep(1000);

			rc_future.reportResult(i);

			if(rc_future.HandlePauseResumeShouldICancel())
			{
				break;
			}
		}
		rc_future.reportFinished();
	});

	AMLMTEST_COUT << "Initial future state:" << state(f);

	EXPECT_TRUE(f.isStarted());
	EXPECT_TRUE(f.isRunning());
	EXPECT_FALSE(f.isCanceled());
	EXPECT_FALSE(f.isFinished());

	// ~immediately cancel the future.
    f.cancel();

	/**
	 * @note QFuture<> behavior.
	 * The QFuture after this cancel() is (Running|Started|Canceled).
	 * A default construced QFuture is (Started|Canceled|Finished)
	 * I assume "Running" might not always be the case, depending on cancel-before-start or cancel-after-completion.
	 */
	AMLMTEST_COUT << "Cancelled future state:" << state(f);

	EXPECT_TRUE(f.isStarted());
	EXPECT_TRUE(f.isCanceled());

    // Canceling alone won't finish the extfuture.
	/// @todo This is not coming back canceled.
	EXPECT_FALSE(f.isFinished()) << state(f);

    f.waitForFinished();

	EXPECT_TRUE(f.isFinished());

	AMLMTEST_COUT << "Cancelled and finished extfuture:" << state(f);

	synchronizer.addFuture(f);
	synchronizer.setCancelOnWait(true);
	synchronizer.waitForFinished();

    TC_EXIT();
}

TYPED_TEST(ExtFutureTypedTestFixture, PExceptionBasic)
{
	TC_ENTER();

	bool caught_exception = false;

	TypeParam main_future;

	main_future = ExtAsync::run([=](int) -> int {
		TC_Sleep(1000);
		AMLMTEST_COUT << "Throwing exception from other thread";
		throw QException();
		return 25;
	}, 5);

	try
	{
		// Exception should propagate here.
		main_future.waitForFinished();

	}
	catch(QException& e)
	{
		AMLMTEST_COUT << "Caught exception";
		caught_exception = true;
	}

	AMLMTEST_EXPECT_FUTURE_POST_EXCEPTION(main_future);

	AMLMTEST_ASSERT_TRUE(caught_exception);

	TC_EXIT();
}



TEST_F(ExtFutureTest, ExtFutureThenThrow)
{
	TC_ENTER();

	SCOPED_TRACE("ExtFutureThenThrow");

	ResultsSequenceMock rsm;
	enum
	{
		MSTART = 0,
		MEND = 1,
		T1STARTCB = 2,
	};
	QSemaphore semDone(0);

	using ::testing::InSequence;
	using ::testing::Return;
	using ::testing::Eq;
	using ::testing::ReturnArg;
	using ::testing::_;

	ON_CALL(rsm, ReportResult(_))
			.WillByDefault(ReturnArg<0>());
	{
		InSequence s;

		EXPECT_CALL(rsm, ReportResult(MSTART))
			.WillOnce(ReturnFromAsyncCall(MSTART, &semDone));
		EXPECT_CALL(rsm, ReportResult(T1STARTCB))
			.WillOnce(ReturnFromAsyncCall(T1STARTCB, &semDone));
		EXPECT_CALL(rsm, ReportResult(MEND))
			.WillOnce(ReturnFromAsyncCall(MEND, &semDone));
//		EXPECT_CALL(rsm, ReportResult(5))
//			.WillOnce(ReturnFromAsyncCall(5, &semDone));
	}

	// So we can assert we're getting the same ExtFuture when we enter the run() callback.
	ExtFuture<int> up;

	rsm.ReportResult(MSTART);

	up = run_again([=, &up, &rsm](ExtFuture<int> upcopy) -> int {

		rsm.ReportResult(T1STARTCB);

		AMLMTEST_EXPECT_EQ(upcopy, up);

			for(int i = 0; i < 10; i++)
			{
				TCOUT << "START Sleep:" << i << upcopy.state();
				TC_Sleep(1000);
				TCOUT << "STOP Sleep:" << i << upcopy.state();

				TCOUT << "upcopy state:" << upcopy.state();

//				if (upcopy.isCanceled())
//				{
//					// The job should be canceled.
//					// The calling runFunctor() should break out of while() loop.
//					TCOUT << "CANCELING FROM RUN() CALLBACK, upcopy state:" << upcopy.state();
//					break;
//				}
				if(upcopy.HandlePauseResumeShouldICancel())
				{
					TCOUT << "CANCELING FROM RUN() CALLBACK, upcopy state:" << upcopy.state();
					break;
				}
			}
			TCOUT << "RETURNING FROM RUN() CALLBACK, upcopy state:" << upcopy.state();

			return 1;
	});
	AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(up);

	// Pause so we're not immediately finished.
//	up.setPaused(true);
//	AMLMTEST_COUT << "UP PAUSED" << up.state();

	ExtFuture<int> down = up.then([&](ExtFuture<int> upcopy) {
		AMLMTEST_EXPECT_EQ(upcopy, up);
		TCOUT << "START up.then(), upcopy:" << upcopy.state();
//			EXPECT_TRUE(false) << "Should never get here";
			try
			{
				auto results = upcopy.get();
			}
			catch(...)
			{
				TCOUT << "CAUGHT EXCEPTION";
			}
			return 5;
			;});

	AMLMTEST_EXPECT_FALSE(down.isCanceled());

	// Check again, up should still not be finished or canceled.
	AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(up);

	AMLMTEST_COUT << "DOWN THROWING CANCEL PRE:" << down.state();
	AMLMTEST_EXPECT_FALSE(down.isFinished() || down.isCanceled());
	down.reportException(ExtAsyncCancelException());
	AMLMTEST_COUT << "DOWN THROWING CANCEL POST:" << down.state();

	try
	{
		AMLMTEST_COUT << "UP WAITING FOR EXCEPTION:" << up.state();
		up.waitForFinished();
		AMLMTEST_COUT << "UP FINISHED" << up.state();
	}
	catch (ExtAsyncCancelException& e) {
		AMLMTEST_COUT << "UP CAUGHT CANCEL" << up.state() << e.what();
	}

	TC_Sleep(1000);


	EXPECT_TRUE(down.isCanceled());
	EXPECT_TRUE(up.isCanceled()) << up;

	rsm.ReportResult(MEND);

	TC_EXIT();
}

/// @todo Don't have the infrastructure for this to work yet.
TEST_F(ExtFutureTest, ExtFutureThenCancel)
{
	TC_ENTER();

	ResultsSequenceMock rsm;
	enum
	{
		MSTART = 0,
		MEND = 1,
		T1STARTCB = 2,
		T1ENDCB = 3,
	};
	QSemaphore semDone(0);

	using ::testing::InSequence;
	using ::testing::Return;
	using ::testing::Eq;
	using ::testing::ReturnArg;
	using ::testing::_;

	ON_CALL(rsm, ReportResult(_))
			.WillByDefault(ReturnArg<0>());
	{
		InSequence s;

		EXPECT_CALL(rsm, ReportResult(MSTART))
			.WillOnce(ReturnFromAsyncCall(MSTART, &semDone));
		EXPECT_CALL(rsm, ReportResult(T1STARTCB))
			.WillOnce(ReturnFromAsyncCall(T1STARTCB, &semDone));
		EXPECT_CALL(rsm, ReportResult(T1ENDCB))
			.WillOnce(ReturnFromAsyncCall(T1ENDCB, &semDone));
		EXPECT_CALL(rsm, ReportResult(MEND))
			.WillOnce(ReturnFromAsyncCall(MEND, &semDone));
//		EXPECT_CALL(rsm, ReportResult(5))
//			.WillOnce(ReturnFromAsyncCall(5, &semDone));
	}

	rsm.ReportResult(MSTART);

	ExtFuture<int> run_down;
	QtConcurrent::run([=, &rsm, &run_down](ExtFuture<int> run_down_copy) {
		TCOUT << "IN RUN CALLBACK, run_down_copy:" << run_down_copy;
		AMLMTEST_EXPECT_EQ(run_down, run_down_copy);

		rsm.ReportResult(T1STARTCB);
		while(true)
		{
			run_down_copy.reportStarted();
			// Wait a sec before doing anything.
			TC_Sleep(1000);
			run_down_copy.reportResult(5);
			if(run_down_copy.HandlePauseResumeShouldICancel())
			{
				break;
			}
		}
		run_down_copy.reportFinished();
		rsm.ReportResult(T1ENDCB);
	}, run_down);

	AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(run_down);

	AMLMTEST_EXPECT_FALSE(run_down.isCanceled()) << run_down;
	ExtFuture<int> down = run_down.then([=, &rsm, &run_down](ExtFuture<int> upcopy){
			// Immediately return.
		AMLMTEST_EXPECT_EQ(upcopy, run_down);
		TCOUT << "THEN RETURNING";
			return 5;
			;});
	EXPECT_FALSE(down.isCanceled());

	// Wait a few ticks.
	TC_Sleep(500);

	// Cancel the downstream future.
	TCOUT << "CANCELING DOWNSTREAM" << down;
	down.reportException(ExtAsyncCancelException());
	TCOUT << "CANCELED DOWNSTREAM" << down;

	TCOUT << "WAITING TO PROPAGATE";
	TC_Wait(2000);

	EXPECT_TRUE(down.isCanceled());
	EXPECT_TRUE(run_down.isCanceled()) << run_down;

	rsm.ReportResult(MEND);

//	QFutureSynchronizer<void> synchronizer;

//	ExtFuture<int> main_future = async_int_generator<ExtFuture<int>>(1, 6, this);

//	ExtFuture<int> then_future = main_future.then([&](ExtFuture<int> in_future) {
//			AMLMTEST_EXPECT_EQ(in_future, main_future);
////			if(in_future.isCanceled())
////			{
////				qDb() << "THEN: CANCELED, throwing";
////				throw ExtAsyncCancelException();
////			}
//		// Should be canceled before we get here.
//		AMLMTEST_EXPECT_FALSE(true);
//			/// @experimental
////			AMLMTEST_COUT << "Throwing Cancel exception";
////			throw ExtAsyncCancelException();

//			// Return the count of items from the future.
//			return in_future.resultCount();
//			;});

//	AMLMTEST_COUT << "Started main_future:" << main_future.state();
//	AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(main_future);
//	AMLMTEST_COUT << "Started then_future:" << then_future.state();
//	AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(then_future);

//	// Wait a bit, then cancel the future returned by then().
//	TC_Sleep(1000);

//	AMLMTEST_COUT << "PRE: Canceling then_future:" << then_future; ///< (Running|Started)
//	AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(then_future); // One last check after the delay.
//	then_future.cancel();
////	then_future.reportCanceled();

//	AMLMTEST_COUT << "POST: Canceled then_future:" << then_future; ///< (Running|Started|Canceled)

//	AMLMTEST_EXPECT_FUTURE_POST_CANCEL(then_future);

//	// Wait a bit for the cancel to propagate.
//	AMLMTEST_COUT << "START WAITING FOR PROPAGATION";
//	TC_Wait(2000);
//	AMLMTEST_COUT << "END WAITING FOR PROPAGATION";

//	/// @note Canceling alone does not finish the main_future.
//	ASSERT_FALSE(main_future.isFinished());

//	ASSERT_TRUE(main_future.isStarted());
//	ASSERT_TRUE(main_future.isCanceled()) << main_future;
////	ASSERT_TRUE(main_future.isFinished()) << main_future;

//	main_future.waitForFinished();

//	ASSERT_TRUE(main_future.isFinished());

//	qDb() << "Canceled and finished extfuture:" << main_future;

//	synchronizer.addFuture(main_future);
//	synchronizer.addFuture(then_future);
//	synchronizer.waitForFinished();

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

TYPED_TEST(ExtFutureTypedTestFixture, ResultsTest)
{
	AMLMTEST_SCOPED_TRACE("Results");

	TC_ENTER();

	QList<int> expected_results {2,3,4,5,6};
	QList<int> results = results_test<TypeParam>(2, 5, this);

	EXPECT_EQ(results, expected_results);

	TC_EXIT();
}

template <class FutureType, class TestFixtureType>
void streaming_tap_test(int startval, int iterations, TestFixtureType* fixture)
{
	AMLMTEST_SCOPED_TRACE("IN streaming_tap_test");

	std::atomic_int num_tap_completions {0};
	QList<int> async_results_from_tap, async_results_from_get;

	// Start the async generator.
	FutureType ef = async_int_generator<FutureType>(startval, iterations, fixture);

	GTEST_COUT_qDB << "Starting ef state:" << ExtFutureState::state(ef);
	EXPECT_TRUE(ef.isStarted());
	EXPECT_FALSE(ef.isCanceled());
	EXPECT_FALSE(ef.isFinished());

	QList<int> expected_results {1,2,3,4,5,6};


	GTEST_COUT_qDB << "Attaching tap and get()";

	FutureType f2 = make_default_future<FutureType, int>();
	EXPECT_TRUE(f2.isStarted());
	EXPECT_FALSE(f2.isCanceled());
	EXPECT_FALSE(f2.isFinished());

	if constexpr (false)//!std::is_same_v<QFuture<int>, FutureType>)
	{
		M_WARNING("TODO: This is still spinning when the test exits.");
		f2 = ef.tap(qApp, [=, &async_results_from_tap](FutureType ef, int begin, int end) mutable {
			GTEST_COUT_qDB << "IN TAP, begin:" << begin << ", end:" << end;
			for(int i = begin; i<end; i++)
			{
				GTEST_COUT_qDB << "Pushing" << ef.resultAt(i) << "to tap list.";
				async_results_from_tap.push_back(ef.resultAt(i));
				num_tap_completions++;
			}
		});
	}
	else
	{
		QtConcurrent::run([=, &async_results_from_tap, &num_tap_completions](FutureType ef, FutureType f2){
			AMLMTEST_COUT << "TAP: START TAP RUN(), ef:" << state(ef) << "f2:" << state(f2);

			if(true /* Roll our own */)
			{
				int i = 0;

				while(true)
				{
					AMLMTEST_COUT << "TAP: Waiting for next result";
					/**
					  * QFutureInterfaceBase::waitForResult(int resultIndex)
					  * - if exception, rethrow.
					  * - if !running, return.
					  * - stealAndRunRunnable()
					  * - lock mutex.
					  * - const int waitIndex = (resultIndex == -1) ? INT_MAX : resultIndex;
					  *   while (isRunning() && !d->internal_isResultReadyAt(waitIndex))
					  *     d->waitCondition.wait(&d->m_mutex);
					  *   d->m_exceptionStore.throwPossibleException();
					  */
					ef.d.waitForResult(i);

					// Check if the wait failed to result in any results.
					int result_count = ef.resultCount();
					if(result_count <= i)
					{
						// No new results, must have finshed etc.
						AMLMTEST_COUT << "NO NEW RESULTS, BREAKING, ef:" << state(ef);
						break;
					}

					// Copy over the new results
					for(; i < result_count; ++i)
					{
						AMLMTEST_COUT << "TAP: Next result available at i = " << i;

						int the_next_val = ef.resultAt(i);
						async_results_from_tap.append(the_next_val);
						f2.d.reportResult(the_next_val);
						num_tap_completions++;

						// Make sure we don't somehow get here too many times.
						AMLMTEST_EXPECT_LT(i, iterations);
					}
				}

				AMLMTEST_COUT << "LEFT WHILE(!Finished) LOOP, ef state:" << state(ef);

				// Check final state.  We know it's at least Finished.
				/// @todo Could we be Finished here with pending results?
				/// Don't care as much on non-Finished cases.
				if(ef.isCanceled())
				{
					AMLMTEST_COUT << "TAP: ef cancelled:" << state(ef);
					/// @todo PROPAGATE
				}
				else if(ef.isFinished())
				{
					AMLMTEST_COUT << "TAP: ef finished:" << state(ef);
					/// @todo PROPAGATE
				}
				else
				{
					/// @todo Exceptions.
					AMLMTEST_COUT << "NOT FINISHED OR CANCELED:" << state(ef);
				}
			}
			else if(false /* Use Java-like iterator */)
			{
				AMLMTEST_COUT << "Starting Java-like iterator";

				QFutureIterator<int> fit(ef);

				AMLMTEST_COUT << "Created Java-like iterator";

				while(fit.hasNext())
				{
					AMLMTEST_COUT << "TAP: GOT hasNext:" << state(ef);
					int the_next_val = fit.next();
					AMLMTEST_COUT << "TAP: GOT RESULT:" << the_next_val;
					//reportResult(&f2, *cit);
					f2.d.reportResult(the_next_val);
					async_results_from_tap.append(the_next_val);
					num_tap_completions++;
				}
			}
			else if(false /* Use STL iterators */)
			{
				auto cit = ef.constBegin();
				while(cit != ef.constEnd())
				{
					AMLMTEST_COUT << "GOT RESULT:" << *cit;
					//reportResult(&f2, *cit);
					f2.d.reportResult(*cit);
					async_results_from_tap.append(*cit);
					num_tap_completions++;

					++cit;
				}
			}
			else // All at once
			{
				/// @note This is passing 40/40 with both QFuture<> and ExtFuture<>,
				/// but f2 doesn't get intermediate results.
				AMLMTEST_COUT << "TAP: PENDING ON ALL RESULTS";
				QVector<int> results = ef.results().toVector();
				AMLMTEST_COUT << "TAP: GOT RESULTS:" << results;
				AMLMTEST_ASSERT_EQ(results.size(), iterations);
				f2.d.reportResults(results); //ef.results().toVector());
				async_results_from_tap.append(results.toList()); //ef.results());
				num_tap_completions += ef.resultCount();
			}

			AMLMTEST_ASSERT_TRUE(ef.isFinished());

			f2.d.reportFinished();
			AMLMTEST_COUT << "EXIT TAP RUN(), ef:" << state(ef) << "resultCount:" << ef.resultCount()
						  << "f2:" << state(f2) << "resultCount:" << f2.resultCount();
			},
		ef, f2);
	}
	GTEST_COUT_qDB << "BEFORE WAITING FOR results()" << state(f2);

	async_results_from_get = f2.results();

	GTEST_COUT_qDB << "AFTER WAITING FOR results()" << state(f2);

	// .results() above should block until f2 is finished.
	AMLMTEST_EXPECT_TRUE(f2.isFinished());
	// If f2 is finished, ef must have finished.
	AMLMTEST_EXPECT_TRUE(ef.isFinished());

	// This shouldn't do anything, should already be finished.
	ef.waitForFinished();

	GTEST_COUT_qDB << "Post .tap().get(), extfuture:" << ExtFutureState::state(ef);

	EXPECT_TRUE(ef.isStarted());
	EXPECT_FALSE(ef.isCanceled());
	EXPECT_TRUE(ef.isFinished());

	// Check the results we got.
	EXPECT_EQ(async_results_from_get.size(), 6);
	EXPECT_EQ(async_results_from_get, expected_results);
	EXPECT_EQ(async_results_from_tap.size(), 6);
	EXPECT_EQ(async_results_from_tap, expected_results);
	AMLMTEST_EXPECT_EQ(num_tap_completions, 6);

	EXPECT_TRUE(ef.isFinished());
}

/**
 * Test1 "streaming" tap().
 * @todo Currently crashes.
 */
TYPED_TEST(ExtFutureTypedTestFixture, PFutureStreamingTap)
{
	TC_ENTER();

	AMLMTEST_ASSERT_NO_FATAL_FAILURE({
										 streaming_tap_test<TypeParam>(1, 6, this);
									 });

	if (::testing::Test::HasFatalFailure())
	{
		AMLMTEST_COUT << "HIT HasFatalFailure";
		return;
	}

	TC_EXIT();
}

TEST_F(ExtFutureTest, ExtFutureSingleThen)
{
	TC_ENTER();

	using eftype = ExtFuture<int>;

	std::atomic_int num_then_completions {0};
	QList<int> async_results_from_then, async_results_from_get;


	QList<int> expected_results {1,2,3,4,5,6};
	eftype ef = async_int_generator<eftype>(1, 6, this);

	GTEST_COUT_qDB << "Starting ef state:" << ef.state();
	ASSERT_TRUE(ef.isStarted());
	ASSERT_FALSE(ef.isCanceled());
	ASSERT_FALSE(ef.isFinished());

	AMLMTEST_COUT << "Attaching then()";

	auto f2 = ef.then([=, &async_results_from_then, &num_then_completions](eftype ef) -> int  {
			AMLMTEST_COUT << "IN THEN, future:" << ef.state() << ef.resultCount();
			AMLMTEST_EXPECT_TRUE(ef.isFinished());
			async_results_from_then = ef.get();
			num_then_completions++;
			return 5;
	});

	AMLMTEST_EXPECT_TRUE(f2.isStarted());
	AMLMTEST_EXPECT_FALSE(f2.isCanceled());
	AMLMTEST_EXPECT_FALSE(f2.isFinished());

	GTEST_COUT_qDB << "BEFORE WAITING FOR THEN()" << f2;

	// Block.
	async_results_from_get = f2.results();

	GTEST_COUT_qDB << "AFTER WAITING FOR THEN()" << f2;

	EXPECT_TRUE(ef.isFinished());
	EXPECT_EQ(num_then_completions, 1);

	// .get() above should block.
	EXPECT_TRUE(ef.isFinished());

	// This shouldn't do anything, should already be finished.
	ef.waitForFinished();

	GTEST_COUT_qDB << "Post .tap().get(), extfuture:" << ef.state();

	EXPECT_TRUE(ef.isStarted());
	EXPECT_FALSE(ef.isCanceled());
	EXPECT_TRUE(ef.isFinished());

	EXPECT_EQ(async_results_from_get.size(), 1);
	EXPECT_EQ(async_results_from_get[0], 5);
	EXPECT_EQ(async_results_from_then.size(), 6);
	EXPECT_EQ(async_results_from_then, expected_results);

	ASSERT_TRUE(ef.isFinished());

	TC_EXIT();
}

TEST_F(ExtFutureTest, ThenChain)
{
	TC_ENTER();

	SCOPED_TRACE("ThenChain");

	ResultsSequenceMock rsm;
	QSemaphore semDone(0);

	using ::testing::InSequence;
	using ::testing::Return;
	using ::testing::Eq;
	using ::testing::ReturnArg;
	using ::testing::_;

	ON_CALL(rsm, ReportResult(_))
			.WillByDefault(ReturnArg<0>());
	{
		InSequence s;

		EXPECT_CALL(rsm, ReportResult(0))
			.WillOnce(ReturnFromAsyncCall(0, &semDone));
		EXPECT_CALL(rsm, ReportResult(1))
			.WillOnce(ReturnFromAsyncCall(1, &semDone));
		EXPECT_CALL(rsm, ReportResult(2))
			.WillOnce(ReturnFromAsyncCall(2, &semDone));
		EXPECT_CALL(rsm, ReportResult(5))
			.WillOnce(ReturnFromAsyncCall(5, &semDone));
	}

	using FutureType = ExtFuture<QString>;

	AMLMTEST_COUT << "STARTING FUTURE";
	ExtFuture<QString> future = ExtAsync::run(delayed_string_func_1, this);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	AMLMTEST_COUT << "Future created:" << future;

	rsm.ReportResult(0);

	future.then([&rsm](FutureType in_future){
			SCOPED_TRACE("In then 1");

			rsm.ReportResult(1);
			return 1;
		;})
		.then([=, &rsm](ExtFuture<int> in_future) {
			SCOPED_TRACE("In then 2");

//			EXPECT_THAT(ran_tap, Eq(true));

			EXPECT_TRUE(in_future.isStarted());
			EXPECT_TRUE(in_future.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
			EXPECT_FALSE(in_future.isRunning());

//			AMLMTEST_COUT << "in then(), extfuture:" << tostdstr(extfuture.qtget_first());
//			EXPECT_EQ(in_future.qtget_first(), QString("delayed_string_func_1() output"));
//			EXPECT_FALSE(ran_then);
//			ran_then = true;

			rsm.ReportResult(2);

			return QString("Then Called");
	})/*.test_tap([&](auto ef){
		AMLMTEST_COUT << "IN TEST_TAP";
		wait_result = ef.result();
		EXPECT_TRUE(wait_result[0] == QString("Then Called"));
	})*/.wait();

//    AMLMTEST_COUT << "after wait(): " << future.state().toString();
//    ASSERT_EQ(wait_result, QString("Then Called"));

	future.wait();
	EXPECT_TRUE(future.isStarted());
	EXPECT_FALSE(future.isRunning());
	EXPECT_TRUE(future.isFinished());

	rsm.ReportResult(5);

//	EXPECT_TRUE(ran_tap);
//	EXPECT_TRUE(ran_then);
//	Q_ASSERT(ran_then);

	bool acquired = semDone.tryAcquire(1, 1000);
	EXPECT_TRUE(acquired);

	TC_EXIT();
}


/**
 * Test streaming tap().
 */
TEST_F(ExtFutureTest, ExtFutureStreamingTap)
{
    TC_ENTER();

	using eftype = ExtFuture<int>;

	std::atomic_int num_tap_completions {0};
	QList<int> async_results_from_tap, async_results_from_get;


    QList<int> expected_results {1,2,3,4,5,6};
    eftype ef = async_int_generator<eftype>(1, 6, this);

    GTEST_COUT_qDB << "Starting ef state:" << ef.state();
    ASSERT_TRUE(ef.isStarted());
    ASSERT_FALSE(ef.isCanceled());
    ASSERT_FALSE(ef.isFinished());

    GTEST_COUT_qDB << "Attaching tap and get()";

//    async_results_from_get =
M_WARNING("TODO: This is still spinning when the test exits.");
	auto f2 = ef.tap(qApp, [=, &async_results_from_tap, &num_tap_completions](eftype ef, int begin, int end) -> void  {
            GTEST_COUT_qDB << "IN TAP, begin:" << begin << ", end:" << end;
        for(int i = begin; i<end; i++)
        {
            GTEST_COUT_qDB << "Pushing" << ef.resultAt(i) << "to tap list.";
            async_results_from_tap.push_back(ef.resultAt(i));
            num_tap_completions++;
        }
	});

	AMLMTEST_EXPECT_TRUE(f2.isStarted());
	AMLMTEST_EXPECT_FALSE(f2.isCanceled());
	AMLMTEST_EXPECT_FALSE(f2.isFinished());

	GTEST_COUT_qDB << "BEFORE WAITING FOR GET()" << f2;

	async_results_from_get = f2.results();

	GTEST_COUT_qDB << "AFTER WAITING FOR GET()" << f2;

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
}
