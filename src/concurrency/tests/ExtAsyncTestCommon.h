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

#include <config.h>

// Std C++
#include <memory>
#include <string>
#include <deque>

// Qt5
#include <QSignalSpy>
#include <QTest>

// Ours, but with GTest/QTest in the middle.
#include <tests/TestHelpers.h>

#ifdef TEST_FWK_IS_GTEST
//	M_WARNING("Building for Google Test framework");
// Google Test Framework
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <tests/TestLifecycleManager.h>

#elif defined(TEST_FWK_IS_QTEST)
//	M_WARNING("Building for QTest framework");
#else
#error "No test framework defined"
#endif // TEST_FWK_IS_GTEST
#if defined(TEST_FWK_IS_QTEST) && defined(TEST_FWK_IS_GTEST)
#error "BOTH TEST FRAMEWORKS DEFINED"
#endif


// Ours
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

struct TestHandle
{
	std::string m_test_id_string {};

	bool empty() const { return m_test_id_string.empty(); };
};

/**
 * Helper class for maintaining state across Google Test fixture invocations.
 * Each TEST_F() gets its own complete copy of the ::testing::Test class,
 * so we need something static to maintain statistics, sanity checks, etc.
 * Public interfaces to this class are all threadsafe.
 */
class InterState
{
public:
	InterState()
	{
		AMLMTEST_COUT << "InterState singleton constructed";
		Q_ASSERT_X(m_current_test_handle.empty(), "constructor", m_current_test_handle.m_test_id_string.c_str());
	};
	virtual ~InterState() { AMLMTEST_COUT << "InterState singleton destructed"; };

    void starting(std::string func);

    void finished(std::string func);

    std::string get_currently_running_test() const;

    bool is_test_currently_running() const;

    void start_SetUp(ExtAsyncTestsSuiteFixtureBase* fixture);
    void start_TearDown(ExtAsyncTestsSuiteFixtureBase* fixture);

	TestHandle register_current_test(ExtAsyncTestsSuiteFixtureBase* fixture);
	void unregister_current_test(TestHandle test_handle, ExtAsyncTestsSuiteFixtureBase* fixture);

    void register_generator(trackable_generator_base* generator);

    void unregister_generator(trackable_generator_base* generator);

    bool check_generators();

protected:

    /// @name Tracking state and a mutex to protect it.
    /// @{
    mutable std::mutex m_fixture_state_mutex;
    std::string m_currently_running_test;
    std::deque<trackable_generator_base*> m_generator_stack;
	TestHandle m_current_test_handle {};
	std::atomic_bool m_setup_called_but_not_teardown {false};
    /// @}
};



#ifdef TEST_FWK_IS_GTEST
using AMLMTEST_BASE_CLASS = ::testing::Test;
#define AMLMTEST_BASE_CLASS_VIRT /**/
#define AMLMTEST_BASE_CLASS_OVERRIDE override
#elif defined(TEST_FWK_IS_QTEST) // !TEST_FWK_IS_GTEST
using AMLMTEST_BASE_CLASS = QObject;
#define AMLMTEST_BASE_CLASS_VIRT virtual
#define AMLMTEST_BASE_CLASS_OVERRIDE /**/
#endif

/**
 * Test Suite (ISTQB) or "Test Case" (Google) for ExtAsyncTests.
 * @link https://github.com/google/googletest/blob/master/googletest/docs/faq.md#can-i-derive-a-test-fixture-from-another
 */
class ExtAsyncTestsSuiteFixtureBase : public AMLMTEST_BASE_CLASS // ::testing::Test or QObject
{

protected:

	AMLMTEST_BASE_CLASS_VIRT void SetUp() AMLMTEST_BASE_CLASS_OVERRIDE;
    virtual void expect_all_preconditions();

	AMLMTEST_BASE_CLASS_VIRT void TearDown() AMLMTEST_BASE_CLASS_OVERRIDE;
    virtual void expect_all_postconditions();

