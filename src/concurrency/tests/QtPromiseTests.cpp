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

/**
 * @file QtPromiseTests.cpp
 */
#include "QtPromiseTests.h"

// Qt5
#include <QString>
#include <QTest>

// Google Test
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Ours
#include <tests/TestHelpers.h>
#include "ExtAsyncTestCommon.h"
#include <tests/IResultsSequenceMock.h>

// QtPromise (Brunel)
#include <src/third_party/qtpromise/include/QtPromise>

using namespace QtPromise;

TEST_F(QtPromiseTests, Sanity)
{
	TC_ENTER();

	int result = -1;
	auto p = qPromise(QtConcurrent::run([]() {
		return 42;
	}));

	Q_STATIC_ASSERT((std::is_same<decltype(p), QPromise<int>>::value));
	AMLMTEST_EXPECT_TRUE(p.isPending());

	p.then([&](int res) {
		result = res;
	}).wait();

	AMLMTEST_EXPECT_TRUE(p.isFulfilled());
	AMLMTEST_EXPECT_EQ(result, 42);

	TC_EXIT();
}

TEST_F(QtPromiseTests, DISABLED_ThenChainCancelFromEnd)
{
	TC_ENTER();

	auto tp = QThreadPool::globalInstance();

	TC_START_RSM(rsm);
	enum
	{
		MSTART,
		MEND,
		J1STARTCB,
		J1CANCELED,
		J1ENDCB,
		T1STARTCB,
		T1ENDCB,
	};

	using ::testing::InSequence;
	using ::testing::Return;
	using ::testing::Eq;
	using ::testing::ReturnArg;
	using ::testing::_;

	ON_CALL(rsm, ReportResult(_))
			.WillByDefault(ReturnArg<0>());
	{
		InSequence s;

		TC_RSM_EXPECT_CALL(rsm, MSTART);
		TC_RSM_EXPECT_CALL(rsm, J1STARTCB);
		TC_RSM_EXPECT_CALL(rsm, J1CANCELED);
		TC_RSM_EXPECT_CALL(rsm, J1ENDCB);
//		TC_RSM_EXPECT_CALL(rsm, T1STARTCB);
//		TC_RSM_EXPECT_CALL(rsm, T1ENDCB);
		TC_RSM_EXPECT_CALL(rsm, MEND);
	}

	std::atomic_bool ran_run_callback {false};
	std::atomic_bool ran_then1_callback {false};
	std::atomic_bool ran_then2_callback {false};

	///
	/// Setup done, test starts here.
	///

	rsm.ReportResult(MSTART);

	// Log the number of free threads in the thread pool.
	TCOUT << M_NAME_VAL(QThreadPool::globalInstance()->maxThreadCount());
	TCOUT << M_NAME_VAL(QThreadPool::globalInstance()->activeThreadCount());
	TCOUT << M_NAME_VAL(QThreadPool::globalInstance()->maxThreadCount() - QThreadPool::globalInstance()->activeThreadCount());

	ExtFuture<int> run_down;

	// The async task.  Spins forever, reporting "5" to run_down until canceled.
	QPromise<int> generator([&](const QPromiseResolve<int>& resolve, const QPromiseReject<int>& reject) {

		QtConcurrent::run([&/*, &ran_run_callback, &ran_then1_callback, &ran_then2_callback, &rsm, &run_down*/](ExtFuture<int> run_down_copy) {
			AMLMTEST_EXPECT_FALSE(ran_run_callback);
			AMLMTEST_EXPECT_FALSE(ran_then1_callback);
			AMLMTEST_EXPECT_FALSE(ran_then2_callback);
			ran_run_callback = true;

			// If we're not (Running|Started) here, something's wildly wrong.
			AMLMTEST_EXPECT_TRUE(run_down_copy.isStarted());
			AMLMTEST_EXPECT_TRUE(run_down_copy.isRunning());
	//		TCOUT << "IN RUN CALLBACK, run_down_copy:" << run_down_copy;
			AMLMTEST_EXPECT_EQ(run_down, run_down_copy);

			rsm.ReportResult(J1STARTCB);

			while(true)
			{
				// Wait one second before doing anything.
				TC_Sleep(1000);
				// Report a result to downstream.
				run_down_copy.reportResult(5);
				// Handle canceling.
				if(run_down_copy.HandlePauseResumeShouldICancel())
				{
					run_down_copy.reportCanceled();
					rsm.ReportResult(J1CANCELED);
					break;
				}
			}

			// We've been canceled, but not finished.
	//		AMLMTEST_ASSERT_TRUE(run_down_copy.isCanceled());
	//		AMLMTEST_ASSERT_FALSE(run_down_copy.isFinished());
			rsm.ReportResult(J1ENDCB);
			run_down_copy.reportFinished();

			if(run_down_copy.isCanceled())
			{
				reject(QPromiseCanceledException());
			}
			else
			{
				resolve(5);
			}

		}, run_down);

		AMLMTEST_EXPECT_FUTURE_STARTED_NOT_FINISHED_OR_CANCELED(run_down);
		AMLMTEST_EXPECT_FALSE(run_down.isCanceled()) << run_down;

		return run_down;
	});

	// Then 1
	auto down = generator.then([=, &ran_run_callback, &ran_then1_callback, &ran_then2_callback, &rsm, &generator]
										(int upstream_result) -> int {
		AMLMTEST_EXPECT_TRUE(ran_run_callback);
		AMLMTEST_EXPECT_FALSE(ran_then1_callback);
		AMLMTEST_EXPECT_FALSE(ran_then2_callback);
		ran_then1_callback = true;

		// No try.  This should throw to down.
//		auto results_from_upstream = upcopy.results();
//		ADD_FAILURE() << "We should never get in here on a cancelation.";
		// Immediately return.
//		TCOUT << "THEN1 RETURNING, future state:" << upcopy;
		return 5;
	});
	EXPECT_FALSE(down.isRejected());

	// Then 2
	auto down2 = down.then([=, &ran_run_callback, &ran_then1_callback, &ran_then2_callback, &rsm, &down]
									 (int upstream2_result) -> int {
		AMLMTEST_EXPECT_TRUE(ran_run_callback);
		AMLMTEST_EXPECT_TRUE(ran_then1_callback);
		AMLMTEST_EXPECT_FALSE(ran_then2_callback);
		ran_then2_callback = true;

		return 6;
	});
	AMLMTEST_EXPECT_FALSE(down2.isRejected());

	// Ok, both then()'s attached, less than a second before the promise sends its first result.

	// Wait a few ticks.
	TC_Sleep(1000);

	// Cancel the downstream future.
	TCOUT << "CANCELING TAIL:";// << down2;
//	down2.cancel();
//	down2.reportException(ExtAsyncCancelException());



	TCOUT << "CANCELED TAIL:";// << down2;

	TCOUT << "WAITING TO PROPAGATE";
	TC_Sleep(2000);
	TCOUT << "SHOULD HAVE PROPAGATED";

	AMLMTEST_EXPECT_TRUE(down2.isRejected()); //<< down2;
	AMLMTEST_EXPECT_TRUE(down.isRejected());// << down;
	AMLMTEST_EXPECT_TRUE(generator.isRejected());// << generator;

	AMLMTEST_EXPECT_TRUE(ran_run_callback);
	AMLMTEST_EXPECT_TRUE(ran_then1_callback);
	AMLMTEST_EXPECT_TRUE(ran_then2_callback);

//	try
//	{
//		QFutureSynchronizer<int> fs;
//		fs.addFuture(run_down);
//		fs.addFuture(down2);
//		fs.addFuture(down);
//		fs.waitForFinished();
//	}
//	catch(...)
//	{
//		TCOUT << "QFutureSync threw";
//	}


	rsm.ReportResult(MEND);

	TC_END_RSM(rsm);

	TC_EXIT();
}
