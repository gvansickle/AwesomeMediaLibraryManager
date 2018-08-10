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
    auto test_id = fixture->get_test_id_string();
    m_generator_id = test_id;
}

ExtFuture<int> async_int_generator(int start_val, int num_iterations, ExtAsyncTestsSuiteFixtureBase *fixture)
{
    SCOPED_TRACE("In async_int_generator");

    auto tgb = new trackable_generator_base(fixture);
    fixture->register_generator(tgb);

    ExtFuture<int> retval = ExtAsync::run_efarg([&](ExtFuture<int>& future) {
            int current_val = start_val;
            SCOPED_TRACE("In async_int_generator callback");
            for(int i=0; i<num_iterations; i++)
    {
        // Sleep for a second.
        qIn() << "SLEEPING FOR 1 SEC";

        QTest::qSleep(1000);
        qIn() << "SLEEP COMPLETE, sending value to future:" << current_val;

        future.reportResult(current_val);
        current_val++;
    }

    // We're done.
    qIn() << "REPORTING FINISHED";
    fixture->unregister_generator(tgb);
    delete tgb;

    future.reportFinished();
});

static_assert(std::is_same_v<decltype(retval), ExtFuture<int>>, "");

qIn() << "RETURNING:" << retval;

return retval;
}

QString delayed_string_func_1(ExtAsyncTestsSuiteFixtureBase *fixture)
{
    SCOPED_TRACE("In delayed_string_func_1");

    auto tgb = new trackable_generator_base(fixture);
    fixture->register_generator(tgb);

    auto retval = QtConcurrent::run([&](){
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

    GTEST_COUT << "delayed_string_func_1() returning" << tostdstr(retval) << std::endl;

    return retval.result();
}

/// ExtAsyncTestsSuiteFixtureBase

void ExtAsyncTestsSuiteFixtureBase::SetUp()
{
    SCOPED_TRACE("In SetUp()");
    auto testinfo = ::testing::UnitTest::GetInstance()->current_test_info();
    GTEST_COUT << "SetUp() for test: " << testinfo->name() << ", test case: " << testinfo->test_case_name() << std::endl;
    starting(get_test_id_string());
}

void ExtAsyncTestsSuiteFixtureBase::TearDown()
{
    SCOPED_TRACE("In TearDown()");
    auto testinfo = ::testing::UnitTest::GetInstance()->current_test_info();
    auto test_id = testinfo->test_case_name() + std::string("_") + testinfo->name();
    GTEST_COUT << "TearDown() for test: " << testinfo->name() << ", test case: " << testinfo->test_case_name() << std::endl;
    EXPECT_EQ(m_generator_stack.size(), 0) << "Generator was not unregistered. Top generator ptr: " << m_generator_stack.top()
                                           << "Top generator ID: " << m_generator_stack.top()->get_generator_id();
    finished(get_test_id_string());
}

std::string ExtAsyncTestsSuiteFixtureBase::get_currently_running_test()
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
    return m_currently_running_test;
}

void ExtAsyncTestsSuiteFixtureBase::starting(std::string func)
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
    m_currently_running_test = func;
}

void ExtAsyncTestsSuiteFixtureBase::finished(std::string func)
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
    m_finished_set.insert(func);
    m_currently_running_test.clear();
}

bool ExtAsyncTestsSuiteFixtureBase::has_finished(std::string func)
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
    return m_finished_set.count(func) > 0;
}

std::string ExtAsyncTestsSuiteFixtureBase::get_test_id_string()
{
    auto testinfo = ::testing::UnitTest::GetInstance()->current_test_info();
    auto test_id = testinfo->test_case_name() + std::string("_") + testinfo->name();
    return test_id;
}

void ExtAsyncTestsSuiteFixtureBase::register_generator(trackable_generator_base *generator)
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
    SCOPED_TRACE("register_generator");
    m_generator_stack.push(generator);
}

void ExtAsyncTestsSuiteFixtureBase::unregister_generator(trackable_generator_base *generator)
{
    std::lock_guard<std::mutex> lock(m_fixture_state_mutex);
    SCOPED_TRACE("unregister_generator");
    auto tgen = m_generator_stack.top();
    EXPECT_EQ(tgen, generator) << "Unregistering incorrect generator";
    m_generator_stack.pop();
}

