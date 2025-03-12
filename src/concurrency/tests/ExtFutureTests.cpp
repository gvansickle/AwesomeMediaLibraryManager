/*
 * Copyright 2018, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
#define QFUTURE_TEST
#include <QtCore/qfutureinterface.h>  // For test purposes only.

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
TYPED_TEST_SUITE(ExtFutureTypedTestFixture, FutureIntTypes);

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
		  TCOUT << "IN CONTINUABLE THEN:" << result;
		  return 1;
	  })
			.then([=](){
		TCOUT << "Sleeping";
		TC_Sleep(1000);
		TCOUT << "Not Sleeping";
	});

	TC_EXIT();
}
#endif

#if 0 /// Boost::thread
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

int calculate_the_answer_to_life_the_universe_and_everything()
{
	return 42;
}

TEST_F(ExtFutureTest, DISABLED_BoostSanity)
{
	boost::packaged_task<int> pt(calculate_the_answer_to_life_the_universe_and_everything);

	boost::unique_future<int> fi=pt.get_future();

	// launch task on a thread
	boost::thread task(boost::move(pt));

	// wait for the future to be finished.
	fi.wait();

	// Make sure we wait for the thread to terminate.
	task.join();

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

int f_value = 10;

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
	// None of these .waitForFinished() calls should actually wait.  This is sort of a degenerate case,
	// and matches QFuture<T>'s behavior.
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
	AMLMTEST_EXPECT_TRUE(intFuture2.isCanceled());
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

	result.reportFinished(&f_value);

	AMLMTEST_EXPECT_TRUE(intFuture.isStarted());
	AMLMTEST_EXPECT_TRUE(intFuture.isFinished());

	int e = intFuture.result();

	AMLMTEST_EXPECT_TRUE(intFuture.isStarted());
	AMLMTEST_EXPECT_TRUE(intFuture.isFinished());
	AMLMTEST_EXPECT_FALSE(intFuture.isCanceled());

	AMLMTEST_ASSERT_EQ(e, f_value);
	intFuture.waitForFinished();

	IntResult intAlgo;
	intFuture = intAlgo.run();
	ExtFuture<int> intFuture2(intFuture);
	AMLMTEST_ASSERT_EQ(intFuture.result(), f_value);
	AMLMTEST_ASSERT_EQ(intFuture2.result(), f_value);
	intFuture.waitForFinished();

#if 0 /// @todo
	UnitResult a;
	a.run().waitForFinished();
#endif
}

template <typename T>
void QTBtestRefCounting()
{
#if 0 // BROKEN BY UPGRADE
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
#endif
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

// For the next test.  This is a dummy-up of what we're doing with ExtFuture<T> and QFuture<T>.
static std::string s_test_results_str;
template<class T>
class Base
{
public:
	~Base()
	{
		s_test_results_str += "~B";
	}
};

template <class T>
class Derived : public Base<T>
{
public:
	virtual ~Derived()
	{
		s_test_results_str += "~D";
	}

	// Extra params.
	int m_some_int;
	std::string m_some_string {"Hello From Derived"};
};

TEST_F(ExtFutureTest, InheritingFromNonvirtualDestructor)
{
	TC_ENTER();

	s_test_results_str.clear();

	/// Smart pointers are smart, this works correctly.
	{
		std::shared_ptr<Base<int>> tmp = std::make_shared<Derived<int>>();
	}
	TCOUT << "shared_ptr<Base<T>> Results:" << s_test_results_str << "\n";
	EXPECT_STREQ("~D~B", s_test_results_str.c_str());

	/// Raw pointers are not smart, this only calls the Base<T> destructor.
	s_test_results_str.clear();
	{
		Base<int>* tmp = new Derived<int>();
		delete tmp;
	}
	TCOUT << "Base<T>* Results:" << s_test_results_str << "\n";
	EXPECT_STREQ("~B", s_test_results_str.c_str());

	/// ????????????????? Raw pointers are not smart, this only calls the Base<T> destructor.
	s_test_results_str.clear();
	{
		Derived<int>* tmp = new Derived<int>();
		delete tmp;
	}
	TCOUT << "Derived<T>* Results:" << s_test_results_str << "\n";
	EXPECT_STREQ("~D~B", s_test_results_str.c_str());

	TC_EXIT();
}

TEST_F(ExtFutureTest, ReadyFutureCompletion)
{
    TC_ENTER();
#if 0
	AMLMTEST_SCOPED_TRACE("In ReadyFutureCompletion");

    /// @note Important safety tip: nL and nLL are different sizes on Windows vs. Linux.
    /// <cstdint> to the rescue.
	ExtFuture<int64_t> ef = ExtAsync::make_ready_future(INT64_C(25));

	// Make sure it's really ready.
	TCOUT << "ExtFuture state:" << state(ef);
	AMLMTEST_EXPECT_TRUE(ef.isStarted());
	AMLMTEST_EXPECT_TRUE(ef.isFinished());
	AMLMTEST_EXPECT_FALSE(ef.isCanceled());

    QList<int64_t> results = ef.get();

    EXPECT_EQ(ef.state(), ExtFutureState::Started | ExtFutureState::Finished);

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], 25L);
#endif
    TC_EXIT();
}

TEST_F(ExtFutureTest, FutureSingleThread)
{
    TC_ENTER();
#if 0
	ExtFuture<int> ef = ExtAsync::make_started_only_future<int>();

//    EXPECT_EQ(ef.state(), ExtFutureState::Started | ExtFutureState::Running);

	ef.reportResult(1);

    EXPECT_EQ(ef.resultCount(), 1);
//    EXPECT_EQ(ef.get()[0], 1);
    EXPECT_EQ(ef.result(), 1);

    ef.reportResult(2);

    EXPECT_EQ(ef.resultCount(), 2);
//    EXPECT_EQ(ef.get()[1], 2);
    EXPECT_EQ(ef.resultAt(1), 2);
#endif
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


TEST_F(ExtFutureTest, InternalExceptionProp)
{
	TC_ENTER();
#if 0
	std::exception_ptr eptr;
	ExtFuture<int> f0 = ExtAsync::make_started_only_future<int>();

	/// @note State here is coming back:
	/// BEFORE f0: ExtFuture<T>( id= 33 "[unknown]" state: QFlags<ExtFutureState::State>(Running|Started) hasException(): false , resultCount(): 0 )
	qIn() << "BEFORE f0:" << f0;

	try
	{
		throw QException();
	}
	catch(...)
	{
		// Capture the exception.
		eptr = std::current_exception();
	}

	ManagedExtFutureWatcher_detail::propagate_eptr_to_future(eptr, f0);

	TC_Wait(1000);

	/// @note We're getting the following state here:
	/// ExtFuture<T>( id= 51 "[unknown]" state: QFlags<ExtFutureState::State>(Running|Started|Canceled) hasException(): true , resultCount(): 0 )
	/// Note the "Running".  Adding a TC_Wait() doesn't seem to make a difference.
	qIn() << "AFTER f0:" << f0;

	EXPECT_TRUE(f0.has_exception());
	EXPECT_TRUE(f0.isStarted());
	EXPECT_TRUE(f0.isFinished());
	EXPECT_TRUE(f0.isCanceled());

	// Trip it and see if it's the exception we threw.
	try
	{
		f0.get_first();
	}
	catch(QException& e)
	{
		SUCCEED();
	}
	catch(...)
	{
		ADD_FAILURE() << "Unexpected exception type";
	}
#endif
	TC_EXIT();
}

TEST_F(ExtFutureTest, InternalTriggerExceptionAndProp)
{
	TC_ENTER();
#if 0
	ExtFuture<int> upstream_f = ExtAsync::make_exceptional_future<int>(QException());
	ExtFuture<int> downstream_f = ExtAsync::make_started_only_future<int>();


	std::exception_ptr eptr;

	/// @note State here is coming back:
	/// BEFORE f0: ExtFuture<T>( id= 33 "[unknown]" state: QFlags<ExtFutureState::State>(Running|Started) hasException(): false , resultCount(): 0 )
	qIn() << "BEFORE futures:" << M_ID_VAL(upstream_f) << M_ID_VAL(downstream_f);

	ManagedExtFutureWatcher_detail::trigger_exception_and_propagate(upstream_f, downstream_f);

	TC_Wait(1000);

	/// @note We're getting the following state here:
	/// ExtFuture<T>( id= 51 "[unknown]" state: QFlags<ExtFutureState::State>(Running|Started|Canceled) hasException(): true , resultCount(): 0 )
	/// Note the "Running".  Adding a TC_Wait() doesn't seem to make a difference.
	qIn() << "AFTER futures:" << M_ID_VAL(upstream_f) << M_ID_VAL(downstream_f);

	EXPECT_TRUE(downstream_f.has_exception());
	EXPECT_TRUE(downstream_f.isStarted());
	EXPECT_TRUE(downstream_f.isFinished());
	EXPECT_TRUE(downstream_f.isCanceled());

	// Trip it and see if it's the exception we threw.
	try
	{
		downstream_f.get_first();
	}
	catch(QException& e)
	{
		SUCCEED();
	}
	catch(...)
	{
		ADD_FAILURE() << "Unexpected exception type";
	}
#endif
	TC_EXIT();
}

/**
 * Test basic cancel properties.
 */
