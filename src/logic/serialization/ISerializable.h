/*
 * Copyright 2018, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
 * @file ISerializable.h
 */

#ifndef SRC_LOGIC_SERIALIZATION_ISERIALIZABLE_H_
#define SRC_LOGIC_SERIALIZATION_ISERIALIZABLE_H_

// Std C++
#include <type_traits>

// Qt5
#include <QMetaType>
#include <QVariant>

// Ours
#include "SerializationExceptions.h"
#include <future/InsertionOrderedMap.h>
#include <future/QVariantHomogenousList.h>


using QVariantInsertionOrderedMap = InsertionOrderedMap<QString, QVariant>;
Q_DECLARE_METATYPE(QVariantInsertionOrderedMap);

/**
 * Abstract interface for adding serialization capabilities to classes which
 * derive from and implement this interface.
 *
 * @see ISerializer.
 */
class ISerializable
{
public:
	virtual ~ISerializable() = default;

	/**
	 * Override in derived classes to serialize to a QVariantMap or QVariantList.
	 */
	virtual QVariant toVariant() const = 0;

	/**
	 * Override in derived classes to serialize from a QVariantMap or QVariantList.
	 */
	virtual void fromVariant(const QVariant& variant) = 0;
};


/**
 */
class SerializableQVariantList : public QVariantHomogenousList, public virtual ISerializable
{
public:
	M_GH_RULE_OF_FIVE_DEFAULT_C21(SerializableQVariantList);

	SerializableQVariantList(const QString& list_tag, const QString& list_item_tag)
	{
		set_tag_names(list_tag, list_item_tag);
	}
	~SerializableQVariantList() override = default;

	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;
};

Q_DECLARE_METATYPE(SerializableQVariantList);


/// @name Some helper templates.
/// @{

template <class MapType, class StringType, class MemberType>
void map_insert_or_die(MapType& map, const StringType& key, const MemberType& member)
{
//	static_assert(qMetaTypeId<MemberType>() != 0, "");
	// InsertionOrderedMap<>::insert() currently returns void.
//	using iterator_type = typename MapType::iterator;
	/*iterator_type it =*/ map.insert( key , QVariant::fromValue<MemberType>( member ) );
//	if(it == map.end())
//	{
//		// Insertion failed for some reason.
//		throw QException();
//	}
}

template <class MapType, class StringType, class MemberType>
void map_read_field_or_warn_fromvar(const MapType& map, const StringType& key, MemberType* member)
{
    if(auto qvar = map.value(key); qvar.isValid())
    {
		member->fromVariant(qvar);
    }
    else
    {
        qWr() << "Couldn't read field:" << key;
    }
}

template <class MapType, class StringType, class MemberType>
void map_read_field_or_warn(const MapType& map, const StringType& key, MemberType* member)
{
	if(auto qvar = map.value(key); qvar.isValid())
	{
		*member = qvar.template value<MemberType>();
	}
	else
	{
		qWr() << "Couldn't read field:" << key;
	}
}

template <class MapType, class StringType, class MemberType>
auto map_read_field_or_warn_fromvar(const MapType& map, const StringType& key, const MemberType& member) -> MemberType
{
	MemberType retval {};

	if(QVariant qvar = map.value(key); qvar.isValid())
	{
		retval = qvar.value<MemberType>();
	}
	else
	{
		qWr() << "Couldn't read field:" << key;
	}

	return retval;
}

template <class ListType, class MemberType>
void list_push_back_or_die(ListType& list, const MemberType& member)
{
	QVariant qvar = QVariant::fromValue<MemberType>( member );
	if(!qvar.isValid())
	{
		throw SerializationException("Coudn't push_back() to list.");
	}

	list.push_back(qvar);
}

template <class ListType, class ListEntryType, template<typename> class OutListType>
void list_read_all_fields_or_warn(const ListType& list, OutListType<ListEntryType>* p_list)
{
	p_list->clear();
	auto num_entries = list.size();
	if(num_entries == 0)
	{
		qWr() << "LIST IS EMPTY:" << list;
		return;
	}

	for(const QVariant& qvar : list)
	{
		if(!qvar.isValid())
		{
			qWr() << "Failed reading list:" << qvar << list;
			return;
		}

		p_list->push_back(qvar.value<ListEntryType>());
	}
}

/// @}


#endif /* SRC_LOGIC_SERIALIZATION_ISERIALIZABLE_H_ */


