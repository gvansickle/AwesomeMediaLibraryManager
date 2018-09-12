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

// Std C++
#include <sstream>

// Qt5
#include <Qt>
#include <QTest>
#include <QString>

// Google Test
//#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>

// Ours
#include <src/concurrency/ExtFuture.h>


// Determine if we're being used in a QTest or Google Test build.
#if defined(TEST_FWK_IS_GTEST)
#warning "GTEST"
#elif defined(TEST_FWK_IS_QTEST)
#warning "QTEST"
#else
#error "No test framework defined"
#endif


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

QT_END_NAMESPACE

/// Divisor for ms delays/timeouts in the tests.
constexpr long TC_MS_DIV = 10;

static inline void TC_Sleep(int ms)
{
    QTest::qSleep(ms / TC_MS_DIV);
}

static inline void TC_Wait(int ms)
{
    QTest::qWait(ms / TC_MS_DIV);
}

/// Quick and dirty way to add information to the test log.
#define GTEST_COUT_ORIGINAL std::cout << "[          ] [ INFO ] "
#define GTEST_COUT GTEST_COUT_ORIGINAL

/// @name Hopefully less quick-and-dirty way to add information to test output.
/// @{
//namespace testing
//{
//    namespace internal
//    {
//    enum GTestColor {
//        COLOR_DEFAULT,
//        COLOR_RED,
//        COLOR_GREEN,
//        COLOR_YELLOW
//    };

/// @warning This is static in gtest now.
//    extern void ColoredPrintf(GTestColor color, const char* fmt, ...);
//    } // namespace internal
//} // namespace testing

//#define PRINTF(...)  do { testing::internal::ColoredPrintf(testing::internal::COLOR_GREEN, "[          ] "); testing::internal::ColoredPrintf(testing::internal::COLOR_YELLOW, __VA_ARGS__); } while(0)

//// C++ stream interface
//class TestCout : public std::stringstream
//{
//public:
//    ~TestCout()
//    {
//        PRINTF("%s", str().c_str());
//    }
//};

//#define TEST_COUT  TestCout()

/// @}

enum class GenericState
{
    NOT_STARTED,
    STARTED,
    FINISHED
};

///

#if defined(TEST_FWK_IS_QTEST)

// QTest framework.

#define SCOPED_TRACE(str) /* nothing */

#define AMLMTEST_COUT qDb()

#define AMLMTEST_EXPECT_TRUE(arg) QVERIFY(arg)
#define AMLMTEST_EXPECT_FALSE(arg) QVERIFY(!(arg))
#define AMLMTEST_ASSERT_TRUE(arg) QVERIFY(arg)
#define AMLMTEST_ASSERT_FALSE(arg) QVERIFY(!(arg))
#define AMLMTEST_EXPECT_EQ(arg1, arg2) QCOMPARE(arg1, arg2)
#define AMLMTEST_ASSERT_EQ(arg1, arg2) QCOMPARE(arg1, arg2)
#define AMLMTEST_ASSERT_NE(arg1, arg2) QVERIFY((arg1) != (arg2))


/// Macros for making sure a KJob gets destroyed before the TEST_F() returns.
#define M_QSIGNALSPIES_SET(kjobptr) \
	QSignalSpy kjob_finished_spy(kjobptr, &KJob::finished); \
	AMLMTEST_EXPECT_TRUE(kjob_finished_spy.isValid()); \
	QSignalSpy kjob_result_spy(kjobptr, &KJob::result); \
	AMLMTEST_EXPECT_TRUE(kjob_result_spy.isValid()); \
	QSignalSpy kjob_destroyed_spy(kjobptr, SIGNAL(destroyed(QObject*))); \
	AMLMTEST_EXPECT_TRUE(kjob_destroyed_spy.isValid()); \
	QSignalSpy kjob_destroyed_spy2(kjobptr, SIGNAL(destroyed())); \
	AMLMTEST_EXPECT_TRUE(kjob_destroyed_spy2.isValid());

#define M_QSIGNALSPIES_EXPECT_IF_DESTROY_TIMEOUT() \
	AMLMTEST_EXPECT_TRUE(kjob_destroyed_spy.wait() || kjob_destroyed_spy2.wait());

#elif defined(TEST_FWK_IS_GTEST)
#warning "GTEST"

#define AMLMTEST_EXPECT_TRUE(arg) EXPECT_TRUE(arg)


#else
#warning "TODO: IF NEITHER"
#endif

#endif /* TESTS_TESTHELPERS_H_ */

