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

#include "AsyncTests.h"

#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>
#include <tests/TestHelpers.h>

#include <QString>
#include <QTest>

#include <type_traits>
#include "../function_traits.hpp"

#include "../ExtAsync.h"


void AsyncTestsSuiteFixture::SetUp()
{
	GTEST_COUT << "SetUp()" << std::endl;
}

void AsyncTestsSuiteFixture::TearDown()
{
	GTEST_COUT << "TearDown()" << std::endl;
}



///
/// Test Cases
///

#if 0
TEST_F(AsyncTestsSuiteFixture, ThisShouldFail)
{
	ASSERT_TRUE(false);
}
#endif

TEST_F(AsyncTestsSuiteFixture, ThisShouldPass)
{
	ASSERT_TRUE(true);
}

TEST_F(AsyncTestsSuiteFixture, QStringPrintTest)
{
	QString test = "Test";
	ASSERT_EQ(test, "Test");
}

static QString delayed_string_func_1()
{
	auto retval = QtConcurrent::run([](){
		// Sleep for a second.
		qDb() << "ENTER, SLEEPING FOR 1 SEC";
		QThread::sleep(1);
		qDb() << "SLEEP COMPLETE";
		return QString("delayed_string_func_1() output");
	});

	return retval;
}

static ExtFuture<QString> delayed_string_func()
{
	auto retval = QtConcurrent::run([](){
		// Sleep for a second.
		qDb() << "ENTER, SLEEPING FOR 1 SEC";
		QThread::sleep(1);
		qDb() << "SLEEP COMPLETE";
		return QString("HELLO");
	});

	return retval;
}

TEST_F(AsyncTestsSuiteFixture, ExtFutureThenChainingTest_ExtFutures)
{
	SCOPED_TRACE("START");
//	qIn() << "START";

	bool ran1 = false;
	bool ran2 = false;
	bool ran3 = false;

	ExtFuture<QString> future = ExtAsync::run(delayed_string_func_1);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	future
	.then([&](ExtFuture<QString> extfuture) -> QString {
		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		qDb() << "Then1, got extfuture:" << extfuture;
		qDb() << "Then1, extfuture val:" << extfuture.get();
		EXPECT_EQ(ran1, false);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran1 = true;
		QString the_str = extfuture.get();
		EXPECT_EQ(the_str, QString("delayed_string_func_1() output"));
		return QString("Then1 OUTPUT");
	})
	.then([&](ExtFuture<QString> extfuture) -> QString {
		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		qDb() << "Then2, got extfuture:" << extfuture;
		qDb() << "Then2, extfuture val:" << extfuture.get();
		EXPECT_EQ(ran1, true);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran2 = true;
		auto the_str = extfuture.get();
		EXPECT_EQ(the_str, QString("Then1 OUTPUT"));
		return QString("Then2 OUTPUT");
	})
	.then([&](ExtFuture<QString> extfuture) -> QString {
		EXPECT_TRUE(extfuture.isFinished()) << "C++ std semantics are that the future is finished when the continuation is called.";
		qDb() << "Then3, got extfuture:" << extfuture;
		qDb() << "Then3, extfuture val:" << extfuture.get();
		EXPECT_EQ(ran1, true);
		EXPECT_EQ(ran2, true);
		EXPECT_EQ(ran3, false);
		ran3 = true;
		return QString("Then3 OUTPUT");
	}).wait();

	ASSERT_TRUE(future.isFinished());

	qDb() << "STARING WAIT";
	/// @todo This doesn't wait here, but the attached wait() above does. Which maybe makes sense.
	future.wait();
	qDb() << "ENDING WAIT";

	ASSERT_TRUE(ran1);
	ASSERT_TRUE(ran2);
	ASSERT_TRUE(ran3);

	qIn() << "Complete";
	SUCCEED();
	RecordProperty("Completed", true);
}

TEST_F(AsyncTestsSuiteFixture, ExtFutureThenChainingTest_MixedTypes)
{
//	qIn() << "START";

	bool ran1 = false;
	bool ran2 = false;
	bool ran3 = false;

	ExtFuture<QString> future = ExtAsync::run(delayed_string_func_1);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	future
	.then([&](ExtFuture<QString> extfuture) -> int {
		qDb() << "Then1, got val:" << extfuture.get();
		EXPECT_EQ(ran1, false);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran1 = true;
		QString the_str = extfuture.get();
		EXPECT_EQ(the_str, QString("delayed_string_func_1() output"));
		return 2;
	})
	.then([&](ExtFuture<int> extfuture) -> int {
		qDb() << "Then2, got val:" << extfuture.get();
		EXPECT_EQ(ran1, true);
		EXPECT_EQ(ran2, false);
		EXPECT_EQ(ran3, false);
		ran2 = true;
		EXPECT_EQ(extfuture.get(), 2);
		return 3;
	})
	.then([&](ExtFuture<int> extfuture) -> double {
		qDb() << "Then3, got val:" << extfuture.get();
		EXPECT_EQ(ran1, true);
		EXPECT_EQ(ran2, true);
		EXPECT_EQ(ran3, false);
		ran3 = true;
		EXPECT_EQ(extfuture.get(), 3);
		return 3.1415;
	}).wait();

	ASSERT_TRUE(future.isFinished());

	qDb() << "STARING WAIT";
	/// @todo This doesn't wait here, but the attached wait() above does. Which maybe makes sense.
	future.wait();
	qDb() << "ENDING WAIT";

	ASSERT_TRUE(ran1);
	ASSERT_TRUE(ran2);
	ASSERT_TRUE(ran3);

	qIn() << "Complete";
	SUCCEED();
	RecordProperty("Completed", true);
}

TEST_F(AsyncTestsSuiteFixture,TestReadyFutures)
{
	ExtFuture<int> future = make_ready_future(45);
	ASSERT_TRUE(future.isStarted());
	ASSERT_TRUE(future.isFinished());
	ASSERT_EQ(future.get(), 45);
}



TEST_F(AsyncTestsSuiteFixture, UnwrapTest)
{

//	auto future = QtConcurrent::run(delayed_string_func);
	ExtFuture<ExtFuture<QString>> future = ExtAsync::run(delayed_string_func);
//	ExtFuture<QString> future = ExtAsync::run([&](ExtFuture<QString> future) {
//		qDb() << "TEST: Running from main run lambda.";
//		// Sleep for a second to make sure then() doesn't run before we get to the Q_ASSERT() after this chain.
//		QThread::sleep(1);
//		future.reportResult("Hello1");
//		future.reportResult("Hello2");
//		future.reportFinished(new QString("FINISHED"));
//		qDb() << "TEST: Finished from main run lambda.";
//		return ExtFuture<QString>(); //("FINISHED");
//	});

//	ExtFuture<QString> unwrapped_future = future.unwrap();
}

/// Static checks
void dummy(void)
{
	// From http://en.cppreference.com/w/cpp/experimental/make_ready_future:
	// "If std::decay_t<T> is std::reference_wrapper<X>, then the type V is X&, otherwise, V is std::decay_t<T>."
	static_assert(std::is_same_v<decltype(make_ready_future(4)), ExtFuture<int> >, "");
	int v;
	static_assert(std::is_same_v<decltype(make_ready_future(std::ref(v))), ExtFuture<int&> >, "");
	/// @todo
//	static_assert(std::is_same_v<decltype(make_ready_future()), ExtFuture<void> >, "");

}



