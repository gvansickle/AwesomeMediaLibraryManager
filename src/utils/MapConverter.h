/*
 * Copyright 2017, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/// @file

// C++
#include <map>
#include <string>
#include <expected>

// Qt
#include <deque>
#include <QVariantMap>


class MapConverter
{
public:
	MapConverter();

	/**
	 * @note This function is misnamed/returns the wrong type.  A QVariantMap is a typedef
	 *       of QMap<QString, QVariant>; this function actually returns a QMap<QString, QVariant<QStringList>>.
	 */
	static QVariantMap TagMaptoVarMap(const std::map<std::string, std::vector<std::string>>& tm);
	static std::map<std::string, std::vector<std::string>> VarMapToTagMap(const QVariantMap& vm);
};


template <class MultiMapType1, class MultiMapType2>
struct mapdiff_result
{
	std::deque<typename MultiMapType1::value_type> m_in1not2;
	std::deque<typename MultiMapType2::value_type> m_in2not1;

	ssize_t size() const { return m_in1not2.size() + m_in2not1.size(); }
};

template <class MultiMapType1, class MultiMapType2>
QDebug operator<<(QDebug dbg, const mapdiff_result<MultiMapType1, MultiMapType2>& diff)
{
	QDebugStateSaver saver(dbg);
	dbg << "in1not2: ";
	for (const auto& e : diff.m_in1not2)
	{
		dbg << e;
	}
	dbg << "in2not1: ";
	for (const auto& e : diff.m_in2not1)
	{
		dbg << e;
	}
	return dbg;
}

template <class MultiMapType1>
bool map_contains_value(const MultiMapType1& map, const typename MultiMapType1::value_type& value)
{
	auto it = std::find_if(map.cbegin(), map.cend(),
		[&value](const auto& val)
		{
			return val == value;
		});
	return it != map.cend();
}

template <class MultiMapType1, class MultiMapType2>
std::expected<mapdiff_result<MultiMapType1,MultiMapType2>, std::string> mapdiff(const MultiMapType1& map1, const MultiMapType2& map2)
{
	mapdiff_result<MultiMapType1,MultiMapType2> retval;

	for(const auto& e1 : map1)
	{
		if (map_contains_value(map2, typename MultiMapType2::value_type(e1)) == false)
		{
			retval.m_in1not2.push_back(e1);
		}
	}
	for(const auto& e2 : map2)
	{
		if (map_contains_value(map1, typename MultiMapType1::value_type(e2)) == false)
		{
			retval.m_in2not1.push_back(e2);
		}
	}

	return retval;
}

#endif // MAPCONVERTER_H
