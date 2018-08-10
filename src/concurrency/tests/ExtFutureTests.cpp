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


//void ExtFutureTest::SetUp()
//{
//    GTEST_COUT << "SetUp()" << std::endl;
//}

//void ExtFutureTest::TearDown()
//{
//    GTEST_COUT << "TearDown()" << std::endl;
//}

///**
// * From a lambda passed to ExtAsync::run(), iterates @a num_iteration times,
// * sleep()ing for 1 sec, then returns the the next value in the sequence to the returned ExtFuture<>.
// *
// * @todo Doesn't handle cancellation or progress reporting.
// */
//static ExtFuture<int> async_int_generator(int start_val, int num_iterations, std::atomic_bool& generator_run_completed)
//{
//    ExtFuture<int> future = ExtAsync::run_efarg([=](ExtFuture<int>& future) {
//        int current_val = start_val;
//        for(int i=0; i<num_iterations; i++)
//        {
//            // Sleep for a second.
//            qWr() << "SLEEPING FOR 1 SEC";

//            QThread::sleep(1);
//            qWr() << "SLEEP COMPLETE, sending value to future:" << current_val;

//            future.reportResult(current_val);
//            current_val++;
//        }
//        // We're done.
//        qWr() << "REPORTING FINISHED";
//        future.reportFinished();
//    });

//    static_assert(std::is_same_v<decltype(future), ExtFuture<int>>, "");

//    qWr() << "RETURNING:" << future;

//    return future;
//}

//
// TESTS
//

TEST_F(ExtFutureTest, ReadyFutureCompletion)
{
    ExtFuture<int64_t> ef = make_ready_future(25L);

    QList<int64_t> results = ef.get();

    EXPECT_EQ(ef.state(), ExtFutureState::Started | ExtFutureState::Finished);

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], 25L);
}

TEST_F(ExtFutureTest, FutureSingleThread)
{
    ExtFuture<int> ef;

    EXPECT_EQ(ef.state(), ExtFutureState::Started);

    ef.reportResult(1);

    EXPECT_EQ(ef.resultCount(), 1);
    EXPECT_EQ(ef.get()[0], 1);

    ef.reportResult(2);

    EXPECT_EQ(ef.resultCount(), 2);
    EXPECT_EQ(ef.get()[1], 2);
}

TEST_F(ExtFutureTest, CopyAssignTests)
{
    SCOPED_TRACE("CopyAssignTests");

    // default constructors
    ExtFuture<int> extfuture_int;
    extfuture_int.waitForFinished();

    ExtFuture<QString> extfuture_string;
    extfuture_string.waitForFinished();

    ExtFuture<Unit> ef_unit;
    ef_unit.waitForFinished();

    ExtFuture<Unit> ef_unit2;
    ef_unit2.waitForFinished();

    // copy constructor
    ExtFuture<int> ef_int2(extfuture_int);
    ExtFuture<Unit> ef_unit3(ef_unit2);

    // assigmnent operator
    ef_int2 = ExtFuture<int>();
    ef_unit3 = ExtFuture<Unit>();

    // state
    ASSERT_EQ(ef_int2.isStarted(), true);
    /// @note This is a difference between QFuture<> and ExtFuture<>, there's no reason this future should be finished here.
//    ASSERT_EQ(ef_int2.isFinished(), true);
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

/**
 * Test "streaming" tap().
 */
TEST_F(ExtFutureTest, ExtFutureStreamingTap)
{
    TC_ENTER();

    using eftype = ExtFuture<int>;

    eftype ef = async_int_generator(2, 5, this);

    qDb() << "Starting extfuture:" << ef;

    ASSERT_TRUE(ef.isStarted());
    ASSERT_FALSE(ef.isCanceled());
    ASSERT_FALSE(ef.isFinished());

    QList<int> async_results_from_tap, async_results_from_get;

    qDb() << "Attaching tap and get()";

    async_results_from_get = ef.tap([&](eftype& ef, int begin, int end){
            GTEST_COUT << "IN TAP";
        for(int i = begin; i<end; i++)
        {
            async_results_from_tap.push_back(ef.resultAt(i));
        }
    }).get();

    // .get() above should block.
    ASSERT_TRUE(ef.isFinished());

    ef.waitForFinished();

    qDb() << "Post .tap().get(), extfuture:" << ef;

//    EXPECT_TRUE(ef.isStarted());
//    EXPECT_FALSE(ef.isCanceled());
//    EXPECT_TRUE(ef.isFinished());

//    EXPECT_EQ(async_results_from_get.size(), 5);
//    EXPECT_EQ(async_results_from_tap.size(), 5);

    TC_DONE_WITH_STACK();
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
