/*
 * DefaultSettingsHelper.cpp
 *
 *  Created on: Apr 8, 2018
 *      Author: gary
 */

#include "DefaultSettingsHelper.h"

#include <QStandardPaths>

DefaultSettingsHelper::DefaultSettingsHelper()
{
	// TODO Auto-generated constructor stub

}

QList<QUrl> DefaultSettingsHelper::defaultCollectionUrlList()
{
	QList<QUrl> default_collection_paths;
	auto path_list = QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
	for(auto path : path_list)
	{
		default_collection_paths.append(QUrl::fromUserInput(path));
	}
	return default_collection_paths;
}

