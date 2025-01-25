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

#ifndef TESTS_TESTHELPERS_H_
#define TESTS_TESTHELPERS_H_

#include <config.h>

// Std C++
#include <sstream>

// Qt5
#include <Qt>
#include <QTest>
#include <QString>

// Determine if we're being used in a QTest or Google Test build.
#include <utils/DebugHelpers.h>
#if defined(TEST_FWK_IS_GTEST)
	M_MESSAGE("Building for Google Test framework");
	// Include Google Test / Mock headers
	#include <gtest/gtest.h>
	#include <gmock/gmock.h>

	// We need a threadsafe googletest build.
#	if !defined(GTEST_IS_THREADSAFE) || (GTEST_IS_THREADSAFE != 1)
#		error "GTEST NOT THREADSAFE"
#	endif

#elif defined(TEST_FWK_IS_QTEST)
	M_MESSAGE("Building for QTest framework");
#else
#error "No test framework defined"
#endif
#if defined(TEST_FWK_IS_QTEST) && defined(TEST_FWK_IS_GTEST)
#error "BOTH TEST FRAMEWORKS DEFINED"
#endif

///
/// @start The TEST_FWK_IS_GTEST/QTEST agnostic common section.
///

// Ours
#include <src/concurrency/ExtFuture.h>


/// Global divisor for ms delays/timeouts in the tests.
constexpr long TC_MS_DIV = 10;

static inline void TC_Sleep(int ms)
{
	QTest::qSleep(ms / TC_MS_DIV);
}

static inline void TC_Wait(int ms)
{
	QTest::qWait(ms / TC_MS_DIV);
}


///
/// @end The TEST_FWK_IS_GTEST/QTEST agnostic common section.
///


#if defined(TEST_FWK_IS_GTEST)
///
/// The TEST_FWK_IS_GTEST section.
///

QT_BEGIN_NAMESPACE

/// To let Google Test print QStrings.
inline void PrintTo(const QString &qString, ::std::ostream *os)
{
    *os << qUtf8Printable(qString);
}

/// To let Google Test print ExtFutures.
template <class T>
inline void PrintTo(const ExtFuture<T> &ef, ::std::ostream *os)
{
    QString str;
    QDebug dbg(&str);
    dbg << ef;
    PrintTo(str, os);
}

/// To let Google Test print QFutures.
template <class T>
inline void PrintTo(const QFuture<T> &qf, ::std::ostream *os)
{
	QString str;
	QDebug dbg(&str);
	dbg << toString(ExtFutureState::state(qf));
	PrintTo(str, os);
}

QT_END_NAMESPACE


/// Quick and dirty way to add information to the test log.
#define GTEST_COUT_ORIGINAL std::cout << "[          ] [ INFO ] "
#define GTEST_COUT GTEST_COUT_ORIGINAL

#if 0 ///@todo To get past compile errors: error: ‘ColoredPrintf’ is not a member of ‘testing::internal’
/// @name Hopefully less quick-and-dirty way to add information to test output.
/// Based on @link https://stackoverflow.com/a/45344932
/// @{
/// @note The link makes it look like this should all be here, but it's exposed by gtest.h.
///       Leaving it here for reference to the color enumerators if nothing else.
namespace testing
{
	namespace internal
	{
		enum GTestColor
		{
			COLOR_DEFAULT,
			COLOR_RED,
			COLOR_GREEN,
			COLOR_YELLOW
		};
		extern void ColoredPrintf(GTestColor color, const char* fmt, ...);
	} // namespace internal
} // namespace testing

/**
 * Class which provides a QDebug output stream to log arbitrary Qt messages to.
 * Messages are written to a QString and then sent to the Google Test logging infrastructure for actual output.
 * @note Do not use this class directly, use the TCOUT macro below.
 */
class TestCout
{
	QString m_log_string {};
	QDebug m_qdebug_obj;

public:
	/// Constructor.
	TestCout() : m_qdebug_obj(&m_log_string)	{ };
    ~TestCout()
    {
		// Send any message that we received to Google Test's logging innards.
		testing::internal::ColoredPrintf(testing::internal::COLOR_GREEN, "[          ] ");
		testing::internal::ColoredPrintf(testing::internal::COLOR_YELLOW, "%s\n", m_log_string.toStdString().c_str());
    }

	QDebug& getQDebugRef()
	{
		return m_qdebug_obj;
	}
};
#endif

