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
#include <deque>

// Qt5
#include <QSignalSpy>
#include <QTest>

// Google Test
#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>

// Ours
//#include <tests/TestHelpers.h>
#include "../ExtFuture.h"
#include "../ExtAsync.h"
#include "../ExtAsync_traits.h"


class ExtAsyncTestsSuiteFixtureBase;

class trackable_generator_base
{
    // Just so we can keep a container of generators in the test fixture.
public:
    explicit trackable_generator_base(ExtAsyncTestsSuiteFixtureBase* fixture);

    std::string get_generator_id() const { return m_generator_id; }

    std::string get_belongs_to_test_case_id() const { return m_belongs_to_test_case_id; }

protected:
    std::string m_generator_id;
    std::string m_belongs_to_test_case_id;
};


/**
 * From a lambda passed to QtConcurrent::run(), sleeps for 1 sec and then returns a single QString.
 * @return
 */
QString delayed_string_func_1(ExtAsyncTestsSuiteFixtureBase* fixture);


/**
 * Helper class for maintaining state across Google Test fixture invocations.
 * Each TEST_F() gets its own complete copy of the ::testing::Test class,
 * so we need something static to maintain statistics, sanity checks, etc.
 */
class InterState
{
public:

    void starting(std::string func);

    void finished(std::string func);

    std::string get_currently_running_test() const;

    bool is_test_currently_running() const;

    void register_generator(trackable_generator_base* generator);

    void unregister_generator(trackable_generator_base* generator);

    bool check_generators();

protected:

    /// @name Tracking state and a mutex to protect it.
    /// @{
    mutable std::mutex m_fixture_state_mutex;
    std::string m_currently_running_test;
    std::deque<trackable_generator_base*> m_generator_stack;
    /// @}
};

/**
 * Test Suite (ISTQB) or "Test Case" (Google) for ExtAsyncTests.
 * @link https://github.com/google/googletest/blob/master/googletest/docs/faq.md#can-i-derive-a-test-fixture-from-another
 */
class ExtAsyncTestsSuiteFixtureBase : public ::testing::Test
{

protected:

    void SetUp() override;
    virtual void expect_all_preconditions();

    void TearDown() override;
    virtual void expect_all_postconditions();

    // Objects declared here can be used by all tests in this Fixture.  However,
    // "googletest does not reuse the same test fixture for multiple tests. Any changes one test makes to the fixture do not affect other tests."
    // @link https://github.com/google/googletest/blob/master/googletest/docs/primer.md

    /// Static object for tracking state across TEST_F()'s.
    static InterState m_interstate;

    std::string get_currently_running_test();

    void starting(std::string func);

    void finished(std::string func);

    bool has_finished(std::string func);

    bool check_generators();

    QObject* m_event_loop_object {nullptr};
    QSignalSpy* m_delete_spy {nullptr};

public:

    /// Per-"test-case" (test fixture) set-up.
    /// Called before the first test in this test case.
    /// Can be omitted if not needed.
    static void SetUpTestCase() { }

    /// Per-"test-case" (test fixture) tear-down.
    /// Called after the last test in this test case.
    /// Can be omitted if not needed.
    static void TearDownTestCase() { }

    /**
     * Returns the Fixture_TestCase name of the currently running test.
     * @note Deliberately not threadsafe.  Only call this in the TC_ENTER() and TC_EXIT() macros.
     */
    std::string get_test_id_string_from_fixture();

    void register_generator(trackable_generator_base* generator);

    void unregister_generator(trackable_generator_base* generator);
};

/// @name Additional test helper macros.
/// @{

#define GTEST_COUT_qDB qDb()

#define TC_ENTER() \
    /* The name of this test as a static std::string. */ \
    static const std::string static_test_id_string {this->get_test_id_string_from_fixture()}; \
    starting(static_test_id_string); \
    static std::atomic_bool test_func_called {true}; \
    static std::atomic_bool test_func_exited {false}; \
    static std::atomic_bool test_func_no_longer_need_stack_ctx {false}; \
    static std::atomic_bool test_func_stack_is_gone {false}; \
    TC_EXPECT_THIS_TC();


#define TC_EXPECT_THIS_TC() \
    EXPECT_EQ(get_currently_running_test(), static_test_id_string);

#define TC_EXPECT_NOT_EXIT() \
    EXPECT_TRUE(test_func_called) << static_test_id_string; \
    EXPECT_FALSE(test_func_exited) << static_test_id_string;

#define TC_EXPECT_STACK() \
    EXPECT_FALSE(test_func_stack_is_gone)

#define TC_DONE_WITH_STACK() \
    test_func_no_longer_need_stack_ctx = true;

#define TC_EXIT() \
    TC_EXPECT_THIS_TC(); \
    TC_DONE_WITH_STACK(); \
    test_func_exited = true; \
    test_func_stack_is_gone = true; \
    ASSERT_TRUE(test_func_called); \
    ASSERT_TRUE(test_func_exited); \
    ASSERT_TRUE(test_func_no_longer_need_stack_ctx);\
    finished(static_test_id_string);


/// @}

