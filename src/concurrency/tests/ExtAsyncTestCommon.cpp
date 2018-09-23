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



#include "ExtAsyncTestCommon.h"

#include <tests/TestHelpers.h>



trackable_generator_base::trackable_generator_base(ExtAsyncTestsSuiteFixtureBase *fixture)
{
    auto test_id = fixture->get_test_id_string_from_fixture();
    m_generator_id = test_id;
    m_belongs_to_test_case_id = fixture->get_test_id_string_from_fixture();
}

/**
 * @note This does not have to be wrapped in an async run() function, it handles that itself.
 * It blocks and then returns the string.
 * @param fixture
 * @return
 */
QString delayed_string_func_1(ExtAsyncTestsSuiteFixtureBase *fixture)
{
	AMLMTEST_SCOPED_TRACE("In delayed_string_func_1");

    auto tgb = new trackable_generator_base(fixture);
    fixture->register_generator(tgb);

	QFuture<QString> qretval = QtConcurrent::run([=]() -> QString {
        // Sleep for a second.
        qDb() << "ENTER, SLEEPING FOR 1 SEC";
		TC_Sleep(1000);
        qDb() << "SLEEP COMPLETE";

        fixture->unregister_generator(tgb);
        delete tgb;

        return QString("delayed_string_func_1() output");
    });
	ExtFuture<QString> retval = qretval;
	// QVERIFY() does a "return;", so we can't use it in a function returning a value.
	Q_ASSERT(retval.isStarted());
	Q_ASSERT(!retval.isFinished());

    GTEST_COUT_qDB << "delayed_string_func_1() returning future:" << retval.state();

    return retval.result();
}


/// InterState

std::string InterState::get_currently_running_test() const
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
    return m_currently_running_test;
}

void InterState::starting(std::string func)
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
	AMLMTEST_ASSERT_TRUE(m_currently_running_test.empty());
    m_currently_running_test = func;
}

void InterState::finished(std::string func)
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
//    m_finished_set.insert(func);
	AMLMTEST_ASSERT_STREQ(m_currently_running_test.c_str(), func.c_str());
	AMLMTEST_ASSERT_FALSE(m_currently_running_test.empty());
    m_currently_running_test.clear();
}

bool InterState::is_test_currently_running() const
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
	return !m_currently_running_test.empty();
}

void InterState::start_SetUp(ExtAsyncTestsSuiteFixtureBase* fixture)
{
	std::lock_guard<std::mutex> lock(m_fixture_state_mutex);

	// Should always only get here after the last TearDown() has finished.
	AMLMTEST_ASSERT_EQ(m_setup_called_but_not_teardown, false);
	m_setup_called_but_not_teardown = true;
}

void InterState::start_TearDown(ExtAsyncTestsSuiteFixtureBase* fixture)
{
	std::lock_guard<std::mutex> lock(m_fixture_state_mutex);

	// Should always only get here after the last SetUp() has started.
	AMLMTEST_ASSERT_EQ(m_setup_called_but_not_teardown, true);
	m_setup_called_but_not_teardown = false;
}

TestHandle InterState::register_current_test(ExtAsyncTestsSuiteFixtureBase* fixture)
{
	std::lock_guard<std::mutex> lock(m_fixture_state_mutex);

	auto test_id_string = fixture->get_test_id_string_from_fixture();

	TestHandle th {test_id_string};

	AMLMTEST_COUT << "REGISTERING TEST:" << test_id_string << "Current test:" << m_current_test_handle.m_test_id_string;

	Q_ASSERT_X(m_current_test_handle.empty(), __PRETTY_FUNCTION__, QString("Last test never unregistered: %1").arg(toqstr(m_current_test_handle.m_test_id_string)).toStdString().c_str());

	m_current_test_handle = th;

	return th;
}

void InterState::unregister_current_test(TestHandle test_handle, ExtAsyncTestsSuiteFixtureBase* fixture)
{
	std::lock_guard<std::mutex> lock(m_fixture_state_mutex);

	AMLMTEST_SCOPED_TRACE(__PRETTY_FUNCTION__);

	AMLMTEST_COUT << "UNREGISTERING TEST:" << test_handle.m_test_id_string << "Current Test:" << m_current_test_handle.m_test_id_string;

	m_current_test_handle.m_test_id_string.clear();

	AMLMTEST_ASSERT_TRUE(m_current_test_handle.empty());

}

void InterState::register_generator(trackable_generator_base *generator)
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
	AMLMTEST_SCOPED_TRACE("register_generator");
    GTEST_COUT_qDB << "REGISTERING GENERATOR:" << generator->get_generator_id();
    m_generator_stack.push_back(generator);
}

void InterState::unregister_generator(trackable_generator_base *generator)
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
	AMLMTEST_SCOPED_TRACE("unregister_generator");
    GTEST_COUT_qDB << "UNREGISTERING GENERATOR:" << generator->get_generator_id();

    // Only makes sense if a test case is running.
	AMLMTEST_ASSERT_FALSE(m_currently_running_test.empty());// << "No test case running when trying to unregister generator";

    // Get the topmost generator.
    auto tgen = m_generator_stack.back();
	AMLMTEST_EXPECT_STREQ(tgen->get_generator_id().c_str(), generator->get_generator_id().c_str()); // << "Unregistering incorrect generator: Top: " << tgen->get_generator_id() << ", Unreg: " << generator->get_generator_id();
    m_generator_stack.pop_back();
}

