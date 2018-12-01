/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include <config.h>

#include "AMLMApp.h"

/// Qt5
#include <QtGlobal>
#include <QSettings>
#include <QIcon>
#include <QLoggingCategory>
#include <QResource>
#include <QProcessEnvironment>
#include <QCommandLineParser>
#include <QImageReader>
#include <QDebug>

/// KF5
#include <KAboutData>
#include <KIconTheme>
#include <KSharedConfig>
#include <KConfigGroup>

#if HAVE_GTKMM01
/// GTK
#include <gtk/gtk.h>
#endif

/// Ours
#include <gui/Theme.h>
#include <utils/AboutDataSetup.h>
#include "utils/DebugHelpers.h"
#include "utils/StringHelpers.h"

#include "gui/MainWindow.h"

#include "resources/VersionInfo.h"

#include "utils/Logging.h"

M_WARNING("BUILDING WITH CMAKE_C_COMPILER_ID: " CMAKE_C_COMPILER_ID " = " CMAKE_C_COMPILER)
M_WARNING("BUILDING WITH CMAKE_CXX_COMPILER_ID: " CMAKE_CXX_COMPILER_ID " = " CMAKE_CXX_COMPILER)

/**
 * Here's where the magic happens.
 */
int main(int argc, char *argv[])
{
	// Make sure our compiled-in static lib resources are linked.
	Q_INIT_RESOURCE(xquery_files);

	QThread::currentThread()->setObjectName("MAIN");

	// Set up top-level logging.
	Logging logging;
	logging.SetFilterRules();
	logging.InstallMessageHandler();
	logging.SetMessagePattern("["
					   "%{time hh:mm:ss.zzz} "
                       "%threadname15 "
					   "%{if-debug}DEBUG%{endif}%{if-info}INFO%{endif}%{if-warning}WARNING%{endif}%{if-critical}CRITICAL%{endif}"
                       "%{if-fatal}FATAL%{endif}"
					   "%{if-category} %{category} %{endif}"
						"] "
					   /*	+ logging.ClickableLinkPattern() + */
                       "%{function}:" /*"%shortfunction:"*/ "%{line} - %{message}"
                       /* "%{if-fatal}%{backtrace}%{endif}" */);

	// Logging test.
	qInfo() << "LOGGING START";
	qDebug() << "TEST: Debug";
	qWarning() << "TEST: Warning";
	qCritical() << "TEST: Critical";

	// Log our startup environment.
	logging.dumpEnvVars();

	// App-wide settings.
	// http://doc.qt.io/qt-5/qt.html#ApplicationAttribute-enum
	// Enable high-DPI scaling in Qt on supported platforms.
	// Makes Qt scale the main (device independent) coordinate system according to display scale factors provided by
	// the operating system. This corresponds to setting the QT_AUTO_SCREEN​_SCALE_FACTOR environment variable to 1.
	/// @note Must be set before Q(Gui)Application is constructed.
    AMLMApp::setAttribute(Qt::AA_EnableHighDpiScaling);
    // Use HighDPI pixmaps as long as we're supporting High DPI scaling.
    /// @note Must be set before Q(Gui)Application is constructed.
    AMLMApp::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
	/// @todo Look at:
	///		Qt::AA_UseStyleSheetPropagationInWidgetStyles
	///		Qt::AA_CompressHighFrequencyEvents (default is true on X11)

    // QStandardPaths::AppDataLocation changes before and after QApp creation, and then again when we set these
    // vars, if we don't set them prior to constructing the app.  This affects KIconTheme's attempt to load
    // an "icontheme.rcc" file from one of these dirs.
    AMLMApp::setOrganizationName("gvansickle");
    AMLMApp::setApplicationName("AwesomeMediaLibraryManager");
    // And KConfig wants this instead.
    AMLMApp::setOrganizationDomain("gvansickle.github.io");


	//
    // Create the Qt5/KF5 app.
    // @note Must be the first QObject created and the last QObject deleted.
    // @note This should have loaded any bundled icontheme.rcc files.
	//
    qIn() << "START Constructing AMLMApp";
    AMLMApp app(argc, argv);
    qIn() << "END Constructing AMLMApp";

    // Log the startup icon theme info.
    Theme::LogIconThemeInfo();

	app.Init();

	// Get our config for use later.
	KSharedConfigPtr config = KSharedConfig::openConfig();
	// Open or create two top-level config groups: "unmanaged" and "version".
	// We use the pre-existence of "version" to detect if this is the first time we've started.
	KConfigGroup grp(config, "unmanaged");
	KConfigGroup initialGroup(config, "version");
    if (!initialGroup.exists())
	{
        // First-time startup.

		/// @todo Not sure if we want to be this draconian.
//        app.KDEOrForceBreeze(grp);
	}

    // If we're forcing Breeze icons, force them here.
    M_WARNING("Not picking up these icons FWICT.  Also interfering with user selected icon theme, and doesn't get saved.");
    bool forceBreeze = grp.readEntry("force_breeze", QVariant(false)).toBool();
    if (forceBreeze)
    {
//        Theme::setIconThemeName("breeze");
    }


	// Set up the KAboutData.
	// From: https://community.kde.org/Frameworks/Porting_Notes#Build_System
	// "Make sure to create KAboutData instance only once the Q*Application instance has been created,
	// if you are using KI18n's i18n() [...]"
	KAboutData aboutData = AboutDataSetup::GetKAboutData();

	// Set the application metadata.
	// "In addition to changing the result of applicationData(), this initializes the equivalent properties of QCoreApplication
	// (and its subclasses) with information from aboutData, if an instance of that already exists. Those properties are:
	//
	//	QCoreApplication::applicationName
	//  QCoreApplication::applicationVersion
	//  QCoreApplication::organizationDomain
	//  QGuiApplication::applicationDisplayName
	//  QGuiApplication::desktopFileName (since 5.16)"
	//
	// https://community.kde.org/Frameworks/Porting_Notes#Build_System:
	// "Make sure to call KAboutData::setApplicationData() only once the Q*Application instance has been created,
	// otherwise the respective Q*Application metadata will not be set (e.g. QGuiApplication::applicationDisplayName),
	// which other KF5 code now relies on."
	// "If you want the config files placed in the correct location instead of being mixed up with other stuff,
	// make sure you have QCoreApplication::setOrganizationDomainName()[1] call that sets the correct name for your application."
	// [1] No such function, not really sure if they mean setOrganizationName(), setOrganizationDomain(), or setApplicationName().
	//     ... looks like setOrganizationName() is empty of we don't explicitly set it below.
	KAboutData::setApplicationData(aboutData);

	// Integrate KAboutData with commandline argument handling
	QCommandLineParser parser;
	aboutData.setupCommandLine(&parser);
	// setup of app specific commandline args
	parser.setApplicationDescription(aboutData.shortDescription());
	parser.addVersionOption();
	parser.addHelpOption();
	// ... addOption() additional options here.
	parser.process(app);
	aboutData.processCommandLine(&parser);

	// Application metadata set, now register to the D-Bus session
	/// @todo No DBus functionality currently.

	// Start the log with the App ID and version info.
	qInfo() << "Organization:" << AMLMApp::organizationName() << "(" << AMLMApp::organizationDomain() << ")";
	qInfo() << " Application:" << AMLMApp::applicationDisplayName() << "(" << AMLMApp::applicationName() << ")";
	qInfo() << "     Version:" << AMLMApp::applicationVersion() << "(" << VersionInfo::get_full_version_info_string() << ")";


	// Set the application Icon.
	///@todo Get an actual icon.
    QIcon appIcon; //= QIcon::fromTheme(QStringLiteral("preferences-desktop-sound"), QApplication::windowIcon());
    appIcon.addFile(":/Icons/128-preferences-desktop-sound.png");
    // "KAboutData::setApplicationData() no longer sets the app window icon. For shells which do not fetch the icon name via
    // the desktop file [i.e. non-plasma], make sure to call QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("foo")));
    // (in GUI apps)."
    // We also have to make sure we don't replace an already-existing app icon with an empty one,
    // hence the default of the current windowIcon().
    QApplication::setWindowIcon(appIcon);

	// Always use INI format for app settings, so we don't hit registry restrictions on Windows.
	QSettings::setDefaultFormat(QSettings::IniFormat);

	qInfo() << "QPA Platform plugin name:" << app.platformName();

	// Session management support.
	if(app.isSessionRestored())
	{
		// Don't need to deal with KMainWindow::restore(), this takes care of it.
		kRestoreMainWindows<MainWindow>();
	}
	else
	{
		// Normal startup.

		// Create and show the main window.
		// From the KDE5 docs: https://api.kde.org/frameworks/kxmlgui/html/classKMainWindow.html#ab0c194be12f0ad123a9ba8be75bb85c9
		// "KMainWindows must be created on the heap with 'new'"
		MainWindow *mainWin = new MainWindow();
		// Tell the app singleton about the main window singleton.
		// Note that there's a lot of code between the two creations.
		amlmApp->MAIN_ONLY_setMainWindow(mainWin);
		mainWin->show();
	}

    return app.exec();
}

