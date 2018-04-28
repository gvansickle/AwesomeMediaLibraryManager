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

#include <QtGlobal>

//#include <QApplication>
#include "AMLMApp.h"

#include <QSettings>
#include <QIcon>
#include <QLoggingCategory>
#include <QResource>
#include <KAboutData>

#include <KIconLoader>
#include <KSharedConfig>
#include <KConfigGroup>
#include <QProcessEnvironment>

#include <QCommandLineParser>

#include <utils/AboutDataSetup.h>

#include <QImageReader>
#include <QDebug>
#include <gtk/gtk.h>
#include "utils/DebugHelpers.h"
#include "utils/StringHelpers.h"

#include "gui/MainWindow.h"
#include "utils/Theme.h"
#include "utils/RegisterQtMetatypes.h"

#include "resources/VersionInfo.h"

#include "utils/Logging.h"


int main(int argc, char *argv[])
{
	// Set up top-level logging.
	Logging logging;
	logging.SetFilterRules();
	logging.InstallMessageHandler();
	logging.SetMessagePattern("["
					   "%{time hh:mm:ss.zzz} "
					   "%{threadid} "
					   "%{if-debug}DEBUG%{endif}%{if-info}INFO%{endif}%{if-warning}WARNING%{endif}%{if-critical}CRITICAL%{endif}"
                       "%{if-fatal}FATAL%{endif}"
                       "] "
					   /*	+ logging.ClickableLinkPattern() + */
                       "%{function}:%{line} - %{message}"
					   "%{if-fatal}%{backtrace}%{endif}");

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
	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	/// @todo Look at:
	///		Qt::AA_UseStyleSheetPropagationInWidgetStyles
	///		Qt::AA_CompressHighFrequencyEvents (default is true on X11)

	//
	// Create the Qt5 app.
    // @note Must be the first QObject created and the last QObject deleted.
	//
    AMLMApp app(argc, argv);


	// Get our config for use later.
	KSharedConfigPtr config = KSharedConfig::openConfig();
	// Open or create two top-level config groups: "unmanaged" and "version".
	// We use the pre-existence of "version" to detect if this is the first time we've started.
	KConfigGroup grp(config, "unmanaged");
	KConfigGroup initialGroup(config, "version");
//	if (!initialGroup.exists())
//	{
//		/// @todo Not sure if we want to be this draconian.
//		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
//		if (env.contains(QStringLiteral("XDG_CURRENT_DESKTOP")) && env.value(QStringLiteral("XDG_CURRENT_DESKTOP")).toLower() == QLatin1String("kde"))
//		{
//			qDb() << "KDE Desktop detected, using system icons";
//		}
//		else
//		{
//			// We are not on a KDE desktop, force breeze icon theme
//			grp.writeEntry("force_breeze", true);
//			qDb() << "Non KDE Desktop detected, forcing Breeze icon theme";
//		}
//	}

	// Use HighDPI pixmaps as long as we're supporting High DPI scaling.
	app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

	// If we're forcing Breeze icons, force them here.
//	bool forceBreeze = grp.readEntry("force_breeze", QVariant(false)).toBool();
//	if (forceBreeze)
//	{
//		QIcon::setThemeName("breeze");
//	}

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
	app.setOrganizationName("gvansickle");

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
	qInfo() << "Organization:" << app.organizationName() << "(" << app.organizationDomain() << ")";
	qInfo() << " Application:" << app.applicationDisplayName() << "(" << app.applicationName() << ")";
	qInfo() << "     Version:" << app.applicationVersion() << "(" << VersionInfo::get_full_version_info_string() << ")";

	// Register types with Qt.
	RegisterQtMetatypes();

M_WARNING("icons not installed properly");
    // Load the icon resources.
	auto rccs = {"icons_oxygen.rcc"};
	for(auto fname : rccs)
	{
		bool opened = QResource::registerResource(fname);
		if(!opened)
		{
			qCritical() << "FAILED TO OPEN RCC:" << fname;
		}
		else
		{
			qIn() << "Loaded RCC file:" << fname;
		}
	}

	// Set the application Icon.
	// "KAboutData::setApplicationData() no longer sets the app window icon. For shells which do not fetch the icon name via
	// the desktop file, make sure to call QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("foo"))); (in GUI apps)."
	///@todo Get an actual icon.
	QIcon appIcon;
    appIcon.addFile(":/Icons/128-preferences-desktop-sound.png");
	app.setWindowIcon(appIcon);

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
		mainWin->show();
	}

    return app.exec();
}

