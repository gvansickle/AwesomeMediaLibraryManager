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
#include <QApplication>
#include <QSettings>
#include <QIcon>
#include <QStorageInfo>
#include <QLoggingCategory>
#include <QResource>

#include <QImageReader>
#include <QDebug>
#include "utils/DebugHelpers.h"
#include "utils/StringHelpers.h"

#include "gui/MainWindow.h"
#include "utils/Theme.h"
#include "utils/RegisterQtMetatypes.h"

#include "resources/VersionInfo.h"


static void printDebugMessagesWhileDebuggingHandler(QtMsgType type, const QMessageLogContext &context, const QString& msg)
{
	QString debug_str = qFormatLogMessage(type, context, msg);

#ifdef Q_OS_WIN
	OutputDebugString(debug_str.toStdWString().c_str());
#else
	std::cerr << debug_str.toStdString() << std::endl;
#endif
}

int main(int argc, char *argv[])
{
    // Allow us to see qDebug() messages, except for mouse movement.
    QLoggingCategory::setFilterRules("*.debug=true\n"
                                     "qt.qpa.input*.debug=false\n"
                                     "qt.*=false\n");

	if(true/** @todo We're running under a debugger.  This still doesn't work on Windows.*/)
    {
		qInstallMessageHandler(printDebugMessagesWhileDebuggingHandler);
    }

	// App-wide settings.
	// http://doc.qt.io/qt-5/qt.html#ApplicationAttribute-enum
	// Enable high-DPI scaling in Qt on supported platforms.
	// Makes Qt scale the main (device independent) coordinate system according to display scale factors provided by
	// the operating system. This corresponds to setting the QT_AUTO_SCREEN​_SCALE_FACTOR environment variable to 1.
	// Must be set before Q(Gui)Application is constructed.
	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	/// @todo Look at:
	///		Qt::AA_UseHighDpiPixmaps
	///		Qt::AA_UseStyleSheetPropagationInWidgetStyles
	///		Qt::AA_CompressHighFrequencyEvents (default is true on X11)

	// Create the Qt5 app.
    QApplication app(argc, argv);

    // Set up top-level logging.
	qSetMessagePattern("["
					   "%{time yyyy-MM-ddThh:mm:ss.zzz} "
					   "%{if-debug}DEBUG%{endif}%{if-info}INFO%{endif}%{if-warning}WARNING%{endif}%{if-critical}CRITICAL%{endif}"
					   "%{if-fatal}FATAL%{endif}"
					   "] "
					   "%{function}:%{line} - %{message}"
					   "%{if-fatal}%{backtrace}%{endif}");

	// Set up app information.
	app.setApplicationName("AwesomeMediaLibraryManager");
    app.setApplicationVersion(toqstr(VersionInfo::get_version_quad()));
    app.setOrganizationName("gvansickle");
	app.setOrganizationDomain("gvansickle.github.io");
    app.setApplicationDisplayName("Awesome Media Library Manager");
	app.setDesktopFileName("AwesomeMediaLibraryManager.desktop");

	// Start the log with the App ID and version info.
	qInfo() << "LOGGING START";
	qInfo() << "Application:" << app.applicationDisplayName() << "(" << app.applicationName() << ")";
	qInfo() << "    Version:" << app.applicationVersion() << "(" << VersionInfo::get_full_version_info_string() << ")";

	// Register types with Qt.
	RegisterQtMetatypes();

    // Load the icon resources.
	bool opened = false;
	auto rccs = {"icons_oxygen.rcc", "icons_Tango.rcc", "icons_App.rcc"};
	for(auto fname : rccs)
	{
		bool opened = QResource::registerResource(fname);
		if(!opened)
		{
			qCritical() << "FAILED TO OPEN RCC:" << fname;
		}
	}

	// Set the application Icon.
	///@todo Get an actual icon.
	QIcon appIcon;
	appIcon.addFile(":/icons/oxygen-icons/64x64/apps/preferences-desktop-sound.png");
	app.setWindowIcon(appIcon);


	// Always use INI format for app settings, so we don't hit registry restrictions on Windows.
	QSettings::setDefaultFormat(QSettings::IniFormat);

    // Log the audio file types we support.
    /// @todo M_WARNING("TODO: Log supported file types")

	qDebug() << "TEST: Debug";
	qWarning() << "TEST: Warning";
	qCritical() << "TEST: Critical";

    // Create and show the main window.
    MainWindow mainWin;
    mainWin.show();
    return app.exec();
}

