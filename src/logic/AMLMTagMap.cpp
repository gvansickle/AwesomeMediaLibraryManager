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

// TagLib.
#include <taglib/xiphcomment.h>

// Ours.
#include <utils/RegisterQtMetatypes.h>

AMLM_QREG_CALLBACK([](){
	qIn() << "Registering AMLMTagMap metatypes";
	qRegisterMetaType<AMLMTagMap>();
	qRegisterMetaTypeStreamOperators<AMLMTagMap>();
});


AMLMTagMap& AMLMTagMap::operator=(const TagMap& tagmap)
{
	clear();

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

AMLMTagMap& AMLMTagMap::operator=(const TagLib::Ogg::FieldListMap& taglib_field_list_map)
{
	clear();

	// Iterate over key+value_vector pairs.
	for(const auto & it : taglib_field_list_map)
	{
		// Iterate over the value, which is a vector of values.
		for(const auto& valit : it.second)
		{
			m_the_map.insert(std::make_pair(tostdstr(it.first), tostdstr(valit)));
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

AMLMTagMap::iterator AMLMTagMap::find(const AMLMTagMap::Key& x)
{
	return m_the_map.find(x);
}

AMLMTagMap::const_iterator AMLMTagMap::find(const AMLMTagMap::Key& x) const
{
	return m_the_map.find(x);
}

bool AMLMTagMap::contains(const AMLMTagMap::Key& key) const
{
	return (m_the_map.find(key) != cend());
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

void AMLMTagMap::clear()
{
	m_the_map.clear();
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

	QVariantHomogenousList list("AMLMTagMapEntries", "entry");

	// Get the list of keys, in... insertion order?
	auto keylist = keys();

	for(const auto& key : keylist)
	{
		// For each key, create a vector of values, in insertion order.
		const auto& vector_of_values = equal_range_vector(key);

		QVariantHomogenousList qvector_of_values("values", "value");
		for(const auto& value : vector_of_values)
		{
			qvector_of_values.push_back(toqstr(value));
		}

		QVariantInsertionOrderedMap kvpair_map;

		// Insert it into the return value.
		map_insert_or_die(kvpair_map, "key", toqstr(key));
		map_insert_or_die(kvpair_map, "values", qvector_of_values);

		list.push_back(QVariant::fromValue(kvpair_map));
	}

	map.insert("m_the_map", QVariant::fromValue(list));
//	map_insert_or_die(map, "m_the_map", list);

	return /*map*/list;
}

void AMLMTagMap::fromVariant(const QVariant& variant)
{
	clear();

	QVariantHomogenousList list("AMLMTagMapEntries", "entry");

	/// @todo REMOVE
	if(variant.canConvert<QVariantInsertionOrderedMap>())
	{
		QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();
		QVariant list_qvar = map.value("m_the_map");
		Q_ASSERT(list_qvar.isValid());
		Q_ASSERT(list_qvar.canConvert<QVariantHomogenousList>());
		list = list_qvar.value<QVariantHomogenousList>();
	}
	else if (variant.canConvert<QVariantHomogenousList>())
	{
		list = variant.value<QVariantHomogenousList>();
	}


	for(auto entry = list.cbegin(); entry != list.cend(); ++entry)
	{
		QVariantInsertionOrderedMap kvpair_map = entry->value<QVariantInsertionOrderedMap>();
		QVariantHomogenousList qvector_of_values("values", "value");

		QString key;
		key = kvpair_map.value("key").toString();
		QVariant qvar_values = kvpair_map.value("values");
		Q_ASSERT(qvar_values.isValid());
		Q_ASSERT(qvar_values.canConvert<QVariantHomogenousList>());
		qvector_of_values = qvar_values.value<QVariantHomogenousList>();
		for(const auto& value : qAsConst(qvector_of_values))
		{
			m_the_map.insert(std::make_pair(tostdstr(key), tostdstr(value.toString())));
		}
	}

	return;
}

//AMLMTagMap::operator QVariant() const
//{
//	return toVariant();
//}

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


