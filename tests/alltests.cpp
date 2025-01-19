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
#include "AMLMApp.h"
#include "utils/DebugHelpers.h"
#include "utils/StringHelpers.h"
#include "utils/RegisterQtMetatypes.h"
#include "resources/VersionInfo.h"
#include "utils/Logging.h"
#include "TestHelpers.h"

// Google Test
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Make sure we've compiled correctly.
#if !defined(GTEST_IS_THREADSAFE) || (GTEST_IS_THREADSAFE != 1)
#error "Google Test wasn't compiled for a multithreaded environment."
#endif
static_assert(BOOST_THREAD_VERSION >= 5);

/**
 * Override of Environment for global setup and teardown.
 */
class StartAndFinish : public ::testing::Environment
{
public:
    ~StartAndFinish() override {}

	// Print the start message.
	void SetUp() override { 	GTEST_COUT << "TEST STARTING" << std::endl; }

	// Print the finished message.
	void TearDown() override { 	GTEST_COUT << "TEST COMPLETE" << std::endl; }
};

// Turn ASSERT failures into exceptions, to allow ASSERTs from subroutines to stop the calling test.
/// @link https://github.com/google/googletest/blob/master/googletest/docs/advanced.md#asserting-on-subroutines-with-an-exception
class ThrowListener : public testing::EmptyTestEventListener
{
  void OnTestPartResult(const testing::TestPartResult& result) override
  {
    if (result.type() == testing::TestPartResult::kFatalFailure)
    {
      throw testing::AssertionException(result);
    }
  }
};

int google_test_main(int argc, char *argv[])
{
//	::testing::InitGoogleTest(&argc, argv);
	::testing::InitGoogleMock(&argc, argv);

	// Create a new environment object and register it with gtest.
	// Don't delete it, gtest takes ownership.
	::testing::AddGlobalTestEnvironment(new StartAndFinish());

	// Add the exception listener as the last listener.
	testing::UnitTest::GetInstance()->listeners().Append(new ThrowListener);

	// No death tests yet, but for when we do:
	testing::FLAGS_gtest_death_test_style="threadsafe";

	auto retval = RUN_ALL_TESTS();
	return retval;
}

///
/// main() for Google Test Framework tests.
///
/// @note main() mods to support Qt5 threading etc. testing per Stack Overflow: https://stackoverflow.com/a/33829950
///
int main(int argc, char *argv[])
{
	QThread::currentThread()->setObjectName("MAIN");

	Logging logging;

	logging.SetFilterRules();
	logging.InstallMessageHandler();
    // Set up top-level logging.
    logging.SetMessagePattern("[          ] "
                              "["
                              "%{time hh:mm:ss.zzz} "
                              "%threadname15 "
                              "%{if-debug}DEBUG%{endif}%{if-info}INFO%{endif}%{if-warning}WARNING%{endif}%{if-critical}CRITICAL%{endif}"
                              "%{if-fatal}FATAL%{endif}"
    							"%{if-category} %{category}%{endif}"
                              "] "
                              /*	+ logging.ClickableLinkPattern() + */
                              "%shortfunction:%{line} - %{message}"
                              "%{if-fatal}%{backtrace}%{endif}");

	//
    // Create the Qt5/KF5 app.
    // @note Must be the first QObject created and the last QObject deleted.
	//
    AMLMApp app(argc, argv);
	app.Init(true);


	// Start the log with the App ID and version info.
	qInfo() << "LOGGING START";
	qInfo() << "Application:" << app.applicationDisplayName() << "(" << app.applicationName() << ")";
	qInfo() << "    Version:" << app.applicationVersion() << "(" << VersionInfo::get_full_version_info_string() << ")";

	// Register types with Qt.
	RegisterQtMetatypes();

	// Timer-based exit from app.exec().
//	QTimer exitTimer;
//	QObject::connect(&exitTimer, &QTimer::timeout, &app, QCoreApplication::quit);
//	exitTimer.start();

	// Start the Google Test from a timer expiration, so we know that we have a
	// legitimate event loop running.
	int retval = 0;
	QTimer::singleShot(0, &app, [&retval, &argc, &argv, &app](){
		retval = google_test_main(argc, argv);
		qDb() << "google_test_main() returned:" << retval;

		// Now send a signal to the app to exit.
		QTimer::singleShot(0, &app, [&app, &retval](){
			qDb() << "EXIT TIMER FIRED, CALLING app.exit()";
			AMLMApp::exit(retval);
		});
		qDb() << "exit() timer created, should exit soon";
	});

	AMLMApp::exec();

	return retval;
}


