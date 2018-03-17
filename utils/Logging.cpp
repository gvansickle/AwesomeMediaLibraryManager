/*
 * Logging.cpp
 *
 *  Created on: Mar 14, 2018
 *      Author: gary
 */

#include "Logging.h"

#include <QtGlobal>
#include <QString>
#include <QLoggingCategory>

#include <iostream>

Logging::Logging()
{
	// TODO Auto-generated constructor stub

}

void printDebugMessagesWhileDebuggingHandler(QtMsgType type, const QMessageLogContext &context, const QString& msg)
{
	QString debug_str = qFormatLogMessage(type, context, msg);

#ifdef Q_OS_WIN
	OutputDebugString(debug_str.toStdWString().c_str());
#else
	std::cerr << debug_str.toStdString() << std::endl;
#endif
}

void Logging::SetFilterRules()
{
	// Allow us to see qDebug() messages, except for mouse movement.
	QLoggingCategory::setFilterRules("*.debug=true\n"
									 "qt.qpa.input*.debug=false\n"
									 "qt.*=false\n");
}

void Logging::InstallMessageHandler()
{
	if(true/** @todo We're running under a debugger.  This still doesn't work on Windows.*/)
    {
		qInstallMessageHandler(printDebugMessagesWhileDebuggingHandler);
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
	/// @todo Doesn't seem to work.
	return "   Loc: [%{file}:%{line}]";
}
