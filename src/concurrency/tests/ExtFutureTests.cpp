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

#define QFUTURE_TEST
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
#include <private/qfutureinterface_p.h>  // For test purposes only.

// Google Test
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Ours
#include <tests/TestHelpers.h>
#include "ExtAsyncTestCommon.h"
#include <tests/IResultsSequenceMock.h>

#include "../ExtAsync.h"
#include "../ExtFuture.h"

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

#if 1 /// Boost::thread
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

int calculate_the_answer_to_life_the_universe_and_everything()
{
	return 42;
}

TEST_F(ExtFutureTest, BoostSanity)
{
	boost::packaged_task<int> pt(calculate_the_answer_to_life_the_universe_and_everything);

	boost::unique_future<int> fi=pt.get_future();

	boost::thread task(boost::move(pt)); // launch task on a thread

	fi.wait(); // wait for it to finish

	EXPECT_TRUE(fi.is_ready());
	EXPECT_TRUE(fi.has_value());
	EXPECT_TRUE(!fi.has_exception());
	EXPECT_EQ(fi.get_state(), boost::future_state::ready);
	EXPECT_EQ(fi.get(), 42);
}

#if 0 // Broken
TEST_F(ExtFutureTest, BoostThenCancelCascade)
{
	TC_ENTER();

//	auto tp = QThreadPool::globalInstance();

	TC_START_RSM(rsm);
	enum
	{
		MSTART,
		MEND,
		J1STARTCB,
		J1CANCELED,
		J1ENDCB,
		T1STARTCB,
		T1ENDCB,
	};

	using ::testing::InSequence;
	using ::testing::Return;
	using ::testing::Eq;
	using ::testing::ReturnArg;
	using ::testing::_;

	ON_CALL(rsm, ReportResult(_))
			.WillByDefault(ReturnArg<0>());
	{
		InSequence s;

		TC_RSM_EXPECT_CALL(rsm, MSTART);
		TC_RSM_EXPECT_CALL(rsm, J1STARTCB);
		TC_RSM_EXPECT_CALL(rsm, J1CANCELED);
		TC_RSM_EXPECT_CALL(rsm, J1ENDCB);
//		TC_RSM_EXPECT_CALL(rsm, T1STARTCB);
//		TC_RSM_EXPECT_CALL(rsm, T1ENDCB);
		TC_RSM_EXPECT_CALL(rsm, MEND);
	}

	std::atomic_bool ran_generator_task_callback {false};
	std::atomic_bool ran_then1_callback {false};
	std::atomic_bool ran_then2_callback {false};

	///
	/// Setup done, test starts here.
	///

	rsm.ReportResult(MSTART);

	// Log the number of free threads in the thread pool.
//	LogThreadPoolInfo(tp);

	// The async generator task.  Spins forever, reporting "5" to run_down until canceled.
	boost::unique_future<int> generator_task_future;
	generator_task_future = boost::async(
				[=, &ran_generator_task_callback, &ran_then1_callback, &ran_then2_callback, &rsm, &generator_task_future]
					  (boost::unique_future<int> generator_task_future_copy) -> int {
		// Check the atomics.
		AMLMTEST_EXPECT_FALSE(ran_generator_task_callback);
		AMLMTEST_EXPECT_FALSE(ran_then1_callback);
		AMLMTEST_EXPECT_FALSE(ran_then2_callback);
		ran_generator_task_callback = true;

		// If we're not (Running|Started) here, something's wildly wrong.
//		AMLMTEST_EXPECT_TRUE(generator_task_future_copy.isStarted());
//		AMLMTEST_EXPECT_TRUE(generator_task_future_copy.isRunning());
//		TCOUT << "IN RUN CALLBACK, generator_task_future_copy:" << generator_task_future_copy;
		AMLMTEST_EXPECT_EQ(generator_task_future, generator_task_future_copy);

		rsm.ReportResult(J1STARTCB);

		while(true)
		{
			// Wait one second before doing anything.
			TC_Sleep(1000);
			// Report a result to downstream.
//			generator_task_future_copy.reportResult(5);
			generator_task_future_copy.(5);
			// Handle canceling.
			if(generator_task_future_copy.HandlePauseResumeShouldICancel())
			{
				rsm.ReportResult(J1CANCELED);
				generator_task_future_copy.reportCanceled();
				break;
			}
		}

		// We've been canceled, but not finished.
		AMLMTEST_EXPECT_TRUE(generator_task_future_copy.isCanceled());
		AMLMTEST_EXPECT_FALSE(generator_task_future_copy.isFinished());
		rsm.ReportResult(J1ENDCB);
		generator_task_future_copy.reportFinished();
		return 1;
	}
	);

	AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(generator_task_future);
	AMLMTEST_EXPECT_FALSE(generator_task_future.isCanceled()) << generator_task_future;

	// Then 1
	ExtFuture<int> downstream_then1 = generator_task_future.then([=, &ran_generator_task_callback, &ran_then1_callback, &ran_then2_callback, &rsm, &generator_task_future]
										(ExtFuture<int> upstream_future_copy) -> int{
		AMLMTEST_EXPECT_EQ(upstream_future_copy, generator_task_future);
		// Check the atomics.
		AMLMTEST_EXPECT_TRUE(ran_generator_task_callback);
		AMLMTEST_EXPECT_FALSE(ran_then1_callback);
		AMLMTEST_EXPECT_FALSE(ran_then2_callback);
		ran_then1_callback = true;

		// Should always be finished if we get in here.
		AMLMTEST_EXPECT_TRUE(upstream_future_copy.isFinished());
		// For this test, we should also be canceled.
		AMLMTEST_EXPECT_TRUE(upstream_future_copy.isCanceled());

		std::exception_ptr eptr; // For rethrowing.
		try
		{
			TCOUT << "THEN1 GETTING, future state:" << upstream_future_copy;
			QList<int> incoming = upstream_future_copy.get();

//			if(upstream_future_copy.isCanceled())
		}
		catch(...)
		{
			TCOUT << "THEN1 RETHROWING, future state:" << upstream_future_copy;
			eptr = std::current_exception();
		}
		// Do we need to rethrow?
		if(eptr)
		{
			qDb() << "rethrowing.";
			std::rethrow_exception(eptr);
		}

		// No try.  This should throw to down.
//		auto results_from_upstream = upcopy.results();
//		ADD_FAILURE() << "We should never get in here on a cancelation.";
		// Immediately return.
		TCOUT << "THEN1 RETURNING, future state:" << upstream_future_copy;
		return 5;
	});
	AMLMTEST_EXPECT_FALSE(downstream_then1.isCanceled());

	// Then 2
	ExtFuture<int> downstream_then2 = downstream_then1.then([=, &ran_generator_task_callback, &ran_then1_callback, &ran_then2_callback, &rsm, &downstream_then1]
									 (ExtFuture<int> upstream_future_copy) -> int {
		AMLMTEST_EXPECT_EQ(upstream_future_copy, downstream_then1);
		// Check the atomics.
		AMLMTEST_EXPECT_TRUE(ran_generator_task_callback);
		AMLMTEST_EXPECT_TRUE(ran_then1_callback);
		AMLMTEST_EXPECT_FALSE(ran_then2_callback);

		// Should always be finished if we get in here.
		AMLMTEST_EXPECT_TRUE(upstream_future_copy.isFinished());
		// For this test, we should also be canceled.
		AMLMTEST_EXPECT_TRUE(upstream_future_copy.isCanceled());
		ran_then2_callback = true;

		std::exception_ptr eptr; // For rethrowing.
		try
		{
			TCOUT << "THEN2 GETTING, future state:" << upstream_future_copy;
			QList<int> incoming = upstream_future_copy.get();
		}
		catch(...)
		{
			TCOUT << "THEN2 RETHROWING, future state:" << upstream_future_copy;
			eptr = std::current_exception();
		}
		// Do we need to rethrow?
		if(eptr)
		{
			qDb() << "rethrowing.";
			std::rethrow_exception(eptr);
		}

		TCOUT << "THEN2 RETURNING, future state:" << upstream_future_copy;
		return 6;
	});
	AMLMTEST_EXPECT_FALSE(downstream_then2.isCanceled());

	// Ok, both then()'s attached, less than a second before the promise sends its first result.

	// Check the number of free threads in the thread pool again.
//	LogThreadPoolInfo(tp);

	// Wait a few ticks.
	TC_Sleep(1000);

	// Cancel the downstream future.
	TCOUT << "CANCELING TAIL downstream_then2:" << downstream_then2;
	downstream_then2.cancel();
//	downstream_then2.reportException(ExtAsyncCancelException());
	TCOUT << "CANCELED TAIL downstream_then2:" << downstream_then2;

	TCOUT << "WAITING FOR CANCEL TO PROPAGATE";
	TC_Sleep(2000);
	TCOUT << "CANCEL SHOULD HAVE PROPAGATED";

//	AMLMTEST_EXPECT_TRUE(downstream_then2.isCanceled()) << downstream_then2;
//	AMLMTEST_EXPECT_TRUE(downstream_then1.isCanceled()) << downstream_then1;
//	AMLMTEST_EXPECT_TRUE(generator_task_future.isCanceled()) << generator_task_future;

	AMLMTEST_EXPECT_TRUE(ran_generator_task_callback);
	AMLMTEST_EXPECT_TRUE(ran_then1_callback);
	AMLMTEST_EXPECT_TRUE(ran_then2_callback);

	rsm.ReportResult(MEND);

	TC_END_RSM(rsm);

	TC_EXIT();
}
#endif // Broken