TEST_F(ExtFutureTest, CancelBasic)
{
    TC_ENTER();
#if 0
	/**
	 * @note QFuture<> behavior.
	 * The QFuture coming out of ::run() here is (Running|Started).
	 * A default construced QFuture is (Started|Canceled|Finished)
	 * I assume "Running" might not always be the case, depending on available threads.
	 */
	ExtFuture<int> f0 = ExtAsync::qthread_async_with_cnr_future([=](ExtFuture<int> rc_future) -> void {
		// This will spin for 5000 ticks.
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
	});
	f0.setName("f0");

	TCOUT << "Initial future f0 state:" << f0;

	EXPECT_TRUE(f0.isStarted());
	EXPECT_TRUE(f0.isRunning());
	EXPECT_FALSE(f0.isCanceled());
	EXPECT_FALSE(f0.isFinished());

	// ~immediately cancel the future.
	TCOUT << "CANCELING FUTURE: " << f0;
	f0.cancel();
	TCOUT << "CANCELED FUTURE: " << f0;

	/**
	 * @note QFuture<> behavior.
	 * The QFuture after this cancel() is (Running|Started|Canceled).
	 * A default construced QFuture is (Started|Canceled|Finished)
	 * I assume "Running" might not always be the case, depending on cancel-before-start or cancel-after-completion.
	 */
	TCOUT << "Cancelled future state:" << f0;

	EXPECT_TRUE(f0.isStarted());
	EXPECT_TRUE(f0.isCanceled());

	// .cancel() alone doesn't seem to finish a QFuture<>, or at least our ExtFuture<>.
	// So ef.cancel() actually .reportsFinished() as well.
	EXPECT_TRUE(f0.isFinished()) << f0;

	try
	{
		TCOUT << "WAITING FOR FINISHED";
		// This should throw because we reported an exception above.
		f0.waitForFinished();
	}
	catch(ExtAsyncCancelException& e)
	{
		TCOUT << "CAUGHT CANCEL EXCEPTION:" << e.what();
	}
	catch(...)
	{
		TCOUT << "CAUGHT UNKNOWN EXCEPTION";
	}
	TCOUT << "FINISHED WAITING FOR FINISHED";

	EXPECT_TRUE(f0.isFinished());

	TCOUT << "Cancelled and finished extfuture: " << f0;
#endif
    TC_EXIT();
}

