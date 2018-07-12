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

#ifndef UTILS_CONCURRENCY_TESTS_ASYNCTESTS_H_
#define UTILS_CONCURRENCY_TESTS_ASYNCTESTS_H_

// Std C++
#include <map>
#include <string>
#include <mutex>

// Qt5
#include <QObject>

// Google Test
#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>

// Ours
#include <tests/TestHelpers.h>

/**
 * Test Suite (ISTQB) or "Test Case" (Google) for ExtAsyncTests.
 */
class ExtAsyncTestsSuiteFixture : public ::testing::Test
{
protected:

	void SetUp() override;
	void TearDown() override;

	// Objects declared here can be used by all tests in this Fixture.

	/// Map of test cases which have finished.
	std::set<std::string> m_finished_set;
	std::mutex m_finished_map_mutex;

	bool has_finished(std::string func)
	{
		std::lock_guard<std::mutex> lock(m_finished_map_mutex);
		return m_finished_set.count(func) > 0;

	}

	void finished(std::string func)
	{
		std::lock_guard<std::mutex> lock(m_finished_map_mutex);
		m_finished_set.insert(func);
	}
};


#endif /* UTILS_CONCURRENCY_TESTS_ASYNCTESTS_H_ */