bool InterState::check_generators()
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
    if(!m_generator_stack.empty())
    {
        // Generator still registered.

        for(auto g = m_generator_stack.cbegin(); g != m_generator_stack.cend(); ++g)
        {
            GTEST_COUT_qDB << "REGISTERED GENERATOR:" << (*g)->get_generator_id();
        }

        return false;
    }
    return true;
}



/// ExtAsyncTestsSuiteFixtureBase

InterState ExtAsyncTestsSuiteFixtureBase::m_interstate;

std::mutex ExtAsyncTestsSuiteFixtureBase::s_startup_teardown_mutex;

void ExtAsyncTestsSuiteFixtureBase::SetUp()
{
	AMLMTEST_SCOPED_TRACE("In SetUp()");

	s_startup_teardown_mutex.lock();

#ifdef TEST_FWK_IS_GTEST
	AMLMTEST_ASSERT_NO_FATAL_FAILURE({
										 m_interstate.start_SetUp(this);
									 });

    auto testinfo = ::testing::UnitTest::GetInstance()->current_test_info();

	GTEST_COUT << "SetUp() for test: " << testinfo->name() << ", test case: " << testinfo->test_case_name() << std::endl;
	AMLMTEST_EXPECT_NO_FATAL_FAILURE({
                                expect_all_preconditions();
										 TestHandle th = m_interstate.register_current_test(this);

                            });
#endif
	AMLMTEST_ASSERT_TRUE(QThread::currentThread()->eventDispatcher() != nullptr);

    // Make sure an event loop is running, and set it up to be destructed.
    /// @see @link https://stackoverflow.com/a/33829950 for what this is trying to do here.
//    ASSERT_TRUE(m_event_loop_object == nullptr);
//    m_event_loop_object = new QObject(qApp);
//    m_delete_spy = new QSignalSpy(m_event_loop_object, &QObject::destroyed);
//    ASSERT_TRUE(QThread::currentThread()->eventDispatcher() != nullptr);
}

void ExtAsyncTestsSuiteFixtureBase::expect_all_preconditions()
{
	AMLMTEST_ASSERT_FALSE(m_interstate.is_test_currently_running()); // << "A test was still running:" << get_currently_running_test();
	AMLMTEST_ASSERT_TRUE(m_interstate.get_currently_running_test().empty()); // << "A test was still running" << get_currently_running_test();
}

void ExtAsyncTestsSuiteFixtureBase::TearDown()
{
	AMLMTEST_SCOPED_TRACE("In TearDown()");

	s_startup_teardown_mutex.unlock();

    // Tear down the event loop.
    /// @see @link https://stackoverflow.com/a/33829950 for what this is trying to do here.
//    m_event_loop_object->deleteLater();
	AMLMTEST_COUT << "Waiting for event loop to be destroyed...";
//    auto didnt_time_out = m_delete_spy->wait(1000*60);
//    ASSERT_TRUE(didnt_time_out);
//    delete m_delete_spy;
//    m_event_loop_object = nullptr;
//    m_delete_spy = nullptr;

#if defined(TEST_FWK_IS_GTEST)
    auto testinfo = ::testing::UnitTest::GetInstance()->current_test_info();
    auto test_id = testinfo->test_case_name() + std::string("_") + testinfo->name();
	AMLMTEST_COUT << "TearDown() for test_id: " << test_id;
	AMLMTEST_ASSERT_NO_FATAL_FAILURE({
										 TestHandle dummy {"dummy"};
										 m_interstate.unregister_current_test(dummy, this);
                                expect_all_postconditions();
										 m_interstate.start_TearDown(this);
                            });
#endif
}

void ExtAsyncTestsSuiteFixtureBase::expect_all_postconditions()
{
	AMLMTEST_ASSERT_TRUE(m_interstate.check_generators()); // << "Generators not cleaned up";
}

std::string ExtAsyncTestsSuiteFixtureBase::get_currently_running_test()
{
    return m_interstate.get_currently_running_test();
}

void ExtAsyncTestsSuiteFixtureBase::starting(std::string func)
{
    m_interstate.starting(func);
}

void ExtAsyncTestsSuiteFixtureBase::finished(std::string func)
{
    m_interstate.finished(func);
}

bool ExtAsyncTestsSuiteFixtureBase::check_generators()
{
    return m_interstate.check_generators();
}

std::string ExtAsyncTestsSuiteFixtureBase::get_test_id_string_from_fixture()
{
#ifdef TEST_FWK_IS_GTEST
    auto testinfo = ::testing::UnitTest::GetInstance()->current_test_info();
    auto test_id = testinfo->test_case_name() + std::string("_") + testinfo->name();
#else
	auto test_id = "TODO";
#endif
	return test_id;
}

//TestHandle ExtAsyncTestsSuiteFixtureBase::get_test_handle_from_fixture()
//{

//}

void ExtAsyncTestsSuiteFixtureBase::register_generator(trackable_generator_base *generator)
{
	AMLMTEST_ASSERT_TRUE(m_interstate.is_test_currently_running()); // << "No test was running";
    m_interstate.register_generator(generator);

}

void ExtAsyncTestsSuiteFixtureBase::unregister_generator(trackable_generator_base *generator)
{
	AMLMTEST_ASSERT_TRUE(m_interstate.is_test_currently_running()); // << "No test was running";
    m_interstate.unregister_generator(generator);
}