TEST_F(ExtFutureTest, MultiThenCancelBasic)
{
	TC_ENTER();
#if 0
	using TypeParam = ExtFuture<int>;

	bool caught_exception = false;

	TypeParam main_future = ExtAsync::qthread_async_with_cnr_future([=](TypeParam rc_future) -> void {

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
	});

	TCOUT << "Initial future state:" << state(main_future);

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
	TCOUT << "Canceled future state:" << main_future;

	EXPECT_TRUE(main_future.isStarted());
	EXPECT_TRUE(main_future.isCanceled());

	// Canceling alone won't finish a QFuture<>, it will an ExtFuture<>.
	EXPECT_TRUE(main_future.isFinished()) << main_future;

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

	TCOUT << "Cancelled and finished extfuture:" << state(main_future);
#endif
	TC_EXIT();
}

TEST_F(ExtFutureTest, MultiThenCancelBasic2)
{
	TC_ENTER();
#if 0
	bool caught_exception = false;

	ExtFuture<int> main_future = ExtAsync::qthread_async_with_cnr_future([=](ExtFuture<int> rc_future) -> void {
		// Spin for 5 secs.
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
	})
	.then([](ExtFuture<int> up){
		TCOUT << "RAN THEN1, up:" << up;
	})
	.then([](ExtFuture<Unit> up){
		TCOUT << "RAN THEN2, up:" << up;
	})
	.then([](ExtFuture<Unit> up){
		TCOUT << "RAN THEN3, up:" << up;
		return 25;
	});

	TCOUT << "Initial future state:" << main_future;

	EXPECT_TRUE(main_future.isStarted());
	EXPECT_TRUE(main_future.isRunning());
	EXPECT_FALSE(main_future.isCanceled());
	EXPECT_FALSE(main_future.isFinished());

	// ~immediately cancel the returned future.
	main_future.cancel();

	// Wait for it to propagate.
	TC_Wait(1000);

	/**
	 * @note QFuture<> behavior.
	 * The QFuture after this cancel() is (Running|Started|Canceled).
	 * A default construced QFuture is (Started|Canceled|Finished)
	 * I assume "Running" might not always be the case, depending on cancel-before-start or cancel-after-completion.
	 */
	TCOUT << "Cancelled future state:" << main_future;

	EXPECT_TRUE(main_future.isStarted()) << main_future;
	EXPECT_TRUE(main_future.isCanceled()) << main_future;
	// Should be finished by now.
	EXPECT_TRUE(main_future.isFinished()) << main_future;
	EXPECT_FALSE(main_future.isRunning()) << main_future;

	// Canceling alone won't finish the extfuture.
	/// @note That depends on how good our implementation is.
	/// @todo This is not coming back canceled.
	EXPECT_TRUE(main_future.isFinished()) << main_future;

	try
	{
		// This may or may not throw, depending on how we've implemented cancelation propagation.
		/// @note Currently this doesn't throw.
		main_future.waitForFinished();
		EXPECT_TRUE(main_future.isCanceled());
		EXPECT_TRUE(main_future.isFinished());
	}
	catch(...)
	{
		TCOUT << "CAUGHT EXCEPTION FROM WAITFORFINISHED";
		ADD_FAILURE() << "SHOULDNT HAVE THROWN";
	}

	EXPECT_TRUE(main_future.isFinished());

	TCOUT << "Cancelled and finished extfuture:" << main_future;
#endif
	TC_EXIT();
}