    // Objects declared here can be used by all tests in this Fixture.  However,
    // "googletest does not reuse the same test fixture for multiple tests. Any changes one test makes to the fixture do not affect other tests."
    // @link https://github.com/google/googletest/blob/master/googletest/docs/primer.md

    std::string get_currently_running_test();

    void starting(std::string func);

    void finished(std::string func);

    bool has_finished(std::string func);

    bool check_generators();

    QObject* m_event_loop_object {nullptr};
    QSignalSpy* m_delete_spy {nullptr};

	/// Mutex covering the entire CFG between StartUp() and TearDown().
	/// Another attempt to make sure the individual tests are serialized.
	static std::mutex s_setup_teardown_mutex;

	/// Static object for tracking state across TEST_F()'s.
	static InterState m_interstate;

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

//	TestHandle get_test_handle_from_fixture();

    void register_generator(trackable_generator_base* generator);

    void unregister_generator(trackable_generator_base* generator);

#if defined(TEST_FWK_IS_QTEST) // !TEST_FWK_IS_GTEST

	/// QTest framework slots.
private Q_SLOTS:

	/// @name QTest framework slots.
	/// @{
	void initTestCase()
	{ qDebug("called before everything else"); }

	/// Gtest equiv == protected SetUp();
	void init()
	{
		qDebug("called before each test function is executed");
		SetUp();
	}

	/// GTest equiv == protected TearDown();
	void cleanup()
	{
		qDebug("called after every test function");
		TearDown();
	}

	void cleanupTestCase()
	{ qDebug("alled after the last test function was executed."); }
	/// @}
#endif

};

//#ifdef TEST_FWK_IS_GTEST
////// Specialize an action that synchronizes with the calling thread.
////// @link https://stackoverflow.com/questions/10767131/expecting-googlemock-calls-from-another-thread
////ACTION_P2(ReturnFromAsyncCall, RetVal, SemDone)
////{
////	SemDone->release();
////	return RetVal;
////}

//#define TC_END_RSM(sem) \
//do {\
//	bool acquired = (sem).tryAcquire(1, 1000);\
//	EXPECT_TRUE(acquired);\
//} while(0)
//#else
//#define TC_END_RSM(sem) /* No gtest... nothing? */
//#endif


/// @name Additional ExtAsync-specific test helper macros.
/// @{

/// Future state is at least Started, not finished or canceled.
#define AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(future) \
	AMLMTEST_EXPECT_TRUE(future.isStarted());\
	AMLMTEST_EXPECT_FALSE(future.isCanceled());\
	AMLMTEST_EXPECT_FALSE(future.isFinished());


/// Future state is (Started|Finished)
#define AMLMTEST_EXPECT_FUTURE_FINISHED(future) \
		AMLMTEST_EXPECT_TRUE(ExtFutureState::state(future) == (ExtFutureState::Started | ExtFutureState::Finished))

/// After .cancel() is called on future.
/// Seems like we get Canceled but not Finished here.
#define AMLMTEST_EXPECT_FUTURE_POST_CANCEL(future) \
	/*AMLMTEST_ASSERT_TRUE(future.isFinished());*/\
	AMLMTEST_ASSERT_TRUE(future.isStarted());\
	AMLMTEST_ASSERT_TRUE(future.isCanceled());

#define AMLMTEST_EXPECT_FUTURE_POST_EXCEPTION(future) \
	AMLMTEST_ASSERT_TRUE(future.isFinished());\
	AMLMTEST_ASSERT_TRUE(future.isStarted());\
	AMLMTEST_ASSERT_TRUE(future.isCanceled());

#define GTEST_COUT_qDB qDb()

