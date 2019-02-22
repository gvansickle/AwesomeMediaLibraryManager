/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/**
 * @file AMLMTagMap.cpp
 */
#include "AMLMTagMap.h"

AMLM_QREG_CALLBACK([](){
	qIn() << "Registering AMLMTagMap metatypes";
	qRegisterMetaType<AMLMTagMap>();
	qRegisterMetaTypeStreamOperators<AMLMTagMap>();
//	QMetaType::registerDebugStreamOperator<Metadata>();
//	QMetaType::registerConverter<Metadata, QString>([](const Metadata& obj){ return obj.name(); });
});


AMLMTagMap& AMLMTagMap::operator=(const TagMap& tagmap)
{
	m_the_map.clear();
	for(const auto & it : tagmap)
	{
		for(const auto& valit : it.second)
		{
			m_the_map.insert(std::make_pair(it.first, valit));
		}
	}
	return *this;
}

std::vector<AMLMTagMap::mapped_type> AMLMTagMap::operator[](const AMLMTagMap::Key& key)
{
	auto range = m_the_map.equal_range(key);
	if(range.first != m_the_map.end())
	{
		auto retval = std::vector<mapped_type>();
		for(auto& i = range.first; i != range.second; ++i)
		{
			retval.push_back(i->second);
		}
		return retval;
	}
	else
	{
		auto it = m_the_map.insert( std::make_pair(key, T()) );
		return std::vector<mapped_type>();
	}
}

std::vector<AMLMTagMap::mapped_type> AMLMTagMap::equal_range_vector(const AMLMTagMap::Key& key) const
{
	auto range = equal_range(key);

	auto retval = std::vector<mapped_type>();
	for(auto& i = range.first; i != range.second; ++i)
	{
		retval.push_back(i->second);
	}
	return retval;
}

QVariant AMLMTagMap::toVariant() const
{
	QVariantMap map;

	std::pair<const_iterator, const_iterator> range;

	// Iterate through the multimap's elements by key.
	for(auto it = m_the_map.begin(); it != m_the_map.end(); it = range.second)
	{
		// Get the range of the current key
		range = m_the_map.equal_range(it->first);

		// Iterate over the values in the range.
M_TODO("THIS IS WRONG, MANY-TO-ONE MAPPING");
		for(auto valit = range.first; valit != range.second; ++valit)
		{
			map.insert(toqstr(valit->first), toqstr(valit->second));
		}
	}

	return map;
}

void AMLMTagMap::fromVariant(const QVariant& variant)
{
	Q_ASSERT(0);

}

