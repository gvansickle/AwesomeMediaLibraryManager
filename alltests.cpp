/*
 * alltests.cpp
 *
 *  Created on: Feb 23, 2018
 *      Author: gary
 */

#include <QApplication>
#include <QTimer>
//#include <QtTest>

//#include <utils/concurrency/tests/AsyncTests.h>

#include <gtest/gtest.h>

////
//#include <gmock/gmock-matchers.h>

QT_BEGIN_NAMESPACE
inline void PrintTo(const QString &qString, ::std::ostream *os)
{
    *os << qUtf8Printable(qString);
}
QT_END_NAMESPACE

/// @note main() mods to support Qt5 threading etc. testing per Stack Overflow: https://stackoverflow.com/a/33829950

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	::testing::InitGoogleTest(&argc, argv);
	auto retval = RUN_ALL_TESTS();

	// Timer-based exit from app.exec().
	QTimer exitTimer;
	QObject::connect(&exitTimer, &QTimer::timeout, &app, QCoreApplication::quit);
	exitTimer.start();
	app.exec();

	return retval;
}

