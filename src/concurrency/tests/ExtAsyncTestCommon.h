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

#ifndef EXTASYNCTESTCOMMON_H
#define EXTASYNCTESTCOMMON_H

// Std C++
#include <memory>
#include <string>
#include <stack>

// Qt5
#include <QTest>

// Google Test
#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>

// Ours
//#include <tests/TestHelpers.h>
#include "../ExtFuture.h"
#include "../ExtAsync.h"


class ExtAsyncTestsSuiteFixtureBase;

class trackable_generator_base
{
    // Just so we can keep a container of generators in the test fixture.
public:
    explicit trackable_generator_base(ExtAsyncTestsSuiteFixtureBase* fixture);

    std::string get_generator_id() const { return m_generator_id; }

protected:
    std::string m_generator_id;
};

//template <class FutureTypeT>
//class async_test_generator : public trackable_generator_base
//{
//public:
//    explicit async_test_generator(ExtAsyncTestsSuiteFixtureBase* fixture) : m_fixture(fixture) {}
//    virtual ~async_test_generator() {}

//    virtual void register_gen() { m_fixture->register_generator(this); }
//    virtual void unregister_gen() { m_fixture->unregister_generator(this); }

//protected:
//    ExtAsyncTestsSuiteFixtureBase* m_fixture;
//};

/**
 * From a lambda passed to ExtAsync::run(), iterates @a num_iteration times,
 * QTest::qSleep()ing for 1 sec, then returns the the next value in the sequence to the returned ExtFuture<>.
 *
 * @todo Doesn't handle cancellation or progress reporting.
 */
ExtFuture<int> async_int_generator(int start_val, int num_iterations, ExtAsyncTestsSuiteFixtureBase* fixture);

/**
 * From a lambda passed to QtConcurrent::run(), sleeps for 1 sec and then returns a single QString.
 * @return
 */
QString delayed_string_func_1(ExtAsyncTestsSuiteFixtureBase* fixture);

/**
 * Test Suite (ISTQB) or "Test Case" (Google) for ExtAsyncTests.
 * @link https://github.com/google/googletest/blob/master/googletest/docs/faq.md#can-i-derive-a-test-fixture-from-another
 */
class ExtAsyncTestsSuiteFixtureBase : public ::testing::Test
{

protected:

    void SetUp() override;

    void TearDown() override;

    // Objects declared here can be used by all tests in this Fixture.

    /// Map of test cases which have finished.
    std::mutex m_fixture_state_mutex;
    std::set<std::string> m_finished_set;
    std::string m_currently_running_test;
    std::stack<trackable_generator_base*> m_generator_stack;


    std::string get_currently_running_test();

    void starting(std::string func);

    void finished(std::string func);

    bool has_finished(std::string func);

public:
    std::string get_test_id_string();

    void register_generator(trackable_generator_base* generator);

    void unregister_generator(trackable_generator_base* generator);

};

/// @name Additional test helper macros.
/// @{
#define TC_ENTER() \
    /* The name of this test as a static std::string. */ \
    static const std::string testname {get_test_id_string()}; \
    ExtAsync::name_qthread();\
    static std::atomic_bool test_func_called {true}; \
    static std::atomic_bool test_func_exited {false}; \
    static std::atomic_bool test_func_no_longer_need_stack_ctx {false}; \
    static std::atomic_bool test_func_stack_is_gone {false};

#define TC_EXPECT_THIS_TC() \
    EXPECT_EQ(get_currently_running_test(), testname);

#define TC_EXPECT_NOT_EXIT() \
    EXPECT_TRUE(test_func_called) << testname; \
    EXPECT_FALSE(test_func_exited) << testname;

#define TC_EXPECT_STACK() \
    EXPECT_FALSE(test_func_stack_is_gone)

#define TC_DONE_WITH_STACK() \
    test_func_no_longer_need_stack_ctx = true;

#define TC_EXIT() \
    test_func_exited = true; \
    test_func_stack_is_gone = true; \
    ASSERT_TRUE(test_func_called); \
    ASSERT_TRUE(test_func_exited); \
    ASSERT_TRUE(test_func_no_longer_need_stack_ctx);

/// @}




#endif // EXTASYNCTESTCOMMON_H
