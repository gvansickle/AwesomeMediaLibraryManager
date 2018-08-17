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
//using FutureTypes = ::testing::Types<ExtFuture<int>, QFuture<int>>;
//TYPED_TEST_CASE(ExtFutureTest, FutureTypes);

//
// TESTS
//

TEST_F(ExtFutureTest, ReadyFutureCompletion)
{
    TC_ENTER();

    ExtFuture<int64_t> ef = make_ready_future(25L);

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

    TC_DONE_WITH_STACK();
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

    TC_DONE_WITH_STACK();
    TC_EXIT();
}

/**
 * Test "streaming" tap().
 * @todo Currently crashes.
 */
TEST_F(ExtFutureTest, DISABLED_ExtFutureStreamingTap)
{
    TC_ENTER();

    static std::atomic_int num_tap_completions {0};

    using eftype = ExtFuture<int>;

    QList<int> expected_results {1,2,3,4,5,6};
    eftype ef = async_int_generator<eftype>(1, 6, this);

    GTEST_COUT_qDB << "Starting ef state:" << ef.state();

    ASSERT_TRUE(ef.isStarted());
    ASSERT_FALSE(ef.isCanceled());
    ASSERT_FALSE(ef.isFinished());

    QList<int> async_results_from_tap, async_results_from_get;

    GTEST_COUT_qDB << "Attaching tap and get()";

//    async_results_from_get =
M_WARNING("TODO: This is still spinning when the test exits.")
    ef.tap(QCoreApplication::instance(), [&](eftype& ef, int begin, int end) {
            GTEST_COUT_qDB << "IN TAP, begin:" << begin << ", end:" << end;
        for(int i = begin; i<end; i++)
        {
            GTEST_COUT_qDB << "Pushing" << ef.resultAt(i) << "to tap list.";
            async_results_from_tap.push_back(ef.resultAt(i));
            num_tap_completions++;
        }
    });

    GTEST_COUT_qDB << "BEFORE WAITING FOR GET()" << ef;

    ef.wait();

    GTEST_COUT_qDB << "AFTER WAITING FOR GET()" << ef;
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

    static_assert(std::is_default_constructible<QString>::value, "");

    // From http://en.cppreference.com/w/cpp/experimental/make_ready_future:
    // "If std::decay_t<T> is std::reference_wrapper<X>, then the type V is X&, otherwise, V is std::decay_t<T>."
    static_assert(std::is_same_v<decltype(make_ready_future(4)), ExtFuture<int> >, "");
    int v;
    static_assert(std::is_same_v<decltype(make_ready_future(std::ref(v))), ExtFuture<int&> >, "");
    /// @todo
//    static_assert(std::is_same_v<decltype(make_ready_future()), ExtFuture<Unit> >, "");

}
