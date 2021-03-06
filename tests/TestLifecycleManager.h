#ifndef TESTLIFECYCLEMANAGER_H
#define TESTLIFECYCLEMANAGER_H

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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef TEST_FWK_IS_GTEST
#if !defined(GTEST_IS_THREADSAFE) || (GTEST_IS_THREADSAFE != 1)
#error "GTEST NOT THREADSAFE"
#endif
#endif

class ITestLifecycleManager
{
public:
	ITestLifecycleManager() {}
	virtual ~ITestLifecycleManager() {}

	virtual void MTC_ENTER() = 0;
	virtual void MTC_EXIT() = 0;

};


class TestLifecycleManager : public ITestLifecycleManager
{
public:
	TestLifecycleManager();
	~TestLifecycleManager() override {}

	MOCK_METHOD0(MTC_ENTER, void());
	MOCK_METHOD0(MTC_EXIT, void());
};

#endif // TESTLIFECYCLEMANAGER_H
