/*
 * Logging.h
 *
 *  Created on: Mar 14, 2018
 *      Author: gary
 */

#ifndef UTILS_LOGGING_H_
#define UTILS_LOGGING_H_

#include <QtGlobal>

class QString;
class QMessageLogContext;

/*
 *
 */
class Logging
{
public:
	Logging();

	void SetFilterRules();

	void InstallMessageHandler();

	void SetMessagePattern(const QString & pattern);

	QString ClickableLinkPattern();

	void dumpEnvVars();
};

/**
 * Replacement message handler we'll install.
 */
void printDebugMessagesWhileDebuggingHandler(QtMsgType type, const QMessageLogContext &context, const QString& msg);


#endif /* UTILS_LOGGING_H_ */