#endif /// Boost::thread

//TEST_F(ExtFutureTest, NestedQTestWrapper)
//{
//	// Trying out https://stackoverflow.com/questions/39032462/can-i-check-the-gtest-filter-from-inside-a-non-gtest-test
////	tst_QString test;
////	ASSERT_NE(QTEST_FAILED, QTest::exec(&test, 0, 0));
//}

///
/// Start of qtbase-inspired tests.
///

// qtbase-inspired test helpers
class IntResult : public QFutureInterface<int>
{
public:
	ExtFuture<int> run()
	{
		this->reportStarted();
		ExtFuture<int> future = ExtFuture<int>(this);

		int res = 10;
		reportFinished(&res);
		return future;
	}
};

int value = 10;

#if 0 /// @todo
class UnitResult : public QFutureInterfaceBase
{
public:
	ExtFuture<Unit> run()
	{
		this->reportStarted();
		/// @todo No EF<Unit> constructor from QFutureInterfaceBase.
//		ExtFuture<Unit> future = ExtFuture<Unit>(this);
		reportFinished();
		return future;
	}
};
#endif

TEST_F(ExtFutureTest, QTBfuture)
{
	// default constructors
	ExtFuture<int> intFuture;
	intFuture.waitForFinished();
	ExtFuture<QString> stringFuture;
	stringFuture.waitForFinished();
	ExtFuture<Unit> voidFuture;
	voidFuture.waitForFinished();
	ExtFuture<Unit> defaultVoidFuture;
	defaultVoidFuture.waitForFinished();

	// copy constructor
	ExtFuture<int> intFuture2(intFuture);
	ExtFuture<Unit> voidFuture2(defaultVoidFuture);

	// assigmnent operator
	intFuture2 = ExtFuture<int>();
	voidFuture2 = ExtFuture<Unit>();

	// state
	AMLMTEST_EXPECT_TRUE(intFuture2.isStarted());
	AMLMTEST_EXPECT_TRUE(intFuture2.isFinished());
}

TEST_F(ExtFutureTest, QTBfutureInterface1)
{
	ExtFuture<Unit> future;
	{
		QFutureInterface<Unit> i;
		i.reportStarted();
		future = i.future();
		i.reportFinished();
	}
}

