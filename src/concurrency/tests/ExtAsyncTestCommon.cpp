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

QString delayed_string_func_1(ExtAsyncTestsSuiteFixtureBase *fixture)
{
    SCOPED_TRACE("In delayed_string_func_1");

    auto tgb = new trackable_generator_base(fixture);
    fixture->register_generator(tgb);

    ExtFuture<QString> retval = QtConcurrent::run([&]() -> QString {
        // Sleep for a second.
        qDb() << "ENTER, SLEEPING FOR 1 SEC";
        QTest::qSleep(1000);
        qDb() << "SLEEP COMPLETE";

        fixture->unregister_generator(tgb);
        delete tgb;

        return QString("delayed_string_func_1() output");
    });

    EXPECT_TRUE(retval.isStarted());
    EXPECT_FALSE(retval.isFinished());

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
    ASSERT_TRUE(m_currently_running_test.empty());
    m_currently_running_test = func;
}

void InterState::finished(std::string func)
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
//    m_finished_set.insert(func);
    ASSERT_STREQ(m_currently_running_test.c_str(), func.c_str());
    ASSERT_FALSE(m_currently_running_test.empty());
    m_currently_running_test.clear();
}

bool InterState::is_test_currently_running() const
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
    return !m_currently_running_test.empty();
}

void InterState::register_generator(trackable_generator_base *generator)
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
    SCOPED_TRACE("register_generator");
    GTEST_COUT_qDB << "REGISTERING GENERATOR:" << generator->get_generator_id();
    m_generator_stack.push_back(generator);
}

void InterState::unregister_generator(trackable_generator_base *generator)
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
    SCOPED_TRACE("unregister_generator");
    GTEST_COUT_qDB << "UNREGISTERING GENERATOR:" << generator->get_generator_id();

    // Only makes sense if a test case is running.
//    EXPECT_EQ(m_currently_running_test.empty()) << "No test case running when trying to unregister generator";

    // Get the topmost generator.
    auto tgen = m_generator_stack.back();
    EXPECT_STREQ(tgen->get_generator_id().c_str(), generator->get_generator_id().c_str()) << "Unregistering incorrect generator: Top: " << tgen->get_generator_id() << ", Unreg: " << generator->get_generator_id();
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

void ExtAsyncTestsSuiteFixtureBase::SetUp()
{
    SCOPED_TRACE("In SetUp()");
    auto testinfo = ::testing::UnitTest::GetInstance()->current_test_info();
    GTEST_COUT << "SetUp() for test: " << testinfo->name() << ", test case: " << testinfo->test_case_name() << std::endl;
    EXPECT_NO_FATAL_FAILURE({
                                expect_all_preconditions();
                            });

    // Make sure an event loop is running, and set it up to be destructed.
    /// @see @link https://stackoverflow.com/a/33829950 for what this is trying to do here.
    ASSERT_TRUE(m_event_loop_object == nullptr);
    m_event_loop_object = new QObject(qApp);
    m_delete_spy = new QSignalSpy(m_event_loop_object, &QObject::destroyed);
}

void ExtAsyncTestsSuiteFixtureBase::expect_all_preconditions()
{
    ASSERT_FALSE(m_interstate.is_test_currently_running()) << "A test was already running";
    EXPECT_TRUE(m_interstate.get_currently_running_test().empty()) << "A test was already running";
}

void ExtAsyncTestsSuiteFixtureBase::TearDown()
{
    SCOPED_TRACE("In TearDown()");

    // Tear down the event loop.
    /// @see @link https://stackoverflow.com/a/33829950 for what this is trying to do here.
    m_event_loop_object->deleteLater();
    qDb() << "Waiting for event loop...";
    m_delete_spy->wait(1000*60);
    delete m_delete_spy;
    m_event_loop_object = nullptr;
    m_delete_spy = nullptr;

    auto testinfo = ::testing::UnitTest::GetInstance()->current_test_info();
    auto test_id = testinfo->test_case_name() + std::string("_") + testinfo->name();
    GTEST_COUT << "TearDown() for test_id: " << test_id << std::endl;
    EXPECT_NO_FATAL_FAILURE({
                                expect_all_postconditions();
                            });
}

void ExtAsyncTestsSuiteFixtureBase::expect_all_postconditions()
{
    EXPECT_TRUE(m_interstate.check_generators()) << "Generators not cleaned up";
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
    auto testinfo = ::testing::UnitTest::GetInstance()->current_test_info();
    auto test_id = testinfo->test_case_name() + std::string("_") + testinfo->name();
    return test_id;
}

void ExtAsyncTestsSuiteFixtureBase::register_generator(trackable_generator_base *generator)
{
    ASSERT_TRUE(m_interstate.is_test_currently_running()) << "No test was running";
    m_interstate.register_generator(generator);

}

void ExtAsyncTestsSuiteFixtureBase::unregister_generator(trackable_generator_base *generator)
{
    ASSERT_TRUE(m_interstate.is_test_currently_running()) << "No test was running";
    m_interstate.unregister_generator(generator);
}



