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


/// Types for gtest's "Typed Test" support.
using FutureIntTypes = ::testing::Types<QFuture<int>, ExtFuture<int>>;
TYPED_TEST_CASE(ExtFutureTypedTestFixture, FutureIntTypes);

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

	AMLMTEST_SCOPED_TRACE("");

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

    TC_DONE_WITH_STACK();
    TC_EXIT();
}

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

	streaming_tap_test<TypeParam>(1, 6, this);

	if (::testing::Test::HasFatalFailure())
	{
		AMLMTEST_COUT << "HIT HasFatalFailure";
		return;
	}

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

//	ExtFutureWatcher<int> efw(nullptr, nullptr);
//	efw.then([=](){
//		qDb() << "GOT EFW THEN() CALLBACK / future finished:" << ef.state();
//	});
//	efw.setFuture(ef);


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

//	ExtFutureWatcher<int> f2w(nullptr, nullptr);
//	f2w.then([=](){
//		qDb() << "GOT F2W THEN() CALLBACK / future finished:" << f2.state();
//	});
//	f2w.setFuture(f2);


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

    // From http://en.cppreference.com/w/cpp/experimental/make_ready_future:
    // "If std::decay_t<T> is std::reference_wrapper<X>, then the type V is X&, otherwise, V is std::decay_t<T>."
	static_assert(std::is_same_v<decltype(make_ready_future(4)), ExtFuture<int> >);
    int v;
	static_assert(!std::is_same_v<decltype(make_ready_future(std::ref(v))), ExtFuture<int&> >);
    /// @todo
//    static_assert(std::is_same_v<decltype(make_ready_future()), ExtFuture<Unit> >, "");

}
