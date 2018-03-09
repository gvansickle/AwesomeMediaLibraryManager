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
#include "../future_type_traits.hpp"
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

/**
 * From a lambda passed to QtConcurrent::run(), sleeps for 1 sec and then returns a single QString.
 * @return
 */
static QString delayed_string_func_1()
{
	auto retval = QtConcurrent::run([](){
		// Sleep for a second.
		qDb() << "ENTER, SLEEPING FOR 1 SEC";
		QThread::sleep(1);
		qDb() << "SLEEP COMPLETE";
		return QString("delayed_string_func_1() output");
	});

	return retval.result();
}

/**
 * From a lambda passed to QtConcurrent::run(), sleeps for 1 sec and then returns a single QString.
 * @return
 */
static ExtFuture<QString> delayed_string_func()
{
	auto retval = QtConcurrent::run([](){
		// Sleep for a second.
		qDb() << "ENTER, SLEEPING FOR 1 SEC";
		QThread::sleep(1);
		qDb() << "SLEEP COMPLETE";
		return QString("HELLO");
	});

	static_assert(std::is_same_v<decltype(retval), QFuture<QString>>, "");

	return retval;
}

/**
 * From a lambda passed to ExtAsync::run(), iterates @a num_iteration times,
 * sleep()ing for 1 sec, then returns the the next value in the sequence to the returned ExtFuture<>.
 *
 * @todo Doesn't handle cancellation or progress reporting.
 */
static ExtFuture<int> async_int_generator(int start_val, int num_iterations)
{
	ExtFuture<int> retval = ExtAsync::run([=](ExtFuture<int>& future) {
		int current_val = start_val;
		for(int i=0; i<num_iterations; i++)
		{
			// Sleep for a second.
			qDb() << "SLEEPING FOR 1 SEC";
			QThread::sleep(1);
			qDb() << "SLEEP COMPLETE, returning value to future:" << current_val;
			future.reportResult(current_val);
		}
		// We're done.
		future.reportFinished();
	});

	static_assert(std::is_same_v<decltype(retval), ExtFuture<int>>, "");

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

TEST_F(AsyncTestsSuiteFixture, ExtFuture_ExtAsyncRun_multi_result_test)
{
	int last_seen_result = 0;
	int num_then_calls = 0;

	// Start generating a sequence of results.
	auto future = async_int_generator(5, 3);

	ASSERT_TRUE(future.isStarted());
	ASSERT_FALSE(future.isFinished());

	// Separated .then() connect.
	future.tap([&](int future_value) {
		if(num_then_calls == 0)
		{
			EXPECT_EQ(last_seen_result, 0);
		}

		int expected_future_val = 5 + num_then_calls;
		EXPECT_EQ(expected_future_val, future_value);
		last_seen_result = future_value;
		num_then_calls++;
		;});
#if 0
		.finally([&]() {
			EXPECT_EQ(num_then_calls, 3);
			EXPECT_EQ(last_seen_result, 7);
		;});
#endif
}


TEST_F(AsyncTestsSuiteFixture,TestReadyFutures)
{
	ExtFuture<int> future = make_ready_future(45);
	ASSERT_TRUE(future.isStarted());
	ASSERT_TRUE(future.isFinished());
	ASSERT_EQ(future.get(), 45);
}



TEST_F(AsyncTestsSuiteFixture, DISABLED_UnwrapTest)
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

TEST_F(AsyncTestsSuiteFixture, TapAndThen_OneResult)
{

	bool ran_tap = false;
	bool ran_then = false;

	ExtFuture<QString> future = ExtAsync::run(delayed_string_func_1);

	GTEST_COUT << "Future created" << std::endl;

	future.tap([&](QString result){
			EXPECT_EQ(result, QString("delayed_string_func_1() output"));
			ran_tap = true;
			EXPECT_FALSE(ran_then);
		;})
		.then([&](ExtFuture<QString> extfuture){
			EXPECT_EQ(extfuture.get(), QString("delayed_string_func_1() output"));
			EXPECT_TRUE(ran_tap);
			EXPECT_FALSE(ran_then);
			ran_then = true;
			return QString("Then Called");
		;}).wait();

//	future.wait();

	ASSERT_TRUE(ran_tap);
	ASSERT_TRUE(ran_then);
}

TEST_F(AsyncTestsSuiteFixture, TapAndThen_MultipleResults)
{
	int tap_call_counter = 0;

	ExtFuture<int> future = ExtAsync::run([&](ExtFuture<int>& extfuture) {
			GTEST_COUT << "TEST: Running from main run lambda." << std::endl;
			// Sleep for a second to make sure then() doesn't run before we get to the Q_ASSERT() after this chain.
			QThread::sleep(1);
			extfuture.reportResult(867);
			QThread::sleep(1);
			extfuture.reportResult(5309);
			QThread::sleep(1);
			extfuture.reportFinished();
			GTEST_COUT << "TEST: Finished from main run lambda." << std::endl;
		})
	.tap([&](int value){
		if(tap_call_counter == 0)
		{
			EXPECT_EQ(value, 867);
		}
		else if(tap_call_counter == 1)
		{
			EXPECT_EQ(value, 5309);
		}
		else
		{
			EXPECT_EQ(tap_call_counter, 1);
		}
		tap_call_counter++;
		;});

	ASSERT_FALSE(future.isFinished());
	future.wait();
	ASSERT_TRUE(future.isFinished());
}

/// Static checks
void dummy(void)
{

	static_assert(std::is_default_constructible<QString>::value, "");

	// From http://en.cppreference.com/w/cpp/experimental/make_ready_future:
	// "If std::decay_t<T> is std::reference_wrapper<X>, then the type V is X&, otherwise, V is std::decay_t<T>."
	static_assert(std::is_same_v<decltype(make_ready_future(4)), ExtFuture<int> >, "");
	int v;
	static_assert(std::is_same_v<decltype(make_ready_future(std::ref(v))), ExtFuture<int&> >, "");
	/// @todo
//	static_assert(std::is_same_v<decltype(make_ready_future()), ExtFuture<void> >, "");

}



