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
#include <utils/RegisterQtMetatypes.h> ///< For <cstdint> metatypes.
#include "SerializationExceptions.h"
#include <future/InsertionOrderedMap.h>
#include <QVariantHomogenousList.h>


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

template <class OutMapType>
void qviomap_from_qvar_or_die(OutMapType* map_out, const QVariant& var_in)
{
	Q_ASSERT(var_in.isValid());
	Q_ASSERT(var_in.canConvert<OutMapType>());
	*map_out = var_in.value<OutMapType>();
}

/**
 */
class SerializableQVariantList : public QVariantHomogenousList, public virtual ISerializable
{
public:
	M_GH_RULE_OF_FIVE_DEFAULT_C21(SerializableQVariantList);
	~SerializableQVariantList() override = default;

	SerializableQVariantList(const QString& list_tag, const QString& list_item_tag) : QVariantHomogenousList(list_tag, list_item_tag) {};

	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;
};

Q_DECLARE_METATYPE(SerializableQVariantList);


/// @name Some helper templates.
/// @{

template <class MapType, class StringType>
void map_insert_or_die(MapType& map, const StringType& key, const ISerializable& member)
{
	map.insert( key , member.toVariant() );
}

template <class MapType, class StringType, class MemberType,
		  REQUIRES(!std::is_base_of_v<ISerializable, MemberType>)> ///< Not clear why these are required to get the ISerializable above to be preferred.
void map_insert_or_die(MapType& map, const StringType& key, const MemberType& member)
{
	static_assert (!std::is_base_of_v<ISerializable, MemberType>, "DEDUCTION FAILED");
	map.insert( key , QVariant::fromValue<MemberType>( member ) );

}

/// @name Read entries from a maplike type into the apropriate @a member.
/// @{

template <class MapType, class StringType>
void map_read_field_or_warn(const MapType& map, const StringType& key, ISerializable* member)
{
	if(QVariant qvar = map.value(key); qvar.isValid())
	{
		member->fromVariant(qvar);
	}
	else
	{
		qWr() << "Couldn't read field:" << key;
	}
}

template <class MapType, class StringType, class MemberType,
		  REQUIRES(!std::is_base_of_v<ISerializable, MemberType>)>
void map_read_field_or_warn(const MapType& map, const StringType& key, MemberType* member)
{
	static_assert (!std::is_base_of_v<ISerializable, MemberType>, "DEDUCTION FAILED");
	if(QVariant qvar = map.value(key); qvar.isValid())
	{
		*member = qvar.value<MemberType>();
	}
	else
	{
		qWr() << "Couldn't read field:" << key;
	}
}

/// @}


/// @name Functions for pushing values/list-likes to a QList<QVariant>.
/// @{

/**
 * Push a single entry given by @a member onto the @a list.
 * @overload For ISerializable @a member's.
 */
template <class ListType>
void list_push_back_or_die(ListType& list, const ISerializable& member)
{
	// ISerializable knows how to turn itself into a QVariant.
	QVariant qvar = member.toVariant();
	if(!qvar.isValid())
	{
		throw SerializationException("Coudn't push_back() to list.");
	}

	list.push_back(qvar);
}

/**
 * Push a single entry given by @a member onto the @a list.
 */
template <class ListType, class MemberType,
		  REQUIRES(!std::is_base_of_v<ISerializable, MemberType>)>
void list_push_back_or_die(ListType& list, const MemberType& member)
{
	static_assert (!std::is_base_of_v<ISerializable, MemberType>, "DEDUCTION FAILED");

	QVariant qvar = QVariant::fromValue<MemberType>( member );
	if(!qvar.isValid())
	{
		throw SerializationException("Coudn't push_back() to list.");
	}

	list.push_back(qvar);
}

/// @}

/**
 * Read all entries in @a list and copy them to @a p_list.
 * @tparam ListType  A list-like of QVariant's, each holding a single @a ListEntryType.
 */
template <class ListType, class ListEntryType, template<typename> class OutListType,
		  REQUIRES(!std::is_base_of_v<ISerializable, ListEntryType>)>
void list_read_all_fields_or_warn(const ListType& list, OutListType<ListEntryType>* p_list,
								  const char* caller = "UNKNOWN"/*__builtin_FUNCTION()*/)
{
	static_assert(std::is_same_v<ListType, QVariantHomogenousList> || std::is_same_v<ListType, QVariantList>,
				  "Not a list type");

	// Make sure return list is empty.
	p_list->clear();

	auto num_entries = list.size();
	if(num_entries == 0)
	{
		qWr() << M_ID_VAL(caller) << "LIST IS EMPTY:" << list;
		return;
	}

	for(const QVariant& qvar : qAsConst(list))
	{
		throwif<SerializationException>(!qvar.isValid(), "Invalid QVariant");
		throwif<SerializationException>(!qvar.canConvert<ListEntryType>(), "Can't convert QVariant contents of list to ListEntryType");

		p_list->push_back(qvar.value<ListEntryType>());
	}
}

/**
 * Read all entries in @a list and copy them to @a p_list.
 * @overload For ISerializable @a ListEntryType's.
 * @tparam ListType  A list-like of QVariant's, each holding a single @a ListEntryType.
 */
template <class ListType, class ListEntryType, template<typename> class OutListType,
		  REQUIRES(std::is_base_of_v<ISerializable, ListEntryType>)>
void list_read_all_fields_or_warn(const ListType& list, OutListType<ListEntryType>* p_list,
								  const char* caller = "UNKNOWN"/*__builtin_FUNCTION()*/)
{
	static_assert(std::is_same_v<ListType, QVariantHomogenousList> || std::is_same_v<ListType, QVariantList>,
				  "Not a list type");

	// Make sure return list is empty.
	p_list->clear();

	auto num_entries = list.size();
	if(num_entries == 0)
	{
		qWr() << M_ID_VAL(caller) << "LIST IS EMPTY:" << list;
		return;
	}

	for(const QVariant& qvar : qAsConst(list))
	{
		throwif<SerializationException>(!qvar.isValid(), "Invalid QVariant");
//		throwif<SerializationException>(!qvar.canConvert<ListEntryType>(), "Can't convert QVariant contents of list to ListEntryType");

		// It's an ISerializable-derived class.
		ListEntryType list_entry = qvar.value<ListEntryType>();
//		ISerializable* Iser = dynamic_cast<ISerializable*>(qvar.value<ListEntryType>());
		list_entry.fromVariant(qvar);
		p_list->push_back(list_entry);
	}
}

/// @}


#endif /* SRC_LOGIC_SERIALIZATION_ISERIALIZABLE_H_ */


