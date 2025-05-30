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

#ifndef SRC_CONCURRENCY_TESTS_AMLMJOBTESTS_H_
#define SRC_CONCURRENCY_TESTS_AMLMJOBTESTS_H_

// Std C++.
#include <map>
#include <string>
#include <mutex>

// Qt
#include <QObject>


#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>

// Ours
//#include <tests/TestHelpers.h>
#include "ExtAsyncTestCommon.h"


/**
 * Test Suite (ISTQB) or "Test Case" (Google) for AMLMJobTests.
 */
class AMLMJobTests : public ExtAsyncTestsSuiteFixtureBase
{
protected:

	// Objects declared here can be used by all tests in this Fixture.

};

class AMLMJobTestsParameterized : public AMLMJobTests,
        public ::testing::WithParamInterface<bool>
{

};

#endif /* SRC_CONCURRENCY_TESTS_AMLMJOBTESTS_H_ */
