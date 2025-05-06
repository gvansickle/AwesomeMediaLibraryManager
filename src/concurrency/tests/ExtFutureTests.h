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

#ifndef AWESOMEMEDIALIBRARYMANAGER_EXTFUTURETESTS_H
#define AWESOMEMEDIALIBRARYMANAGER_EXTFUTURETESTS_H

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
class ExtFutureTest : public ExtAsyncTestsSuiteFixtureBase
{
	using BASE_CLASS = ExtAsyncTestsSuiteFixtureBase;

public:


protected:

};

template <class FutureTypeT>
class ExtFutureTypedTestFixture : public ExtFutureTest
{
	using BASE_CLASS = ExtFutureTest;

public:

    using List = std::list<FutureTypeT>;
    static FutureTypeT shared_;
    FutureTypeT value_;

protected:

};

#endif //AWESOMEMEDIALIBRARYMANAGER_EXTFUTURETEST_H
