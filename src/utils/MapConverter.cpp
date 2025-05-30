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

/// @file

#include "MapConverter.h"

// Qt
#include <QVariant>
#include <QString>

MapConverter::MapConverter()
{

}

QVariantMap MapConverter::TagMaptoVarMap(const std::map<std::string, std::vector<std::string> >& tm)
{
	QVariantMap retval;
	for(const auto& e : tm)
	{
		QStringList sl;
		for(const auto& str : e.second)
		{
			sl.append(QString::fromUtf8(str.c_str()));
		}
		retval.insert(QString::fromUtf8(e.first.c_str()), sl);
	}

	return retval;
}

std::map<std::string, std::vector<std::string> > MapConverter::VarMapToTagMap(const QVariantMap& vm)
{
	std::map<std::string, std::vector<std::string>> retval;

	QVariantMap::const_iterator cit;
	for(cit = vm.cbegin(); cit != vm.cend(); ++cit)
	{
		std::vector<std::string> sl;

		QStringList qsl = cit->value<QStringList>();

		for(const auto& cstr : std::as_const(qsl))
		{
			sl.push_back(cstr.toStdString());
		}
		retval[cit.key().toStdString()] = sl;
	}

	return retval;
}
