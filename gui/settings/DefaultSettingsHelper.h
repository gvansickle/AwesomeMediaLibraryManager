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

/*
 *
 */
class DefaultSettingsHelper : public QObject
{
	Q_OBJECT

public:
	DefaultSettingsHelper();

	static QList<QUrl> defaultCollectionUrlList();
};

#endif /* GUI_SETTINGS_DEFAULTSETTINGSHELPER_H_ */
