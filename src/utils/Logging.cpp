/*
 * Logging.cpp
 *
 *  Created on: Mar 14, 2018
 *      Author: gary
 */

#include <config.h>

// Header for this file.
#include "Logging.h"

// Std C++
#include <iostream>

// Qt5
#include <QGuiApplication>
#include <QLibraryInfo>
#include <QLoggingCategory>
#include <QProcessEnvironment>
#include <QString>
#include <QThread>

// Ours
#include "DebugHelpers.h"


Logging::Logging()
{
}

void printDebugMessagesWhileDebuggingHandler(QtMsgType type, const QMessageLogContext &context, const QString& msg)
{

    QString debug_str;

    //
    auto cur_thread = QThread::currentThread();
    QString thread_name;
    if(cur_thread)
    {
        thread_name = cur_thread->objectName();
    }

    if(thread_name.isEmpty())
    {
        // No name yet, last-ditch we'll print the native thread ID.

        auto cur_thread_id = QThread::currentThreadId();
        thread_name = QString("%1").arg((unsigned long)cur_thread_id);
    }
    // Fit to 15 chars, fixed width.
    thread_name = thread_name.leftJustified(15, '_', true);

    debug_str = qFormatLogMessage(type, context, msg);

    debug_str.replace(QStringLiteral("%threadname15"), thread_name);

    /// @todo I must be missing a header on Windows, all I get is "OutputDebugString not defined" here.
#if 0 //def Q_OS_WIN
	OutputDebugString(debug_str.toStdWString().c_str());
#else
	std::cerr << debug_str.toStdString() << std::endl;
#endif
}

void Logging::SetFilterRules()
{
	// Allow us to see qDebug() messages, except for mouse movement.
	QLoggingCategory::setFilterRules(
			"*.debug=true\n"
			"qt.qpa.input*.debug=false\n"
			"qt.*=false\n"
			"qt.core.qabstractitemmodel.checkindex=true\n"
			);
}

void Logging::InstallMessageHandler()
{
	if(true/** @todo We're running under a debugger.  This still doesn't work on Windows.*/)
    {
#ifndef Q_OS_WIN
        qInstallMessageHandler(printDebugMessagesWhileDebuggingHandler);
#endif
    }
}

void Logging::SetMessagePattern(const QString& pattern)
{
	qSetMessagePattern(pattern);
}

QString Logging::ClickableLinkPattern()
{
	// In QtCreator, this magic pattern will result in a link you can click to go to the source of the message.
	// Must start with three spaces.
	/// @todo This doesn't seem to work with current QtCreator and relative paths anyway.
	return "   Loc: [%{file}:%{line}]";
}

void Logging::dumpEnvVars()
{
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

	QStringList known_env_vars;
	// See: https://www.freedesktop.org/software/systemd/man/pam_systemd.html
	//      https://specifications.freedesktop.org/desktop-entry-spec/latest/
	//      https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
	known_env_vars << "XDG_SESSION_ID" // A session identifier, suitable to be used in filenames. The string itself should be considered opaque, although often it is just the audit session ID as reported by /proc/self/sessionid. Each ID will be assigned only once during machine uptime. It may hence be used to uniquely label files or other resources of this session.
				   << "XDG_RUNTIME_DIR" // Path to a user-private user-writable directory that is bound to the user login time on the machine.
				   << "XDG_SESSION_TYPE" // The session type.
				   << "XDG_SESSION_CLASS" // The session class.
				   << "XDG_SESSION_DESKTOP" // A single, short identifier string for the desktop environment.
											// For example: "GNOME", or "KDE".
					 << "XDG_CURRENT_DESKTOP"
					  ;

	QStringList known_xdg_base_dir_spec_vars;
	known_xdg_base_dir_spec_vars << "XDG_DATA_HOME" // base directory relative to which user specific data files should be stored
													// If either not set or empty, a default equal to $HOME/.local/share should be used.
								<< "XDG_CONFIG_HOME" // base directory relative to which user specific configuration files should be stored.
													// If either not set or empty, a default equal to $HOME/.config should be used.
								<< "XDG_DATA_DIRS" // preference-ordered colon-separated set of base directories to search for data files in addition to the $XDG_DATA_HOME base directory.
													// If not set or empty, a value equal to /usr/local/share/:/usr/share/ should be used.
								<< "XDG_CONFIG_DIRS" // preference-ordered set of base directories to search for configuration files in addition to the $XDG_CONFIG_HOME base directory.
													// If either not set or empty, a value equal to /etc/xdg should be used.
								<< "XDG_CACHE_HOME" // base directory relative to which user specific non-essential data files should be stored.
													// If either not set or empty, a default equal to $HOME/.cache should be used.
								<< "XDG_RUNTIME_DIR" // the base directory relative to which user-specific non-essential runtime files and other file objects (such as sockets, named pipes, ...) should be stored.
													// MUST be owned by the user, and he MUST be the only one having read and write access to it. Its Unix access mode MUST be 0700.
								   ;

	QStringList known_qt_kde_vars;
	known_qt_kde_vars << "QT_SELECT"
					  << "QT_QPA_PLATFORM_PLUGIN_PATH"
					  << "QT_QPA_PLATFORM" // Platform Plugin, one of: eglfs, linuxfb, minimal, minimalegl, offscreen, vnc, wayland-egl, wayland, wayland-xcomposite-egl, wayland-xcomposite-glx, xcb.
					  << "QT_QPA_PLATFORMTHEME"
					  << "QT_STYLE_OVERRIDE"
					  << "QT_IM_MODULE"
					  << "KDEDIRS"
						 ;


	qInfo() << "QT/KDE Environment variables:";
	for(auto str : known_qt_kde_vars)
	{
		qInfo() << str + ":" << env.value(str, "<unset or empty>");
	}

	qInfo() << "QLibraryInfo::platformPluginArguments(\"xcb\")" << QLibraryInfo::platformPluginArguments("xcb");

	// Check our library path.
	qInfo() << "QGuiApplication::libraryPaths():" << QGuiApplication::libraryPaths();

	qInfo() << "XDG Environment variables:";
	for(auto str : known_env_vars)
	{
		qInfo() << str + ":" << env.value(str, "<unset or empty>");
	}

	qInfo() << "XDG Base Directory Specification environment variables:";

	for(auto str : known_xdg_base_dir_spec_vars)
	{
		qInfo() << str + ":" << env.value(str, "<unset or empty>");
	}
}
