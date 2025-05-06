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

// Qt
#include <QObject>

// Google Test
#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>

// Ours
//#include <tests/TestHelpers.h>
#include "ExtAsyncTestCommon.h"

/**
 * Test Suite (ISTQB) or "Test Case" (Google) for ExtAsyncTests.
 */
class ExtAsyncTestsSuiteFixture : public ExtAsyncTestsSuiteFixtureBase
{
//public:
//    // For type-parameterized tests.
//    using List = std::list<T>;
//    static T shared_;
//    T value_;

protected:

	// Objects declared here can be used by all tests in this Fixture.


};

class ExtAsyncTestsParameterized : public ExtAsyncTestsSuiteFixture,
		public ::testing::WithParamInterface<bool>
{

};


#endif /* UTILS_CONCURRENCY_TESTS_ASYNCTESTS_H_ */
