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

// Qt5
#include <QApplication>
#include <QLoggingCategory>
#include <QTimer>
#include <QDebug>
#include <QtTest>

// Ours
#include "utils/DebugHelpers.h"
#include "utils/StringHelpers.h"
#include "utils/RegisterQtMetatypes.h"
#include "resources/VersionInfo.h"
#include "utils/Logging.h"
#include <tests/TestHelpers.h>

// Google Test
#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>

// Make sure we've compiled correctly.
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
	Logging logging;

	logging.SetFilterRules();

	logging.InstallMessageHandler();

	// Create the Qt5 app.
	QApplication app(argc, argv);

	// Set up top-level logging.
	logging.SetMessagePattern("[          ] ["
					   "%{time hh:mm:ss.zzz} "
					   "%{threadid} "
					   "%{if-debug}DEBUG%{endif}%{if-info}INFO%{endif}%{if-warning}WARNING%{endif}%{if-critical}CRITICAL%{endif}"
					   "%{if-fatal}FATAL%{endif}"
					   "] "
						+ logging.ClickableLinkPattern() +
					   /*"%{function}:%{line}*/ " - %{message}"
					   "%{if-fatal}%{backtrace}%{endif}");

	// Start the log with the App ID and version info.
	qInfo() << "LOGGING START";
	qInfo() << "Application:" << app.applicationDisplayName() << "(" << app.applicationName() << ")";
	qInfo() << "    Version:" << app.applicationVersion() << "(" << VersionInfo::get_full_version_info_string() << ")";

	// Register types with Qt.
	RegisterQtMetatypes();

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


