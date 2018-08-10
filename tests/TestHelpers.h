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

#ifndef TESTS_TESTHELPERS_H_
#define TESTS_TESTHELPERS_H_

// Std C++
#include <sstream>
#include <stack>

// Qt5
#include <Qt>
#include <QString>

// Google Test
#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>

// Ours
#include <src/concurrency/ExtFuture.h>

QT_BEGIN_NAMESPACE

/// To let Google Test print QStrings.
inline void PrintTo(const QString &qString, ::std::ostream *os)
{
    *os << qUtf8Printable(qString);
}

/// To let Google Test print ExtFutures.
template <class T>
inline void PrintTo(const ExtFuture<T> &ef, ::std::ostream *os)
{
    QString str;
    QDebug dbg(&str);
    dbg << ef;
    PrintTo(str, os);
}

QT_END_NAMESPACE


/// Quick and dirty way to add information to the test log.
#define GTEST_COUT_ORIGINAL std::cout << "[          ] [ INFO ] "
#define GTEST_COUT GTEST_COUT_ORIGINAL

/// @name Hopefully less quick-and-dirty way to add information to test output.
/// @{
//namespace testing
//{
//    namespace internal
//    {
//    enum GTestColor {
//        COLOR_DEFAULT,
//        COLOR_RED,
//        COLOR_GREEN,
//        COLOR_YELLOW
//    };

/// @warning This is static in gtest now.
//    extern void ColoredPrintf(GTestColor color, const char* fmt, ...);
//    } // namespace internal
//} // namespace testing

//#define PRINTF(...)  do { testing::internal::ColoredPrintf(testing::internal::COLOR_GREEN, "[          ] "); testing::internal::ColoredPrintf(testing::internal::COLOR_YELLOW, __VA_ARGS__); } while(0)

//// C++ stream interface
//class TestCout : public std::stringstream
//{
//public:
//    ~TestCout()
//    {
//        PRINTF("%s", str().c_str());
//    }
//};

//#define TEST_COUT  TestCout()

/// @}

enum class GenericState
{
    NOT_STARTED,
    STARTED,
    FINISHED
};

class trackable_generator_base;

/**
 * Test Suite (ISTQB) or "Test Case" (Google) for ExtAsyncTests.
 * @link https://github.com/google/googletest/blob/master/googletest/docs/faq.md#can-i-derive-a-test-fixture-from-another
 */
class ExtAsyncTestsSuiteFixtureBase : public ::testing::Test
{

protected:

    void SetUp() override
    {
        SCOPED_TRACE("In SetUp()");
        auto testinfo = ::testing::UnitTest::GetInstance()->current_test_info();
        GTEST_COUT << "SetUp() for test: " << testinfo->name() << ", test case: " << testinfo->test_case_name() << std::endl;
        starting(get_test_id_string());
    }

    void TearDown() override
    {
        SCOPED_TRACE("In TearDown()");
        auto testinfo = ::testing::UnitTest::GetInstance()->current_test_info();
        auto test_id = testinfo->test_case_name() + std::string("_") + testinfo->name();
        GTEST_COUT << "TearDown() for test: " << testinfo->name() << ", test case: " << testinfo->test_case_name() << std::endl;
        EXPECT_EQ(m_generator_stack.size(), 0) << "Generator was not unregistered. Generator pointer: " << m_generator_stack.top();
        finished(get_test_id_string());
    }

    // Objects declared here can be used by all tests in this Fixture.

    /// Map of test cases which have finished.
    std::mutex m_fixture_state_mutex;
    std::set<std::string> m_finished_set;
    std::string m_currently_running_test;
    std::stack<trackable_generator_base*> m_generator_stack;

    std::string get_test_id_string()
    {
        auto testinfo = ::testing::UnitTest::GetInstance()->current_test_info();
        auto test_id = testinfo->test_case_name() + std::string("_") + testinfo->name();
        return test_id;
    }

    std::string get_currently_running_test()
    {
        std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
        return m_currently_running_test;
    }

    void starting(std::string func)
    {
        std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
        m_currently_running_test = func;
    }

    void finished(std::string func)
    {
        std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
        m_finished_set.insert(func);
        m_currently_running_test.clear();
    }

    bool has_finished(std::string func)
    {
        std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
        return m_finished_set.count(func) > 0;
    }

public:
    void register_generator(trackable_generator_base* generator)
    {
        std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
        m_generator_stack.push(generator);
    }

    void unregister_generator(trackable_generator_base* generator)
    {
        SCOPED_TRACE("unregister_generator");
        std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
        auto tgen = m_generator_stack.top();
        EXPECT_EQ(tgen, generator) << "Unregistering incorrect generator";
        m_generator_stack.pop();
    }

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
	GTEST_COUT << "EXITING: " << __PRETTY_FUNCTION__ << std::endl; \
	ASSERT_TRUE(test_func_called); \
	ASSERT_TRUE(test_func_exited); \
    ASSERT_TRUE(test_func_no_longer_need_stack_ctx);

/// @}

#endif /* TESTS_TESTHELPERS_H_ */
