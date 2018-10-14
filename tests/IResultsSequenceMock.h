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
 * @file IResultsSequenceMock.h
 */
#ifndef TESTS_IRESULTSSEQUENCEMOCK_H_
#define TESTS_IRESULTSSEQUENCEMOCK_H_

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*
 *
 */
class IResultsSequenceMock
{
public:
	IResultsSequenceMock() {};
	virtual ~IResultsSequenceMock() {};

	virtual int ReportResult(int int_result) = 0;
};

class ResultsSequenceMock : public IResultsSequenceMock
{
public:
	ResultsSequenceMock() {};
	~ResultsSequenceMock() override {}

	MOCK_METHOD1(ReportResult, int(int int_result));

};

/// @name Helper definietions/macros.
/// @{

#ifdef TEST_FWK_IS_GTEST
// Specialize an action that synchronizes with the calling thread.
// @link https://stackoverflow.com/questions/10767131/expecting-googlemock-calls-from-another-thread
ACTION_P2(ReturnFromAsyncCall, RetVal, SemDone)
{
	SemDone->release();
	return RetVal;
}

/// Call this at the start of a test function to get a ResultsSequenceMock setup.
#define TC_START_RSM(rsm_name) \
		ResultsSequenceMock rsm_name;\
		QSemaphore rsm_name##_sem_done(0);

/// Call this at the end of a test function using TC_START_RSM().
#define TC_END_RSM(rsm_name) \
do {\
	bool acquired = (rsm_name##_sem_done).tryAcquire(1, 1000);\
	EXPECT_TRUE(acquired);\
} while(0)
#else
#define TC_END_RSM(rsm_name, sem) /* No gtest... nothing? */
#endif

#define TC_RSM_EXPECT_CALL(rsm_name, enumerator) \
	EXPECT_CALL(rsm_name, ReportResult(enumerator)) \
		.WillOnce(ReturnFromAsyncCall(enumerator, & (rsm_name##_sem_done)));

/// @}

#endif /* TESTS_IRESULTSSEQUENCEMOCK_H_ */