TYPED_TEST(ExtFutureTypedTestFixture, PExceptionBasic)
{
	TC_ENTER();
#if 0
	bool caught_exception = false;

	TypeParam main_future;

	main_future = ExtAsync::qthread_async([=](int) -> int {
		TC_Sleep(1000);
		TCOUT << "Throwing exception from other thread";
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
		TCOUT << "Caught exception";
		caught_exception = true;
	}

	AMLMTEST_EXPECT_FUTURE_POST_EXCEPTION(main_future);

	AMLMTEST_ASSERT_TRUE(caught_exception);
#endif
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
#if 0
	SCOPED_TRACE("ExtFutureThenThrow");

	// So we can assert we're getting the same ExtFuture when we enter the run() callback.
//	ExtFuture<int> root_async_operation_future = ExtAsync::make_started_only_future<int>();

	// Create and start the async operation.
	ExtFuture<int> root_async_operation_future =
//			ExtAsync::run_again(
			ExtAsync::qthread_async_with_cnr_future(
				[=](ExtFuture<int> root_async_operation_future_copy) /*-> int*/ {

		SCOPED_TRACE("In ExtAsync::run()");

		TCOUT << "STARTING ASYNC LOOP, root_async_operation_future_copy:" << root_async_operation_future_copy;
		// Loop for 10 secs.
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
//			return 1;
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

	EXPECT_TRUE(final_downstream_future.isCanceled()) << final_downstream_future;
	EXPECT_TRUE(root_async_operation_future.isCanceled()) << root_async_operation_future;
#endif
	TC_EXIT();
}

TEST_F(ExtFutureTest, ThenFutureDeleted)
{
	TC_ENTER();
#if 0
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
//	ExtFuture<int>* promise = new ExtAsync::make_started_only_future<int>();//ExtFuture<int>();
	ExtFuture<int>* promise = new ExtFuture<int>(ExtAsync::make_started_only_future<int>());

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
#endif
	TC_EXIT();
}


TEST_F(ExtFutureTest, ParallelThens)  // NOLINT
{
	TC_ENTER();
#if 0
	std::atomic_bool then1, then2;

	ExtFuture<int> f0 = ExtAsync::qthread_async_with_cnr_future([&](ExtFuture<int> cmdresp_future) {
			TC_Sleep(1000);
			cmdresp_future.reportResult(25);
	});
	f0.setName("f0");

	auto f1 = f0.then([&](ExtFuture<int> dummy){
		then1 = true;
	});
	f1.setName("f1");
	auto f2 = f0.then([&](ExtFuture<int> dummy){
		then2 = true;
	});
	f2.setName("f2");

	// Wait for the thens to finish.
	f1.waitForFinished();
	f2.waitForFinished();

	EXPECT_TRUE(then1);
	EXPECT_TRUE(then2);

	EXPECT_TRUE(f1.isFinished());
	EXPECT_TRUE(f2.isFinished());
#endif
	TC_EXIT();
}

#if 0
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

	ExtFuture<int> main_future = ExtAsync::qthread_async_with_cnr_future([=, &rsm, &main_future](ExtFuture<int> main_future_copy) {
		TCOUT << "IN RUN CALLBACK, main_future_copy:" << main_future_copy;
		AMLMTEST_EXPECT_EQ(main_future, main_future_copy);

		rsm.ReportResult(T1STARTCB);
		while(true)
		{
			// Wait a sec before doing anything.
			TC_Sleep(1000);
			TCOUT << "::RUN() REPORTING RESULT(5)";
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
		TCOUT << "LEAVING MAING THREAD, MAIN_FUTURE_COPY:" << main_future_copy;
	});

	AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(main_future);

	AMLMTEST_EXPECT_FALSE(main_future.isCanceled()) << main_future;
	ExtFuture<int> down = main_future.then([=, &rsm, &main_future](ExtFuture<int> upstream_copy){

		AMLMTEST_EXPECT_EQ(upstream_copy, main_future);

		try
		{
			// Should never block, might throw.
			auto incoming = upstream_copy.get();
		}
		catch(...)
		{
			Q_ASSERT(0);
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
	down.reportFinished();
	TCOUT << "CANCELED DOWNSTREAM" << down;

	TCOUT << "WAITING TO PROPAGATE";
	TC_Wait(2000);
//	TC_Sleep(2000);

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
	ExtFuture<int> generator_task_future =
//			ExtAsync::run(
			ExtAsync::qthread_async_with_cnr_future(
				[=, &ran_generator_task_callback, &ran_then1_callback, &ran_then2_callback, &rsm, &generator_task_future]
					  (ExtFuture<int> generator_task_future_copy) -> void {
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

		// We've been canceled, which should also mean we're finished.
		AMLMTEST_EXPECT_TRUE(generator_task_future_copy.isCanceled()) << generator_task_future_copy;
		AMLMTEST_EXPECT_TRUE(generator_task_future_copy.isFinished()) << generator_task_future_copy;
		rsm.ReportResult(J1ENDCB);
		// Report that this callback is finished.
		generator_task_future_copy.reportFinished();
	});
	generator_task_future.setName("generator_task_future");

	AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(generator_task_future);
	AMLMTEST_EXPECT_FALSE(generator_task_future.isCanceled()) << generator_task_future;

	// Then 1
	ExtFuture<int> downstream_then1 = generator_task_future.then([=, &ran_generator_task_callback, &ran_then1_callback, &ran_then2_callback, &rsm, &generator_task_future]
										(ExtFuture<int> upstream_future_copy) -> int {
		AMLMTEST_EXPECT_EQ(upstream_future_copy, generator_task_future);
		// Should never be not ready.
		EXPECT_TRUE(upstream_future_copy.is_ready());

		// Check the atomics.
		AMLMTEST_EXPECT_TRUE(ran_generator_task_callback) << "FAIL: Generator task never ran";
		AMLMTEST_EXPECT_FALSE(ran_then1_callback);
		/// @todo This is coming back as true on a cancel. ???
		AMLMTEST_EXPECT_FALSE(ran_then2_callback);

		// Do a normal .get().
		TCOUT << "CALLING GET()";
		upstream_future_copy.get();

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
	downstream_then1.setName("downstream_then1");
	AMLMTEST_EXPECT_FALSE(downstream_then1.isCanceled());

	// Then 2
	ExtFuture<int> downstream_then2 = downstream_then1.then([=, &ran_generator_task_callback, &ran_then1_callback, &ran_then2_callback, &rsm, &downstream_then1]
									 (ExtFuture<int> upstream_future_copy) -> int {
		AMLMTEST_EXPECT_EQ(upstream_future_copy, downstream_then1);

		// Should never be not ready.
		EXPECT_TRUE(upstream_future_copy.is_ready());

		// Check the atomics.
		AMLMTEST_EXPECT_TRUE(ran_generator_task_callback);
		// On a cancel, we may have never run the upstream callback.
		// In this test, f0 cycles forever at a 1000-tick rate unless canceled, so we shouldn't
		// have ever gotten into then1.
		AMLMTEST_EXPECT_FALSE(ran_then1_callback);
		AMLMTEST_EXPECT_FALSE(ran_then2_callback);

		// Should always be finished if we get in here.
		AMLMTEST_EXPECT_TRUE(upstream_future_copy.isFinished());
		// For this test, we should also be canceled.
		AMLMTEST_EXPECT_TRUE(upstream_future_copy.isCanceled());

		// Do a normal .get().
		TCOUT << "CALLING GET()";
		upstream_future_copy.get();

		ran_then2_callback = true;

		TCOUT << "THEN2 GETTING, future state:" << upstream_future_copy;
		QList<int> incoming = upstream_future_copy.get();

		TCOUT << "THEN2 RETURNING, future state:" << upstream_future_copy;
		return 6;
	});
	downstream_then2.setName("ds_then2");
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
	downstream_then2.reportFinished();
	TCOUT << "CANCELED TAIL downstream_then2:" << downstream_then2;

	TCOUT << "WAITING FOR CANCEL TO PROPAGATE";
	TC_Sleep(6000);
	TCOUT << "CANCEL SHOULD HAVE PROPAGATED";

	AMLMTEST_EXPECT_TRUE(downstream_then2.isCanceled()) << downstream_then2;
	AMLMTEST_EXPECT_TRUE(downstream_then1.isCanceled()) << downstream_then1;
	AMLMTEST_EXPECT_TRUE(generator_task_future.isCanceled()) << generator_task_future;

	AMLMTEST_EXPECT_TRUE(ran_generator_task_callback);
	AMLMTEST_EXPECT_FALSE(ran_then1_callback);
	AMLMTEST_EXPECT_FALSE(ran_then2_callback);

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

	ExtFuture<Unit> promise_side = ExtAsync::make_started_only_future<Unit>();
	ExtFuture<Unit> future_side = ExtAsync::make_started_only_future<Unit>();

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

	ExtFuture<Unit> promise_side = ExtAsync::make_started_only_future<Unit>();
	ExtFuture<Unit> future_side = ExtAsync::make_started_only_future<Unit>();

	EXPECT_TRUE(future_side.isStarted());

    promise_side.reportStarted();
    future_side = promise_side;

	EXPECT_TRUE(future_side.isStarted());

	EXPECT_FALSE(promise_side.isCanceled());

	// Cancel future.
	TCOUT << "CANCELING FUTURE: " << future_side;
	future_side.cancel();
	future_side.wait();

	EXPECT_TRUE(promise_side.isCanceled());

    promise_side.reportFinished();

    TC_EXIT();
}


template <class FutureType, class TestFixtureType>
QList<int> results_test(int startval, int iterations, TestFixtureType* fixture)
{
    SCOPED_TRACE("In results_test");

	TCOUT << "START GENERATOR";

    FutureType f = async_int_generator<FutureType>(startval, iterations, fixture);
//    EXPECT_TRUE(f.isStarted());
//    EXPECT_FALSE(f.isFinished());

	TCOUT << "START RESULTS()" << ExtFutureState::state(f);
    QList<int> retval = f.results();
	TCOUT << "END RESULTS():" << ExtFutureState::state(f);

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

	TCOUT << "Starting ef state:" << ExtFutureState::state(ef);
	EXPECT_TRUE(ef.isStarted());
	EXPECT_FALSE(ef.isCanceled());
	EXPECT_FALSE(ef.isFinished());

	QList<int> expected_results {1,2,3,4,5,6};


	TCOUT << "Attaching tap and get()";

	FutureType f2 = make_default_future<FutureType, int>();
	EXPECT_TRUE(f2.isStarted());
	EXPECT_FALSE(f2.isCanceled());
	EXPECT_FALSE(f2.isFinished());

	if constexpr (false)//!std::is_same_v<QFuture<int>, FutureType>)
	{
		M_WARNING("TODO: This is still spinning when the test exits.");
		f2 = ef.tap(qApp, [=, &async_results_from_tap](FutureType ef, int begin, int end) mutable {
			TCOUT << "IN TAP, begin:" << begin << ", end:" << end;
			for(int i = begin; i<end; i++)
			{
				TCOUT << "Pushing" << ef.resultAt(i) << "to tap list.";
				async_results_from_tap.push_back(ef.resultAt(i));
				num_tap_completions++;
			}
		});
	}
	else
	{
		QtConcurrent::run([=, &async_results_from_tap, &num_tap_completions](FutureType ef, FutureType f2){
			TCOUT << "TAP: START TAP RUN(), ef:" << state(ef) << "f2:" << state(f2);

			if(true /* Roll our own */)
			{
				int i = 0;

				while(true)
				{
					TCOUT << "TAP: Waiting for next result";
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
						TCOUT << "NO NEW RESULTS, BREAKING, ef:" << state(ef);
						break;
					}

					// Copy over the new results
					for(; i < result_count; ++i)
					{
						TCOUT << "TAP: Next result available at i = " << i;

						int the_next_val = ef.resultAt(i);
						async_results_from_tap.append(the_next_val);
						f2.d.reportResult(the_next_val);
						num_tap_completions++;

						// Make sure we don't somehow get here too many times.
						AMLMTEST_EXPECT_LT(i, iterations);
					}
				}

				TCOUT << "LEFT WHILE(!Finished) LOOP, ef state:" << state(ef);

				// Check final state.  We know it's at least Finished.
				/// @todo Could we be Finished here with pending results?
				/// Don't care as much on non-Finished cases.
				if(ef.isCanceled())
				{
					TCOUT << "TAP: ef cancelled:" << state(ef);
					/// @todo PROPAGATE
				}
				else if(ef.isFinished())
				{
					TCOUT << "TAP: ef finished:" << state(ef);
					/// @todo PROPAGATE
				}
				else
				{
					/// @todo Exceptions.
					TCOUT << "NOT FINISHED OR CANCELED:" << state(ef);
				}
			}
			else if(false /* Use Java-like iterator */)
			{
				TCOUT << "Starting Java-like iterator";

				QFutureIterator<int> fit(ef);

				TCOUT << "Created Java-like iterator";

				while(fit.hasNext())
				{
					TCOUT << "TAP: GOT hasNext:" << state(ef);
					int the_next_val = fit.next();
					TCOUT << "TAP: GOT RESULT:" << the_next_val;
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
					TCOUT << "GOT RESULT:" << *cit;
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
				TCOUT << "TAP: PENDING ON ALL RESULTS";
				QVector<int> results = ef.results().toVector();
				TCOUT << "TAP: GOT RESULTS:" << results;
				AMLMTEST_ASSERT_EQ(results.size(), iterations);
				f2.d.reportResults(results); //ef.results().toVector());
				async_results_from_tap.append(results.toList()); //ef.results());
				num_tap_completions += ef.resultCount();
			}

			AMLMTEST_ASSERT_TRUE(ef.isFinished());

			f2.d.reportFinished();
			TCOUT << "EXIT TAP RUN(), ef:" << state(ef) << "resultCount:" << ef.resultCount()
						  << "f2:" << state(f2) << "resultCount:" << f2.resultCount();
			},
		ef, f2);
	}
	TCOUT << "BEFORE WAITING FOR results()" << state(f2);

	async_results_from_get = f2.results();

	TCOUT << "AFTER WAITING FOR results()" << state(f2);

	// .results() above should block until f2 is finished.
	AMLMTEST_EXPECT_TRUE(f2.isFinished());
	// If f2 is finished, ef must have finished.
	AMLMTEST_EXPECT_TRUE(ef.isFinished());

	// This shouldn't do anything, should already be finished.
	ef.waitForFinished();

	TCOUT << "Post .tap().get(), extfuture:" << ExtFutureState::state(ef);

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
		TCOUT << "HIT HasFatalFailure";
		return;
	}

	TC_EXIT();
}

TEST_F(ExtFutureTest, ExtFutureSingleThen)
{
	qDb() << "ENTERING TEST";

	TC_ENTER();

	using eftype = ExtFuture<int>;

	std::atomic_int num_then_completions {0};
	QList<int> async_results_from_then, async_results_from_get;


	QList<int> expected_results {1,2,3,4,5,6};
	eftype root_future = async_int_generator<eftype>(1, 6, this);
	root_future.setName("root_future");

	TCOUT << "Starting root_future state:" << root_future;
	EXPECT_TRUE(root_future.isStarted());
	EXPECT_FALSE(root_future.isCanceled());
	EXPECT_FALSE(root_future.isFinished());

	TCOUT << "Attaching then() to root_future:" << root_future;

	ExtFuture<int> f2 = root_future.then([=, &async_results_from_then, &num_then_completions](eftype root_future_copy) -> int  {
			TCOUT << "IN THEN, future:" << root_future_copy;
			AMLMTEST_EXPECT_TRUE(root_future_copy.isFinished());
			TCOUT << "CALLING GET ON ROOT_FUTURE:" << root_future_copy;
			async_results_from_then = root_future_copy.get();
			num_then_completions++;
			return 5;
	});
	f2.setName("f2");

	AMLMTEST_EXPECT_TRUE(f2.isStarted());
	EXPECT_TRUE(f2.isRunning());
	AMLMTEST_EXPECT_FALSE(f2.isCanceled());
	AMLMTEST_EXPECT_FALSE(f2.isFinished());

	TCOUT << "BEFORE WAITING FOR THEN()" << f2;

	// Block waiting on the results.
	async_results_from_get = f2.results();

	TCOUT << "AFTER WAITING FOR THEN()" << f2; /// @todo Getting here before the .then() starts.

	EXPECT_EQ(async_results_from_get.size(), 1);
	EXPECT_EQ(async_results_from_get[0], 5);

	EXPECT_TRUE(root_future.isFinished());
	EXPECT_EQ(num_then_completions, 1);

	// .get() above should block.
	EXPECT_TRUE(root_future.isFinished());

	// This shouldn't do anything, should already be finished.
	root_future.waitForFinished();

	TCOUT << "Post .tap().get(), extfuture:" << root_future;

	EXPECT_TRUE(root_future.isStarted());
	EXPECT_FALSE(root_future.isCanceled()) << root_future;
	EXPECT_TRUE(root_future.isFinished());

	EXPECT_EQ(async_results_from_then.size(), 6);
	EXPECT_EQ(async_results_from_then, expected_results);

	EXPECT_TRUE(root_future.isFinished());

	qDb() << "EXITING TEST";

	TC_EXIT();
}

TEST_F(ExtFutureTest, ThenChain)
{
	TC_ENTER();

	std::atomic_bool ran_then_1 = false;
	std::atomic_bool ran_then_2 = false;

	using FutureType = ExtFuture<QString>;

	TCOUT << "STARTING FUTURE";
	ExtFuture<QString> future = ExtAsync::qthread_async_with_cnr_future([=](ExtFuture<QString> retf){
		auto retval = delayed_string_func_1(this);
			retf.reportResult(retval);
			retf.reportFinished();
	});

	ASSERT_TRUE(future.isStarted());
//	ASSERT_FALSE(future.isFinished());

	TCOUT << "Future created:" << future;

	future.then([&ran_then_1](FutureType in_future) {

			ran_then_1 = true;
			return 1;
		;})
		.then([&](ExtFuture<int> in_future) {

			EXPECT_TRUE(ran_then_1);
			ran_then_2 = true;

			EXPECT_TRUE(in_future.isStarted());
			EXPECT_TRUE(in_future.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
			EXPECT_FALSE(in_future.isRunning());

			auto val = in_future.result();

			EXPECT_EQ(val, 1);

//			TCOUT << "in then(), extfuture:" << tostdstr(extfuture.qtget_first());
//			EXPECT_EQ(in_future.qtget_first(), QString("delayed_string_func_1() output"));
//			EXPECT_FALSE(ran_then);
//			ran_then = true;

//			rsm.ReportResult(T2ENTERED);

			return QString("Then Called");
	})/*.test_tap([&](auto ef){
		TCOUT << "IN TEST_TAP";
		wait_result = ef.result();
		EXPECT_TRUE(wait_result[0] == QString("Then Called"));
	})*/.wait();

//    TCOUT << "after wait(): " << future.state().toString();
//    ASSERT_EQ(wait_result, QString("Then Called"));

	future.waitForFinished();
	EXPECT_TRUE(future.isStarted());
	EXPECT_FALSE(future.isRunning());
	EXPECT_TRUE(future.isFinished());

	EXPECT_TRUE(ran_then_1);
	EXPECT_TRUE(ran_then_2);

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
	EXPECT_TRUE(ef.isStarted());
	EXPECT_FALSE(ef.isCanceled());
	EXPECT_FALSE(ef.isFinished());

	TCOUT << "Attaching stap to ef:" << ef;

	auto f2 = ef.stap(/*qApp,*/ [=, &async_results_from_tap, &num_tap_completions](eftype ef, int begin, int end) -> void  {
			TCOUT << "IN TAP, begin:" << begin << ", end:" << end;
        for(int i = begin; i<end; i++)
        {
			TCOUT << "Pushing" << ef.resultAt(i) << "to tap list.";
            async_results_from_tap.push_back(ef.resultAt(i));
            num_tap_completions++;
        }
	}); // Will block on f2 below.
	f2.setName("f2");

	AMLMTEST_EXPECT_TRUE(f2.isStarted());
	AMLMTEST_EXPECT_FALSE(f2.isCanceled());
	AMLMTEST_EXPECT_FALSE(f2.isFinished());

	TCOUT << "BEFORE WAITING FOR GET():" << f2;

	async_results_from_get = f2.results();

	TCOUT << "AFTER WAITING FOR GET():" << f2;

    EXPECT_TRUE(ef.isFinished());
    EXPECT_EQ(num_tap_completions, 6);

    // .get() above should block.
    EXPECT_TRUE(ef.isFinished());

    // This shouldn't do anything, should already be finished.
    ef.waitForFinished();

	TCOUT << "Post .tap().get(), extfuture:" << ef.state();

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

	/// Tests of the stream operators.
//	ExtFuture<int> f;
//	TCOUT << "ExtFuture<int>:" << f;
//	TCOUT << "ExtFuture<int>.state() 1:" << f.state();
//	TCOUT << "ExtFuture<int>.state() 2:" << ExtFutureState::Started;
//	TCOUT << "ExtFuture<int>.state() 3:" << toqstr<ExtFutureState::State>(f.state());

//	QString str;
//	QDebug dbg(&str);
////	dbg << toString(ExtFutureState::state(f));
//	dbg << f.state();
////	PrintTo(str, os);
//	TCOUT << "qDb():" << str;

//	std::cout << "std::cout: " << f.state() << std::endl;
//	std::cout << "std::cout: " << f << std::endl;

//	static_assert(std::is_default_constructible<QString>::value);

	TC_EXIT();

}

#endif

/// @} // END Our own uninspired tests.