/// @name Template helpers to allow the same syntax for QFuture<> and ExtFuture<> in tests.
/// @{

/**
 * Helper to which returns a finished QFuture<T>.
 */
template <typename T>
QFuture<T> make_finished_QFuture(const T &val)
{
   QFutureInterface<T> fi;
   fi.reportFinished(&val);
   return QFuture<T>(&fi);
}

/**
 * Helper which returns a Started but not Cancelled QFuture<T>.
 */
template <typename T>
QFuture<T> make_startedNotCanceled_QFuture()
{
    SCOPED_TRACE("");
    QFutureInterface<T> fi;
    fi.reportStarted();
    EXPECT_EQ(ExtFutureState::state(fi), ExtFutureState::Started | ExtFutureState::Running);
    return QFuture<T>(&fi);
}

/**
 * Helper for creating a non-canceled QFuture<T>.  Default constructor leaves it canceled and finished.
 */
template<class FutureT, class T>
FutureT make_default_future()
{
    SCOPED_TRACE("make_default_future");
    QFutureInterface<T> fi;
    fi.reportStarted();
    EXPECT_EQ(ExtFutureState::state(fi), ExtFutureState::Started | ExtFutureState::Running);
    return FutureT(&fi);
}

template <typename T>
void reportFinished(QFuture<T>& f)
{
    SCOPED_TRACE("reportFinished(QFuture<T>& f)");
    f.d.reportFinished();
    // May have been already canceled by the caller.
    EXPECT_TRUE((ExtFutureState::state(f) & ~ExtFutureState::Canceled) & (ExtFutureState::Started | ExtFutureState::Finished));
}

template <typename T>
void reportFinished(ExtFuture<T>& f)
{
    SCOPED_TRACE("reportFinished(ExtFuture<T>& f)");

    f.reportFinished();

    EXPECT_TRUE(f.isFinished());
}

template <typename FutureT, class ResultType>
void reportResult(FutureT& f, ResultType t)
{
    SCOPED_TRACE("reportResult");

    if constexpr (isExtFuture_v<FutureT>)
    {
        f.reportResult(t);
    }
    else
    {
        f.d.reportResult(t);
    }
}

/// @}

/**
 * From a lambda passed to ExtAsync::run(), iterates @a num_iteration times,
 * QTest::qSleep()ing for 1 sec, then returns the the next value in the sequence to the returned ExtFuture<>.
 *
 * @todo Doesn't handle cancellation or progress reporting.
 */
template<class ReturnFutureT>
ReturnFutureT async_int_generator(int start_val, int num_iterations, ExtAsyncTestsSuiteFixtureBase *fixture)
{
    SCOPED_TRACE("In async_int_generator");

    auto tgb = new trackable_generator_base(fixture);
    fixture->register_generator(tgb);

    auto lambda = [=](ReturnFutureT future) {
        int current_val = start_val;
        SCOPED_TRACE("In async_int_generator callback");
        for(int i=0; i<num_iterations; i++)
        {
            // Sleep for a second.
            GTEST_COUT_qDB << "SLEEPING FOR 1 SEC";

            QTest::qSleep(1000);
            GTEST_COUT_qDB << "SLEEP COMPLETE, sending value to future:" << current_val;

            reportResult(future, current_val);
            current_val++;
        }

        // We're done.
        GTEST_COUT_qDB << "REPORTING FINISHED";
        fixture->unregister_generator(tgb);
        delete tgb;

        reportFinished(future);
    };

    ReturnFutureT retval;
    if constexpr (std::is_same_v<ReturnFutureT, QFuture<int>>)
    {
        // QFuture() creates an empty, cancelled future (Start|Canceled|Finished).
        GTEST_COUT_qDB << "QFuture<>, clearing state";
        retval = make_startedNotCanceled_QFuture<int>();
    }

    GTEST_COUT_qDB << "ReturnFuture initial state:" << ExtFutureState::state(retval);

    EXPECT_TRUE(retval.isStarted());
    EXPECT_FALSE(retval.isFinished());

    if constexpr (std::is_same_v<ReturnFutureT, QFuture<int>>)
    {
        GTEST_COUT_qDB << "Qt run()";
        auto qrunfuture = QtConcurrent::run(lambda, retval);
    }
    else
    {
        GTEST_COUT_qDB << "ExtAsync::run()";
        retval = ExtAsync::run_efarg(lambda);
    }

    static_assert(std::is_same_v<decltype(retval), ReturnFutureT>, "");

    GTEST_COUT_qDB << "RETURNING future:" << ExtFutureState::state(retval);

    EXPECT_TRUE(retval.isStarted());
//    if constexpr (std::is_same_v<ReturnFutureT, QFuture<int>>)
//    {
//        // QFuture starts out Start|Canceled|Finished.
//        EXPECT_TRUE(retval.isCanceled());
//        EXPECT_TRUE(retval.isFinished());
//    }
//    else
//    {
//        EXPECT_FALSE(retval.isCanceled());
//        EXPECT_FALSE(retval.isFinished());
//    }

    return retval;
}



#endif // EXTASYNCTESTCOMMON_H
