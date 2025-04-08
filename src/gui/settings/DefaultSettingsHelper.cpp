/*
 * DefaultSettingsHelper.cpp
 *
 *  Created on: Apr 8, 2018
 *      Author: gary
 */

#include "DefaultSettingsHelper.h"

#include <QStandardPaths>

DefaultSettingsHelper::DefaultSettingsHelper(QObject* parent) : QObject(parent)
{
	// TODO Auto-generated constructor stub

}

QStringList DefaultSettingsHelper::defaultCollectionUrlList()
{
	QStringList default_collection_paths;
	auto path_list = QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
	for(const auto& path : std::as_const(path_list))
	{
		default_collection_paths.append(QUrl::fromUserInput(path).toDisplayString());
	}
	return default_collection_paths;
}

QStringList DefaultSettingsHelper::getSettingsFileList()
{
	QStringList retval;
	retval << "NULL";
	return retval;
}