#ifdef TEST_FWK_IS_GTEST
#define TC_ENTER() \
	/* The Google Mock-based TestLifecycleManager instance for this test. */ \
	TestLifecycleManager tlm; \
	{ ::testing::InSequence s; \
		EXPECT_CALL(tlm, MTC_ENTER()) \
			.Times(1); \
			EXPECT_CALL(tlm, MTC_EXIT()) \
			.Times(1); \
	}\
	tlm.MTC_ENTER(); \
    /* The name of this test as a static std::string. */ \
	const std::string static_test_id_string {this->get_test_id_string_from_fixture()}; \
	this->starting(static_test_id_string); \
	std::atomic_bool test_func_called {true}; \
	std::atomic_bool test_func_exited {false}; \
	std::atomic_bool test_func_no_longer_need_stack_ctx {false}; \
	std::atomic_bool test_func_stack_is_gone {false}; \
    TC_EXPECT_THIS_TC();
#else // !TEST_FWK_IS_GTEST
#define TC_ENTER() \
	/* The name of this test as a static std::string. */ \
	const std::string static_test_id_string {this->get_test_id_string_from_fixture()}; \
	this->starting(static_test_id_string); \
	std::atomic_bool test_func_called {true}; \
	std::atomic_bool test_func_exited {false}; \
	std::atomic_bool test_func_no_longer_need_stack_ctx {false}; \
	std::atomic_bool test_func_stack_is_gone {false}; \
	TC_EXPECT_THIS_TC();
#endif // TEST_FWK_IS_QTEST

#define TC_EXPECT_THIS_TC() \
	AMLMTEST_EXPECT_EQ(this->get_currently_running_test(), static_test_id_string);

#define TC_EXPECT_NOT_EXIT() \
	AMLMTEST_EXPECT_TRUE(test_func_called) << static_test_id_string; \
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
	AMLMTEST_ASSERT_TRUE(test_func_called); \
	AMLMTEST_ASSERT_TRUE(test_func_exited); \
	AMLMTEST_ASSERT_TRUE(test_func_no_longer_need_stack_ctx);\
	this->finished(static_test_id_string); \
	tlm.MTC_EXIT();

/// @name Macros for making sure a KJob emits the expected signals and gets destroyed before the TEST_F() returns.
/// @{
#define M_QSIGNALSPIES_SET(kjobptr) \
    QSignalSpy kjob_finished_spy(kjobptr, &KJob::finished); \
	AMLMTEST_EXPECT_TRUE(kjob_finished_spy.isValid()); \
    QSignalSpy kjob_result_spy(kjobptr, &KJob::result); \
	AMLMTEST_EXPECT_TRUE(kjob_result_spy.isValid()); \
	/*QSignalSpy kjob_destroyed_spy(static_cast<QObject*>(kjobptr), &QObject::destroyed);*/ \
	/*EXPECT_TRUE(kjob_destroyed_spy.isValid());*/ \
	/* Workaround for what otherwise should be doable with QSignalSpy() above, but isn't for some reason. */ \
	std::atomic_bool got_job_destroyed_signal {false}; \
	connect_or_die(kjobptr, &QObject::destroyed, qApp, [&](QObject* obj){ \
		TCOUT << "GOT DESTROYED SIGNAL:" << &obj; \
		got_job_destroyed_signal = true; \
	});

#define M_QSIGNALSPIES_EXPECT_IF_DESTROY_TIMEOUT() \
	{ \
		auto didnt_timeout = QTest::qWaitFor([&]() { return got_job_destroyed_signal.load(); }, 5000); \
		AMLMTEST_ASSERT_TRUE(got_job_destroyed_signal.load()); \
		AMLMTEST_ASSERT_TRUE(didnt_timeout); \
	/* EXPECT_TRUE(kjob_destroyed_spy.wait()); */ \
	}
/// @}

/// @}

/// @name Template helpers to allow the same syntax for QFuture<> and ExtFuture<> in tests.
/// @{

/**
 * Helper to which returns a finished (Started|Finished) QFuture<T>.
 */
template <typename T>
QFuture<T> make_finished_QFuture(const T &val)
{
	/// @note QFuture<>'s returned by QtConcurrent::run() are Started|Finished when the
	/// future is finished.
	/// So, that seems to be the natural state of a successfully finished QFuture<>,
	/// and we copy that here.

	AMLMTEST_SCOPED_TRACE("make_finished_QFuture");

	QFutureInterface<T> fi;
	fi.reportStarted();
	fi.reportFinished(&val);
	return QFuture<T>(&fi);
}

