/*
 * DefaultSettingsHelper.h
 *
 *  Created on: Apr 8, 2018
 *      Author: gary
 */

#ifndef GUI_SETTINGS_DEFAULTSETTINGSHELPER_H_
#define GUI_SETTINGS_DEFAULTSETTINGSHELPER_H_

#include <QObject>
#include <QList>
#include <QUrl>
#include <QStringList>

/*
 *
 */
class DefaultSettingsHelper : public QObject
{
	Q_OBJECT

public:
	DefaultSettingsHelper();

	static QStringList defaultCollectionUrlList();

	static QStringList getSettingsFileList();
};

#endif /* GUI_SETTINGS_DEFAULTSETTINGSHELPER_H_ */