TEST_F(ExtFutureTest, QTBfutureInterface2)
{
	ExtFuture<int> future;
	{
		QFutureInterface<int> i;
		i.reportStarted();
		i.reportResult(10);
		future = i.future();
		i.reportFinished();
	}
	EXPECT_EQ(future.resultAt(0), 10);
}

TEST_F(ExtFutureTest, QTBfutureInterface3)
{
	ExtFuture<int> intFuture;

	AMLMTEST_EXPECT_TRUE(intFuture.isStarted());
/// @todo
//	AMLMTEST_EXPECT_TRUE(intFuture.isFinished());

	IntResult result;

	result.reportStarted();
	intFuture = result.future();

	AMLMTEST_EXPECT_TRUE(intFuture.isStarted());
	AMLMTEST_EXPECT_FALSE(intFuture.isFinished());

	result.reportFinished(&value);

	AMLMTEST_EXPECT_TRUE(intFuture.isStarted());
	AMLMTEST_EXPECT_TRUE(intFuture.isFinished());

	int e = intFuture.result();

	AMLMTEST_EXPECT_TRUE(intFuture.isStarted());
	AMLMTEST_EXPECT_TRUE(intFuture.isFinished());
	AMLMTEST_EXPECT_FALSE(intFuture.isCanceled());

	AMLMTEST_ASSERT_EQ(e, value);
	intFuture.waitForFinished();

	IntResult intAlgo;
	intFuture = intAlgo.run();
	ExtFuture<int> intFuture2(intFuture);
	AMLMTEST_ASSERT_EQ(intFuture.result(), value);
	AMLMTEST_ASSERT_EQ(intFuture2.result(), value);
	intFuture.waitForFinished();

#if 0 /// @todo
	UnitResult a;
	a.run().waitForFinished();
#endif
}

template <typename T>
void QTBtestRefCounting()
{
	QFutureInterface<T> interface;
	AMLMTEST_ASSERT_EQ(interface.d->refCount.load(), 1);

	{
		interface.reportStarted();

		ExtFuture<T> f = ExtFuture<T>(interface.future());
		AMLMTEST_ASSERT_EQ(interface.d->refCount.load(), 2);

		ExtFuture<T> f2(f);
		AMLMTEST_ASSERT_EQ(interface.d->refCount.load(), 3);

		ExtFuture<T> f3;
		f3 = f2;
		AMLMTEST_ASSERT_EQ(interface.d->refCount.load(), 4);

		interface.reportFinished(0);
		AMLMTEST_ASSERT_EQ(interface.d->refCount.load(), 4);
	}

	AMLMTEST_ASSERT_EQ(interface.d->refCount.load(), 1);
}

TEST_F(ExtFutureTest, QTBrefcounting)
{
	QTBtestRefCounting<int>();
}

TEST_F(ExtFutureTest, QTBcancel1)
{
	ExtFuture<Unit> f;
	QFutureInterface<Unit> result;

	result.reportStarted();
	f = result.future();
	AMLMTEST_ASSERT_FALSE(f.isCanceled());
	result.reportCanceled();
	AMLMTEST_ASSERT_TRUE(f.isCanceled());
	result.reportFinished();
	AMLMTEST_ASSERT_TRUE(f.isCanceled());
	f.waitForFinished();
	AMLMTEST_ASSERT_TRUE(f.isCanceled());
}

TEST_F(ExtFutureTest, QTBcancel2)
{
	// Cancel from the promise side and verify the future side is canceled.
	QFutureInterface<Unit> result;

	ExtFuture<Unit> f;
	AMLMTEST_EXPECT_TRUE(f.isStarted());

	result.reportStarted();
	f = result.future();

	AMLMTEST_EXPECT_TRUE(f.isStarted());

	AMLMTEST_EXPECT_FALSE(result.isCanceled());
	f.cancel();

	AMLMTEST_EXPECT_TRUE(result.isCanceled());

	result.reportFinished();
}

/// Verify that finished futures can be canceled.
TEST_F(ExtFutureTest, QTBcancelFinishedFutures)
{
	QFutureInterface<Unit> result;

	ExtFuture<Unit> f;
	AMLMTEST_EXPECT_TRUE(f.isStarted());

	result.reportStarted();
	f = result.future();

	AMLMTEST_EXPECT_TRUE(f.isStarted());

	result.reportFinished();

	f.cancel();

	AMLMTEST_EXPECT_TRUE(result.isCanceled());
	AMLMTEST_EXPECT_TRUE(f.isCanceled());
}

/// Canceled future shouldn't propagate new results.
TEST_F(ExtFutureTest, QTBnoResultsPropAfterCancel)
{
	QFutureInterface<int> futureInterface;
	futureInterface.reportStarted();
	ExtFuture<int> f = ExtFuture<int>(futureInterface.future());

	int result = 0;
	futureInterface.reportResult(&result);
	result = 1;
	futureInterface.reportResult(&result);
	f.cancel();
	result = 2;
	futureInterface.reportResult(&result);
	result = 3;
	futureInterface.reportResult(&result);
	futureInterface.reportFinished();
	AMLMTEST_EXPECT_EQ(f.results(), QList<int>());
}

TEST_F(ExtFutureTest, QTBstatePropagation)
{
	ExtFuture<Unit> f1;
	ExtFuture<Unit> f2;

	AMLMTEST_EXPECT_TRUE(f1.isStarted());

	QFutureInterface<Unit> result;
	result.reportStarted();
	f1 = result.future();

	f2 = f1;

	AMLMTEST_EXPECT_TRUE(f2.isStarted());

	result.reportCanceled();

	AMLMTEST_EXPECT_TRUE(f2.isStarted());
	AMLMTEST_EXPECT_TRUE(f2.isCanceled());

	ExtFuture<Unit> f3 = f2;

	AMLMTEST_EXPECT_TRUE(f3.isStarted());
	AMLMTEST_EXPECT_TRUE(f3.isCanceled());

	result.reportFinished();

	AMLMTEST_EXPECT_TRUE(f2.isStarted());
	AMLMTEST_EXPECT_TRUE(f2.isCanceled());

	AMLMTEST_EXPECT_TRUE(f3.isStarted());
	AMLMTEST_EXPECT_TRUE(f3.isCanceled());
}