/// Macro which exposes a new TestCout object to stream messages to, e.g.:
/// @code
///     TCOUT << "This is a test";
/// @endcode
//#define TCOUT  TestCout().getQDebugRef()
#define TCOUT  qDb()

/// @}

#endif /// END #if defined(TEST_FWK_IS_GTEST)


enum class GenericState
{
    NOT_STARTED,
    STARTED,
    FINISHED
};

///

#if defined(TEST_FWK_IS_QTEST) /// QTest framework.

///
/// The TEST_FWK_IS_QTEST section.
///

#define AMLMTEST_SCOPED_TRACE(str) /* nothing */

#define AMLMTEST_COUT qDb()
#define TCOUT qDb()

/// @todo QVERIFY() does a "return;", so we can't use it in a function returning a value.

#define AMLMTEST_EXPECT_TRUE(arg) QVERIFY(arg)
#define AMLMTEST_EXPECT_FALSE(arg) QVERIFY(!(arg))
#define AMLMTEST_ASSERT_TRUE(arg) QVERIFY(arg)
#define AMLMTEST_ASSERT_FALSE(arg) QVERIFY(!(arg))
#define AMLMTEST_EXPECT_EQ(arg1, arg2) QCOMPARE(arg1, arg2)
#define AMLMTEST_ASSERT_EQ(arg1, arg2) QCOMPARE(arg1, arg2)
#define AMLMTEST_ASSERT_NE(arg1, arg2) QVERIFY((arg1) != (arg2))
#define AMLMTEST_ASSERT_GT(arg1, arg2) ???ASSERT_GT(arg1, arg2)
#define AMLMTEST_ASSERT_GE(arg1, arg2) ??ASSERT_GE(arg1, arg2)
#define AMLMTEST_ASSERT_LT(arg1, arg2) ??ASSERT_LT(arg1, arg2)
#define AMLMTEST_ASSERT_LE(arg1, arg2) ??ASSERT_LE(arg1, arg2)
#define AMLMTEST_ASSERT_STREQ(actual, expected) QCOMPARE(actual, expected)
#define AMLMTEST_EXPECT_STREQ(actual, expected) QCOMPARE(actual, expected)

#define AMLMTEST_EXPECT_NO_FATAL_FAILURE(...) __VA_ARGS__

#elif defined(TEST_FWK_IS_GTEST)

///
/// Google Test Framework
///

#define AMLMTEST_SCOPED_TRACE(str) SCOPED_TRACE(str)

#define AMLMTEST_COUT TCOUT
//#define TCOUT qDb()

#define AMLMTEST_EXPECT_TRUE(arg) EXPECT_TRUE(arg)
#define AMLMTEST_EXPECT_FALSE(arg) EXPECT_FALSE(arg)
#define AMLMTEST_ASSERT_TRUE(arg) ASSERT_TRUE(arg)
#define AMLMTEST_ASSERT_FALSE(arg) ASSERT_FALSE(arg)
#define AMLMTEST_EXPECT_EQ(arg1, arg2) EXPECT_EQ((arg1), (arg2))
#define AMLMTEST_ASSERT_EQ(arg1, arg2) ASSERT_EQ((arg1), (arg2))
#define AMLMTEST_ASSERT_NE(arg1, arg2) ASSERT_NE((arg1), (arg2))
#define AMLMTEST_EXPECT_GT(arg1, arg2) EXPECT_GT((arg1), (arg2))
#define AMLMTEST_EXPECT_GE(arg1, arg2) EXPECT_GE((arg1), (arg2))
#define AMLMTEST_EXPECT_LT(arg1, arg2) EXPECT_LT((arg1), (arg2))
#define AMLMTEST_EXPECT_LE(arg1, arg2) EXPECT_LE((arg1), (arg2))
#define AMLMTEST_ASSERT_STREQ(actual, expected) ASSERT_STREQ((actual), (expected))
#define AMLMTEST_EXPECT_STREQ(actual, expected) EXPECT_STREQ((actual), (expected))

#define AMLMTEST_EXPECT_NO_FATAL_FAILURE(...) EXPECT_NO_FATAL_FAILURE(__VA_ARGS__)
#define AMLMTEST_ASSERT_NO_FATAL_FAILURE(...) ASSERT_NO_FATAL_FAILURE(__VA_ARGS__)

#elif defined(TEST_FWK_IS_QTEST) && defined(TEST_FWK_IS_GTEST)
#error "BOTH TEST FRAMEWORKS"
#else
#error "NO TEST FRAMEWORK"
#endif

#endif /* TESTS_TESTHELPERS_H_ */

