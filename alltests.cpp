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

#include <QApplication>
#include <QTimer>
//#include <QtTest>

#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>
#include <tests/TestHelpers.h>

#if !defined(GTEST_IS_THREADSAFE) || (GTEST_IS_THREADSAFE != 1)
#error "Google Test wasn't compiled for a multithreaded environment."
#endif

/**
 * Override of Environment for global setup and teardown.
 */
class StartAndFinish : public ::testing::Environment
{
public:
	virtual ~StartAndFinish() {}

	// Print the start message.
	void SetUp() override { 	GTEST_COUT << "TEST STARTING" << std::endl; }

	// Print the finished message.
	void TearDown() override { 	GTEST_COUT << "TEST COMPLETE" << std::endl; }
};

/// @note main() mods to support Qt5 threading etc. testing per Stack Overflow: https://stackoverflow.com/a/33829950

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	::testing::InitGoogleTest(&argc, argv);

	// Create a new environment object and register it with gtest.
	// Don't delete it, gtest takes ownership.
	::testing::AddGlobalTestEnvironment(new StartAndFinish());

	auto retval = RUN_ALL_TESTS();

	// Timer-based exit from app.exec().
	QTimer exitTimer;
	QObject::connect(&exitTimer, &QTimer::timeout, &app, QCoreApplication::quit);
	exitTimer.start();
	app.exec();

	return retval;
}
