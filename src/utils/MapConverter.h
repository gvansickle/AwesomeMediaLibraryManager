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

#ifndef MAPCONVERTER_H
#define MAPCONVERTER_H

#include <QVariantMap>
#include <map>
#include <string>

class MapConverter
{
public:
	MapConverter();

	static QVariantMap TagMaptoVarMap(const std::map<std::string, std::vector<std::string>>& tm);
	static std::map<std::string, std::vector<std::string>> VarMapToTagMap(const QVariantMap& vm);
};

#endif // MAPCONVERTER_H