//
// Exception-related tests.
//

ExtFuture<Unit> createExceptionFuture()
{
	QFutureInterface<Unit> i;
	i.reportStarted();
	ExtFuture<Unit> f = ExtFuture<Unit>(i.future());

	QException e;
	i.reportException(e);
	i.reportFinished();
	return f;
}

ExtFuture<int> createExceptionResultFuture()
{
	QFutureInterface<int> i;
	i.reportStarted();
	ExtFuture<int> f = ExtFuture<int>(i.future());
	int r = 0;
	i.reportResult(r);

	QException e;
	i.reportException(e);
	i.reportFinished();
	return f;
}

class DerivedException : public QException
{
public:
	void raise() const override { throw *this; }
	DerivedException *clone() const override { return new DerivedException(*this); }
};

ExtFuture<Unit> createDerivedExceptionFuture()
{
	QFutureInterface<Unit> i;
	i.reportStarted();
	ExtFuture<Unit> f = ExtFuture<Unit>(i.future());

	DerivedException e;
	i.reportException(e);
	i.reportFinished();
	return f;
}

/// Throwing from waitForFinished().
TEST_F(ExtFutureTest, QTBexceptions1)
{
	ExtFuture<Unit> f = createExceptionFuture();
	bool caught = false;
	try
	{
		f.waitForFinished();
	}
	catch (QException &)
	{
		caught = true;
	}
	AMLMTEST_EXPECT_TRUE(caught);
}

/// Throwing from result().
TEST_F(ExtFutureTest, QTBexceptions2)
{
	ExtFuture<int> f = createExceptionResultFuture();
	bool caught = false;
	try
	{
		f.result();
	}
	catch (QException &)
	{
		caught = true;
	}
	AMLMTEST_EXPECT_TRUE(caught);
}

/// Throwing from result() on a temporary rvalue ExtFuture.
TEST_F(ExtFutureTest, QTBexceptions3)
{
	bool caught = false;
	try
	{
		createExceptionResultFuture().result();
	}
	catch (QException &)
	{
		caught = true;
	}
	AMLMTEST_EXPECT_TRUE(caught);
}

/// Throwing from results().
TEST_F(ExtFutureTest, QTBexceptions4)
{
	ExtFuture<int> f = createExceptionResultFuture();
	bool caught = false;
	try
	{
		f.results();
	}
	catch (QException &)
	{
		caught = true;
	}
	AMLMTEST_EXPECT_TRUE(caught);
}

/// Throwing from results() in a for loop.
TEST_F(ExtFutureTest, QTBexceptions5)
{
	ExtFuture<int> f = createExceptionResultFuture();
	bool caught = false;
	try
	{
		for(int e : f.results())
		{
			Q_UNUSED(e);
			AMLMTEST_EXPECT_EQ(0, 1) << "Exception not thrown from for loop";
		}
	}
	catch (QException &)
	{
		caught = true;
	}
	AMLMTEST_EXPECT_TRUE(caught);
}

/// Throwing derived exceptions from waitForFinished().
TEST_F(ExtFutureTest, QTBexceptions6)
{
	bool caught = false;
	try
	{
		createDerivedExceptionFuture().waitForFinished();
	}
	catch (QException &)
	{
		caught = true;
	}
	AMLMTEST_EXPECT_TRUE(caught);
}

/// Throwing derived exceptions from waitForFinished().
TEST_F(ExtFutureTest, QTBexceptions7)
{
	bool caught = false;
	try
	{
		createDerivedExceptionFuture().waitForFinished();
	}
	catch (DerivedException &)
	{
		caught = true;
	}
	AMLMTEST_EXPECT_TRUE(caught);
}


///
/// @name Start of our own uninspired tests.
///
/// @{

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

	ExtFuture<int> ef = make_started_only_future<int>();

//    EXPECT_EQ(ef.state(), ExtFutureState::Started | ExtFutureState::Running);

	ef.reportResult(1);

    EXPECT_EQ(ef.resultCount(), 1);
//    EXPECT_EQ(ef.get()[0], 1);
    EXPECT_EQ(ef.result(), 1);

    ef.reportResult(2);

    EXPECT_EQ(ef.resultCount(), 2);
//    EXPECT_EQ(ef.get()[1], 2);
    EXPECT_EQ(ef.resultAt(1), 2);

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

	/**
	 * @note QFuture<> behavior.
	 * The QFuture coming out of ::run() here is (Running|Started).
	 * A default construced QFuture is (Started|Canceled|Finished)
	 * I assume "Running" might not always be the case, depending on available threads.
	 */
	ExtFuture<int> f = ExtAsync::run_again([=](ExtFuture<int> rc_future) -> int {

		for(int i = 0; i<5; ++i)
		{
			// Do nothing for 1000 ms.
			TC_Sleep(1000);

			rc_future.reportResult(i);

			if(rc_future.HandlePauseResumeShouldICancel())
			{
				rc_future.reportCanceled();
				break;
			}
		}
		rc_future.reportFinished();

		return 1;
	});

	TCOUT << "Initial future state:" << state(f);

	EXPECT_TRUE(f.isStarted());
	EXPECT_TRUE(f.isRunning());
	EXPECT_FALSE(f.isCanceled());
	EXPECT_FALSE(f.isFinished());

	// ~immediately cancel the future.
	TCOUT << "CANCELING FUTURE";
