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

#include <QObject>

#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>


QT_BEGIN_NAMESPACE
inline void PrintTo(const QString &qString, ::std::ostream *os)
{
    *os << qUtf8Printable(qString);
}
QT_END_NAMESPACE

/**
 * Test Suite (ISTQB) or "Test Case" (Google) for AsyncTests.
 */
class AsyncTestsSuiteFixture : public ::testing::Test
{
protected:

	void SetUp() override;
	void TearDown() override;

	// Objects declared here can be used by all tests in this Fixture.
	int m_example;
};


/*
 *
 */
class AsyncTests: public QObject
{
	Q_OBJECT

private Q_SLOTS:

	void initTestCase();
	void test1();
	void test2();
	void cleanupTestCase();

	void RunAllTests();

	void TestReadyFutures();

	void ExtFutureThenChainingTest();

	void UnwrapTest();
};


#endif /* UTILS_CONCURRENCY_TESTS_ASYNCTESTS_H_ */
