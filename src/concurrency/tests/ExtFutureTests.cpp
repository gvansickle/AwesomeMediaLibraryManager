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

#include "../ExtFuture.h"


void ExtFutureTest::SetUp()
{
    GTEST_COUT << "SetUp()" << std::endl;
}

void ExtFutureTest::TearDown()
{
    GTEST_COUT << "TearDown()" << std::endl;
}


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

/// Static checks
TEST_F(ExtFutureTest, StaticAsserts){

    static_assert(std::is_default_constructible<QString>::value, "");

    // From http://en.cppreference.com/w/cpp/experimental/make_ready_future:
    // "If std::decay_t<T> is std::reference_wrapper<X>, then the type V is X&, otherwise, V is std::decay_t<T>."
    static_assert(std::is_same_v<decltype(make_ready_future(4)), ExtFuture<int> >, "");
    int v;
    static_assert(std::is_same_v<decltype(make_ready_future(std::ref(v))), ExtFuture<int&> >, "");
    /// @todo
//    static_assert(std::is_same_v<decltype(make_ready_future()), ExtFuture<Unit> >, "");

}