//    f.cancel();
	f.reportException(ExtAsyncCancelException());
	TCOUT << "CANCELED FUTURE";

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

	TCOUT << "WAITING FOR FINISHED";
	try
	{
		// This will throw.
		f.waitForFinished();
	}
	catch(...)
	{
		TCOUT << "CAUGHT UNKNOWN EXCEPTION";
	}
	TCOUT << "FINISHED WAITING FOR FINISHED";

	EXPECT_TRUE(f.isFinished());

	AMLMTEST_COUT << "Cancelled and finished extfuture:" << state(f);

    TC_EXIT();
}

TEST_F(ExtFutureTest, MultiThenCancelBasic)
{
	TC_ENTER();

	using TypeParam = ExtFuture<int>;

	bool caught_exception = false;

	TypeParam main_future = ExtAsync::run([=](TypeParam rc_future) -> void {

		for(int i = 0; i<5; ++i)
		{
			// Do nothing for 1000 ms.
			TC_Sleep(1000);

			qfiface(rc_future).reportResult(i);

			if(ExtFuture<int>(rc_future).HandlePauseResumeShouldICancel())
			{
				qfiface(rc_future).reportCanceled();
				break;
			}
		}
		qfiface(rc_future).reportFinished();
	});

	AMLMTEST_COUT << "Initial future state:" << state(main_future);

	EXPECT_TRUE(main_future.isStarted());
	EXPECT_TRUE(main_future.isRunning());
	EXPECT_FALSE(main_future.isCanceled());
	EXPECT_FALSE(main_future.isFinished());

	// ~immediately cancel the future.
	main_future.cancel();

	/**
	 * @note QFuture<> behavior.
	 * The QFuture after this cancel() is (Running|Started|Canceled).
	 * A default construced QFuture is (Started|Canceled|Finished)
	 * I assume "Running" might not always be the case, depending on cancel-before-start or cancel-after-completion.
	 */
	AMLMTEST_COUT << "Cancelled future state:" << state(main_future);

	EXPECT_TRUE(main_future.isStarted());
	EXPECT_TRUE(main_future.isCanceled());

	// Canceling alone won't finish the extfuture.
	/// @todo This is not coming back canceled.
	EXPECT_FALSE(main_future.isFinished()) << state(main_future);

	try
	{
		// This will throw.
		main_future.waitForFinished();
	}
	catch(...)
	{
		TCOUT << "CAUGHT EXCEPTION FROM WAITFORFINISHED";
	}

	EXPECT_TRUE(main_future.isFinished());

	AMLMTEST_COUT << "Cancelled and finished extfuture:" << state(main_future);

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


/**
 * Start an ExtAsync::run() operation, attach a then(), and before it completes, report
 * an ExtAsyncCancelException() to the future returned from the then().
 * Make sure the exception propagates to the run().
 */
TEST_F(ExtFutureTest, ExtFutureThenThrow)
{
	TC_ENTER();

	SCOPED_TRACE("ExtFutureThenThrow");

	// So we can assert we're getting the same ExtFuture when we enter the run() callback.
//	ExtFuture<int> root_async_operation_future = make_started_only_future<int>();

	// Create and start the async operation.
	ExtFuture<int> root_async_operation_future = ExtAsync::run_again([=]
													  (ExtFuture<int> root_async_operation_future_copy) -> int {

		SCOPED_TRACE("In ExtAsync::run()");

		TCOUT << "STARTING ASYNC LOOP, root_async_operation_future_copy:" << root_async_operation_future_copy;

			for(int i = 0; i < 10; i++)
			{
				TCOUT << "LOOP" << i << "root_future_copy:" << root_async_operation_future_copy;
				TC_Sleep(1000);

				if(root_async_operation_future_copy.HandlePauseResumeShouldICancel())
				{
					TCOUT << "CANCELING FROM RUN() CALLBACK, upcopy state:" << root_async_operation_future_copy;
					root_async_operation_future_copy.reportCanceled();
					break;
				}
			}
			TCOUT << "LEAVING ASYNC LOOP";
			root_async_operation_future_copy.reportFinished();
			return 1;
	});
	TCOUT << "INITIAL root_async_op STATE:" << root_async_operation_future;
	AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(root_async_operation_future);

	// Set up the one and only .then().
	ExtFuture<int> final_downstream_future = root_async_operation_future.then([&](ExtFuture<int> upcopy) -> int {
//		AMLMTEST_EXPECT_EQ(upcopy, root_async_operation_future);
		TCOUT << "THEN() START, root_async_operation_future.then(), upcopy:" << upcopy;

			TCOUT << "THEN() CALLING .GET(), upcopy:" << upcopy;
				auto results = upcopy.get();

			return 5;
			;});
	TCOUT << "THEN() FUTURE:" << final_downstream_future;

	AMLMTEST_EXPECT_FALSE(final_downstream_future.isCanceled());

	// Check again, up should still not be finished or canceled.
	AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(root_async_operation_future);

	// Report an ExtAsyncCancelException() to the final future, the one returned by .then().
	TC_Sleep(5000);
	TCOUT << "CANCELING THEN(), root/final:" << root_async_operation_future << final_downstream_future;
	AMLMTEST_EXPECT_FALSE(final_downstream_future.isFinished() || final_downstream_future.isCanceled()) << final_downstream_future.state();
//	final_downstream_future.reportException(ExtAsyncCancelException());
	final_downstream_future.cancel();
	TCOUT << "CANCELED THEN(), root/final" << root_async_operation_future << final_downstream_future;

	try
	{
		TCOUT << "ROOT WAITING FOR FINISH/CANCEL/EXCEPTION:" << root_async_operation_future;
		root_async_operation_future.waitForFinished();
		TCOUT << "ROOT WAITING FINISHED" << root_async_operation_future;
	}
	catch (ExtAsyncCancelException& e)
	{
		TCOUT << "ROOT CAUGHT CANCEL" << root_async_operation_future << e.what();
	}
	catch(...)
	{
		TCOUT << "ROOT CAUGHT UNKNOWN EXCEPTION" << root_async_operation_future;
	}

	TC_Sleep(2000);

	EXPECT_TRUE(final_downstream_future.isCanceled());
	EXPECT_TRUE(root_async_operation_future.isCanceled()) << root_async_operation_future;

	TC_EXIT();
}

TEST_F(ExtFutureTest, ThenFutureDeleted)
{
	TC_ENTER();

	TC_START_RSM(rsm);
	enum
	{
		MSTART,
		MEND,
		T1STARTCB,
		T1ENDCB,
	};

	using ::testing::InSequence;
	using ::testing::Return;
	using ::testing::Eq;
	using ::testing::ReturnArg;
	using ::testing::_;

	ON_CALL(rsm, ReportResult(_))
			.WillByDefault(ReturnArg<0>());
	{
		InSequence s;
		TC_RSM_EXPECT_CALL(rsm, MSTART);
//		TC_RSM_EXPECT_CALL(rsm, T1STARTCB);
//		TC_RSM_EXPECT_CALL(rsm, T1ENDCB);
		TC_RSM_EXPECT_CALL(rsm, MEND);
	}

	rsm.ReportResult(MSTART);

	std::atomic_bool got_into_then {false};

	// Create a new future.
//	ExtFuture<int>* promise = new make_started_only_future<int>();//ExtFuture<int>();
	ExtFuture<int>* promise = new ExtFuture<int>(make_started_only_future<int>());

//	AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(*promise);

//	AMLMTEST_EXPECT_FALSE(promise.isCanceled()) << promise;

	// Attach a .then() to it.
	promise->then([&got_into_then](ExtFuture<int> upcopy){
		got_into_then = true;
		ADD_FAILURE() << "I THINK WE SHOULDNT GET IN HERE ON DELETE OF UPSTREAM... maybe";
		AMLMTEST_EXPECT_TRUE(upcopy.isFinished() | upcopy.isCanceled());
		// Immediately return.
		return 5;
	});

	// Now delete the promise out from under the then().
	delete promise;

	// Sleep/Wait a few ticks for something bad to happen.
	TC_Sleep(500);
	TC_Wait(2000);

	AMLMTEST_EXPECT_FALSE(got_into_then);

	rsm.ReportResult(MEND);

	TC_END_RSM(rsm);

	TC_EXIT();
}

TEST_F(ExtFutureTest, ExtFutureThenCancel)
{
	TC_ENTER();

	TC_START_RSM(rsm);
	enum
	{
		MSTART,
		MEND,
		T1STARTCB,
		J1CANCELED,
		T1ENDCB,
	};

	using ::testing::InSequence;
	using ::testing::Return;
	using ::testing::Eq;
	using ::testing::ReturnArg;
	using ::testing::_;

	ON_CALL(rsm, ReportResult(_))
			.WillByDefault(ReturnArg<0>());
	{
		InSequence s;
		TC_RSM_EXPECT_CALL(rsm, MSTART);
		TC_RSM_EXPECT_CALL(rsm, T1STARTCB);
		TC_RSM_EXPECT_CALL(rsm, J1CANCELED);
		TC_RSM_EXPECT_CALL(rsm, T1ENDCB);
		TC_RSM_EXPECT_CALL(rsm, MEND);
	}

	rsm.ReportResult(MSTART);

	ExtFuture<int> main_future = make_started_only_future<int>();
	QtConcurrent::run([=, &rsm, &main_future](ExtFuture<int> main_future_copy) {
		TCOUT << "IN RUN CALLBACK, main_future_copy:" << main_future_copy;
		AMLMTEST_EXPECT_EQ(main_future, main_future_copy);

		rsm.ReportResult(T1STARTCB);
		while(true)
		{
			// Wait a sec before doing anything.
			TC_Sleep(1000);
			main_future_copy.reportResult(5);
			if(main_future_copy.HandlePauseResumeShouldICancel())
			{
				rsm.ReportResult(J1CANCELED);
				main_future_copy.reportCanceled();
				break;
			}
		}
		rsm.ReportResult(T1ENDCB);
		main_future_copy.reportFinished();
	}, main_future);

	AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(main_future);

	AMLMTEST_EXPECT_FALSE(main_future.isCanceled()) << main_future;
	ExtFuture<int> down = main_future.then([=, &rsm, &main_future](ExtFuture<int> upstream_copy){

		AMLMTEST_EXPECT_EQ(upstream_copy, main_future);

		std::exception_ptr eptr; // For rethrowing.
		try
		{
			// Should never block, might throw.
			auto incoming = upstream_copy.get();
		}
		catch(ExtAsyncCancelException& e)
		{
			TCOUT << "CAUGHT CANCEL EXCEPTION:" << e.what();
		}
		catch(...)
		{
			eptr = std::current_exception();
		}
		// Do we need to rethrow?
		if(eptr)
		{
			qDb() << "rethrowing.";
			std::rethrow_exception(eptr);
		}

		AMLMTEST_EXPECT_TRUE(upstream_copy.isFinished() || upstream_copy.isCanceled());
		// Immediately return.

		TCOUT << "THEN RETURNING";
			return 5;
			;});
	EXPECT_FALSE(down.isCanceled());

	// Wait a few ticks.
	TC_Sleep(500);

	// Cancel the downstream future.
	TCOUT << "CANCELING DOWNSTREAM" << down;
//	down.reportException(ExtAsyncCancelException());
	down.cancel();
	TCOUT << "CANCELED DOWNSTREAM" << down;

	TCOUT << "WAITING TO PROPAGATE";
	TC_Sleep(2000);

	EXPECT_TRUE(down.isCanceled());
	EXPECT_TRUE(main_future.isCanceled()) << main_future;

	rsm.ReportResult(MEND);

	TC_END_RSM(rsm);

	TC_EXIT();
}

inline static void LogThreadPoolInfo(QThreadPool* tp)
{
	TCOUT << "Max thread count:" << tp->maxThreadCount();
	TCOUT << "Active thread count:" << tp->activeThreadCount();
	TCOUT << "Free thread count:" << tp->maxThreadCount() - tp->activeThreadCount();
}

TEST_F(ExtFutureTest, ExtFutureThenCancelCascade)
{
	TC_ENTER();

	auto tp = QThreadPool::globalInstance();

	TC_START_RSM(rsm);
	enum
	{
		MSTART,
		MEND,
		J1STARTCB,
		J1CANCELED,
		J1ENDCB,
		T1STARTCB,
		T1ENDCB,
	};

	using ::testing::InSequence;
	using ::testing::Return;
	using ::testing::Eq;
	using ::testing::ReturnArg;
	using ::testing::_;

	ON_CALL(rsm, ReportResult(_))
			.WillByDefault(ReturnArg<0>());
	{
		InSequence s;

		TC_RSM_EXPECT_CALL(rsm, MSTART);
		TC_RSM_EXPECT_CALL(rsm, J1STARTCB);
		TC_RSM_EXPECT_CALL(rsm, J1CANCELED);
		TC_RSM_EXPECT_CALL(rsm, J1ENDCB);
//		TC_RSM_EXPECT_CALL(rsm, T1STARTCB);
//		TC_RSM_EXPECT_CALL(rsm, T1ENDCB);
		TC_RSM_EXPECT_CALL(rsm, MEND);
	}

	std::atomic_bool ran_generator_task_callback {false};
	std::atomic_bool ran_then1_callback {false};
	std::atomic_bool ran_then2_callback {false};

	///
	/// Setup done, test starts here.
	///

	rsm.ReportResult(MSTART);

	// Log the number of free threads in the thread pool.
	LogThreadPoolInfo(tp);

	// The async generator task.  Spins forever, reporting "5" to generator_task_future until canceled.
	ExtFuture<int> generator_task_future = ExtAsync::run_again(
				[=, &ran_generator_task_callback, &ran_then1_callback, &ran_then2_callback, &rsm, &generator_task_future]
					  (ExtFuture<int> generator_task_future_copy) -> int {
		// Check the atomics.
		AMLMTEST_EXPECT_FALSE(ran_generator_task_callback);
		AMLMTEST_EXPECT_FALSE(ran_then1_callback);
		AMLMTEST_EXPECT_FALSE(ran_then2_callback);
		ran_generator_task_callback = true;

		// If we're not (Running|Started) here, something's wildly wrong.
		AMLMTEST_EXPECT_TRUE(generator_task_future_copy.isStarted());
		AMLMTEST_EXPECT_TRUE(generator_task_future_copy.isRunning());
//		TCOUT << "IN RUN CALLBACK, generator_task_future_copy:" << generator_task_future_copy;
		AMLMTEST_EXPECT_EQ(generator_task_future, generator_task_future_copy);

		rsm.ReportResult(J1STARTCB);

		while(true)
		{
			// Wait one second before doing anything.
			TC_Sleep(1000);
			// Report a result to downstream.
			generator_task_future_copy.reportResult(5);
			// Handle canceling.
			if(generator_task_future_copy.HandlePauseResumeShouldICancel())
			{
				rsm.ReportResult(J1CANCELED);
				generator_task_future_copy.reportCanceled();
				break;
			}
		}

		// We've been canceled, but not finished.
		AMLMTEST_EXPECT_TRUE(generator_task_future_copy.isCanceled());
		AMLMTEST_EXPECT_FALSE(generator_task_future_copy.isFinished());
		rsm.ReportResult(J1ENDCB);
		generator_task_future_copy.reportFinished();
		return 1;
	}
	);

	AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(generator_task_future);
	AMLMTEST_EXPECT_FALSE(generator_task_future.isCanceled()) << generator_task_future;

	// Then 1
	ExtFuture<int> downstream_then1 = generator_task_future.then([=, &ran_generator_task_callback, &ran_then1_callback, &ran_then2_callback, &rsm, &generator_task_future]
										(ExtFuture<int> upstream_future_copy) -> int {
		AMLMTEST_EXPECT_EQ(upstream_future_copy, generator_task_future);
		// Check the atomics.
		AMLMTEST_EXPECT_TRUE(ran_generator_task_callback);
		AMLMTEST_EXPECT_FALSE(ran_then1_callback);
		AMLMTEST_EXPECT_FALSE(ran_then2_callback);
		ran_then1_callback = true;

		// Should always be finished if we get in here.
		AMLMTEST_EXPECT_TRUE(upstream_future_copy.isFinished());
		// For this test, we should also be canceled.
		AMLMTEST_EXPECT_TRUE(upstream_future_copy.isCanceled());

		TCOUT << "THEN1 GETTING, future state:" << upstream_future_copy;
		QList<int> incoming = upstream_future_copy.get();

		// No try.  This should throw to down.
//		auto results_from_upstream = upcopy.results();
//		ADD_FAILURE() << "We should never get in here on a cancelation.";
		// Immediately return.
		TCOUT << "THEN1 RETURNING, future state:" << upstream_future_copy;
		return 5;
	});
	AMLMTEST_EXPECT_FALSE(downstream_then1.isCanceled());

	// Then 2
	ExtFuture<int> downstream_then2 = downstream_then1.then([=, &ran_generator_task_callback, &ran_then1_callback, &ran_then2_callback, &rsm, &downstream_then1]
									 (ExtFuture<int> upstream_future_copy) -> int {
		AMLMTEST_EXPECT_EQ(upstream_future_copy, downstream_then1);
		// Check the atomics.
		AMLMTEST_EXPECT_TRUE(ran_generator_task_callback);
		AMLMTEST_EXPECT_TRUE(ran_then1_callback);
		AMLMTEST_EXPECT_FALSE(ran_then2_callback);

		// Should always be finished if we get in here.
		AMLMTEST_EXPECT_TRUE(upstream_future_copy.isFinished());
		// For this test, we should also be canceled.
		AMLMTEST_EXPECT_TRUE(upstream_future_copy.isCanceled());
		ran_then2_callback = true;

		TCOUT << "THEN2 GETTING, future state:" << upstream_future_copy;
		QList<int> incoming = upstream_future_copy.get();

		TCOUT << "THEN2 RETURNING, future state:" << upstream_future_copy;
		return 6;
	});
	AMLMTEST_EXPECT_FALSE(downstream_then2.isCanceled());

	// Ok, both then()'s attached, less than a second before the promise sends its first result.

	// Check the number of free threads in the thread pool again.
	LogThreadPoolInfo(tp);

	// Wait a few ticks.
	TC_Sleep(1000);

	// Cancel the downstream future.
	TCOUT << "CANCELING TAIL downstream_then2:" << downstream_then2;
//	downstream_then2.cancel();
	downstream_then2.reportException(ExtAsyncCancelException());
	TCOUT << "CANCELED TAIL downstream_then2:" << downstream_then2;

	TCOUT << "WAITING FOR CANCEL TO PROPAGATE";
	TC_Sleep(3000);
	TCOUT << "CANCEL SHOULD HAVE PROPAGATED";

	AMLMTEST_EXPECT_TRUE(downstream_then2.isCanceled()) << downstream_then2;
	AMLMTEST_EXPECT_TRUE(downstream_then1.isCanceled()) << downstream_then1;
	AMLMTEST_EXPECT_TRUE(generator_task_future.isCanceled()) << generator_task_future;

	AMLMTEST_EXPECT_TRUE(ran_generator_task_callback);
	AMLMTEST_EXPECT_TRUE(ran_then1_callback);
	AMLMTEST_EXPECT_TRUE(ran_then2_callback);

	rsm.ReportResult(MEND);

	TC_END_RSM(rsm);

	TC_EXIT();
}


/**
 * Cancel the Promise side, see if the Future side detects it.
 */
TEST_F(ExtFutureTest, ExtFutureCancelPromise)
{
    TC_ENTER();

	ExtFuture<Unit> promise_side = make_started_only_future<Unit>();
	ExtFuture<Unit> future_side = make_started_only_future<Unit>();

	future_side.reportStarted();
	promise_side = future_side;

	ASSERT_FALSE(promise_side.isCanceled());
	future_side.reportCanceled();
	ASSERT_TRUE(promise_side.isCanceled());

	future_side.reportFinished();
	ASSERT_TRUE(promise_side.isCanceled());

	try
	{
		future_side.waitForFinished();
	}
	catch(...)
	{
		TCOUT << "CAUGHT";
	}
	ASSERT_TRUE(promise_side.isCanceled());

    TC_EXIT();
}

/**
 * Cancel the Future side, see if the promise side detects it.
 */
TEST_F(ExtFutureTest, ExtFutureCancelFuture)
{
    TC_ENTER();

	ExtFuture<Unit> promise_side = make_started_only_future<Unit>();
	ExtFuture<Unit> future_side = make_started_only_future<Unit>();

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
	EXPECT_FALSE(ef.isCanceled()) << ef.state();
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

	TC_START_RSM(rsm);

	SCOPED_TRACE("ThenChain");

	using ::testing::InSequence;
	using ::testing::Return;
	using ::testing::Eq;
	using ::testing::ReturnArg;
	using ::testing::_;

	ON_CALL(rsm, ReportResult(_))
			.WillByDefault(ReturnArg<0>());
	enum
	{
		MSTART,
		MEND,
		T1ENTERED,
		T2ENTERED
	};
	{
		InSequence s;

		TC_RSM_EXPECT_CALL(rsm, MSTART);
		TC_RSM_EXPECT_CALL(rsm, T1ENTERED);
		TC_RSM_EXPECT_CALL(rsm, T2ENTERED);
		TC_RSM_EXPECT_CALL(rsm, MEND);
	}

	using FutureType = ExtFuture<QString>;

	AMLMTEST_COUT << "STARTING FUTURE";
	ExtFuture<QString> future = ExtAsync::run(delayed_string_func_1, this);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	AMLMTEST_COUT << "Future created:" << future;

	rsm.ReportResult(MSTART);

	future.then([&rsm](FutureType in_future){
			SCOPED_TRACE("In then 1");

			rsm.ReportResult(T1ENTERED);
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

			rsm.ReportResult(T2ENTERED);

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

	rsm.ReportResult(MEND);

//	EXPECT_TRUE(ran_tap);
//	EXPECT_TRUE(ran_then);
//	Q_ASSERT(ran_then);

	TC_END_RSM(rsm);

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

	TCOUT << "Starting ef state:" << ef;
    ASSERT_TRUE(ef.isStarted());
    ASSERT_FALSE(ef.isCanceled());
    ASSERT_FALSE(ef.isFinished());

	TCOUT << "Attaching tap and get()";

//    async_results_from_get =
M_WARNING("TODO: This is still spinning when the test exits.");
	auto f2 = ef.tap(qApp, [=, &async_results_from_tap, &num_tap_completions](eftype ef, int begin, int end) -> void  {
			TCOUT << "IN TAP, begin:" << begin << ", end:" << end;
        for(int i = begin; i<end; i++)
        {
			TCOUT << "Pushing" << ef.resultAt(i) << "to tap list.";
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
	TC_ENTER();

	ExtFuture<int> f;
	TCOUT << "ExtFuture<int>:" << f;
	TCOUT << "ExtFuture<int>.state() 1:" << f.state();
	TCOUT << "ExtFuture<int>.state() 2:" << ExtFutureState::Started;
	TCOUT << "ExtFuture<int>.state() 3:" << toqstr<ExtFutureState::State>(f.state());

	QString str;
	QDebug dbg(&str);
//	dbg << toString(ExtFutureState::state(f));
	dbg << f.state();
//	PrintTo(str, os);
	TCOUT << "qDb():" << str;

	std::cout << "std::cout: " << f.state() << std::endl;
	std::cout << "std::cout: " << f << std::endl;

	static_assert(std::is_default_constructible<QString>::value);

	TC_EXIT();

}

/// @} // END Our own uninspired tests.
