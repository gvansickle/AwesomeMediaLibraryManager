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
#include <utils/RegisterQtMetatypes.h>

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

	// Iterate over key+value pairs.
	for(const auto & it : tagmap)
	{
		// Iterate over the value, which is a vector of values.
		for(const auto& valit : it.second)
		{
			m_the_map.insert(std::make_pair(it.first, valit));
		}
	}
	return *this;
}

//AMLMTagMap::operator TagMap() const
//{
//	return TagMap();
//}


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

std::vector<AMLMTagMap::key_type> AMLMTagMap::keys() const
{
	std::vector<AMLMTagMap::key_type> retval;

	AMLMTagMap::key_type last_key;

	for(const auto& it : m_the_map)
	{
		// Pick out the unique keys.
		if(it.first != last_key)
		{
			last_key = it.first;
			retval.push_back(it.first);
		}
	}

	return retval;
}


QVariant AMLMTagMap::toVariant() const
{
	QVariantInsertionOrderedMap map;

	QVariantHomogenousList key_list("keys", "key");

	// Get all the keys.
	auto all_keys = keys();

	QVariantList qvar_list;

	// Iterate over all keys.
	for(const auto& it : all_keys)
	{
		// Get all values for this key.
//		QVariantHomogenousList qval_list("values", "value");
		QVariantList qval_list;

		const auto ervec = equal_range_vector(it);
		for(const auto& val : ervec)
		{
			qval_list.push_back(toqstr(val));
		}
//		iomap.insert(toqstr(it), qvar_list);
		QVariantList key_values_pair;
		key_values_pair.push_back(toqstr(it));
		key_values_pair.push_back(qval_list);
		qvar_list.push_back(key_values_pair);
	}

	map.insert("m_the_map", QVariant::fromValue(qvar_list));

	return map;
}

void AMLMTagMap::fromVariant(const QVariant& variant)
{
	Q_ASSERT(variant.canConvert<QVariantInsertionOrderedMap>());
	QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();

	QVariantInsertionOrderedMap iomap;
//	QVariant qvar_iomap = map.value("m_the_map");
//	Q_ASSERT(qvar_iomap.canConvert<QVariantInsertionOrderedMap>());
//	iomap = qvar_iomap.value<QVariantInsertionOrderedMap>();

//	// Get all
//	for(const auto& entry_qvar : qAsConst(list))
//	{
//		auto entry_list = entry_qvar.value<QStringList>();
//		auto entry = std::make_pair(tostdstr(entry_list[0]), tostdstr(entry_list[1]));
//		m_the_map.insert(entry);
//	}

}

AMLMTagMap::operator QVariant() const
{
	return toVariant();
}

QTH_DEFINE_QDEBUG_OP(AMLMTagMap, << obj.m_the_map );

#define DATASTREAM_FIELDS(X) \
	X(m_the_map, m_the_map)

QDataStream& operator<<(QDataStream& out, const AMLMTagMap& obj)
{
	Q_ASSERT(0);
//#define X(unused, field) << obj.field
//	out DATASTREAM_FIELDS(X);
//#undef X
//	return out;
}

QDataStream& operator>>(QDataStream& in, AMLMTagMap& obj)
{
	Q_ASSERT(0);
//#define X(unused, field) >> obj.field
//	return in DATASTREAM_FIELDS(X);
//#undef X
}