/**
 * Helper which returns a Started but not Cancelled QFuture<T>.
 */
template <typename T>
QFuture<T> make_startedNotCanceled_QFuture()
{
	AMLMTEST_SCOPED_TRACE("make_startedNotCanceled_QFuture");
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
	AMLMTEST_SCOPED_TRACE("make_default_future");
    QFutureInterface<T> fi;
    fi.reportStarted();
    EXPECT_EQ(ExtFutureState::state(fi), ExtFutureState::Started | ExtFutureState::Running);
    return FutureT(&fi);
}

template <typename T>
void reportFinished(QFuture<T>* f)
{
	AMLMTEST_SCOPED_TRACE("reportFinished(QFuture<T>& f)");
	AMLMTEST_COUT << "REPORTING FINISHED";
	f->d.reportFinished();
    // May have been already canceled by the caller.
	EXPECT_TRUE((ExtFutureState::state(*f) & ~ExtFutureState::Canceled) & (ExtFutureState::Started | ExtFutureState::Finished));
}

template <typename T>
void reportFinished(ExtFuture<T>* f)
{
	AMLMTEST_SCOPED_TRACE("reportFinished(ExtFuture<T>& f)");

	f->reportFinished();

	AMLMTEST_EXPECT_TRUE(f->isFinished());
}

template <typename FutureT, class ResultType>
void reportResult(FutureT* f, ResultType t)
{
	AMLMTEST_SCOPED_TRACE("reportResult");
	if constexpr (is_ExtFuture_v<FutureT>)
    {
		f->reportResult(t);
    }
    else
    {
		f->d.reportResult(t);
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
	AMLMTEST_SCOPED_TRACE("In async_int_generator");

    auto tgb = new trackable_generator_base(fixture);
    fixture->register_generator(tgb);

    auto lambda = [=](ReturnFutureT future) {
        int current_val = start_val;
        AMLMTEST_SCOPED_TRACE("In async_int_generator callback");
        for(int i=0; i<num_iterations; i++)
        {
			/// @todo Not sure if we want this to work or not.
        	AMLMTEST_EXPECT_FALSE(ExtFutureState::state(future) & ExtFutureState::Canceled);

            // Sleep for a second.
            GTEST_COUT_qDB << "SLEEPING FOR 1 SEC";

            TC_Sleep(1000);
            GTEST_COUT_qDB << "SLEEP COMPLETE, sending value to future:" << current_val;

			reportResult(&future, current_val);
            current_val++;
        }

        // We're done.
        GTEST_COUT_qDB << "REPORTING FINISHED";
		reportFinished(&future);

        fixture->unregister_generator(tgb);
        delete tgb;

    };

    ReturnFutureT retval;
    if constexpr (std::is_same_v<ReturnFutureT, QFuture<int>>)
    {
        // QFuture() creates an empty, cancelled future (Start|Canceled|Finished).
        GTEST_COUT_qDB << "QFuture<>, clearing state";
        retval = make_startedNotCanceled_QFuture<int>();
    }

    GTEST_COUT_qDB << "ReturnFuture initial state:" << ExtFutureState::state(retval);

//    AMLMTEST_EXPECT_TRUE(retval.isStarted());
//	AMLMTEST_EXPECT_FALSE(retval.isCanceled());
//    AMLMTEST_EXPECT_FALSE(retval.isFinished());

	if constexpr (std::is_same_v<ReturnFutureT, QFuture<int>>)
    {
        GTEST_COUT_qDB << "QtConcurrent::run()";
        auto qrunfuture = QtConcurrent::run(lambda, retval);
    }
    else
    {
		GTEST_COUT_qDB << "ExtAsync::run()";
		retval = ExtAsync::run(lambda);
    }

    GTEST_COUT_qDB << "RETURNING future:" << ExtFutureState::state(retval);

//	AMLMTEST_EXPECT_TRUE(retval.isStarted());
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
