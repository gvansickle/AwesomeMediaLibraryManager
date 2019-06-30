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
#include <QtConcurrent>

// Ours
#include <utils/RegisterQtMetatypes.h> ///< For <cstdint> metatypes.
#include <future/future_type_traits.hpp>
#include "SerializationExceptions.h"
#include <future/InsertionOrderedMap.h>
#include "QVariantHomogenousList.h"


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

class IUUIDSerializable : public virtual ISerializable
{
public:
	IUUIDSerializable() : m_uuid(QUuid::createUuid()) {};
	~IUUIDSerializable() override = default;

	QUuid m_uuid;
};

Q_DECLARE_METATYPE(IUUIDSerializable*);

struct KeyValuePair
{
	std::string m_key;
	std::string m_value;
};

class AttributedQVariant
{
public:
	AttributedQVariant() {};
	AttributedQVariant(const QVariant& var, std::initializer_list<KeyValuePair> kvpairs) : m_variant(var)
	{
		for(const auto& it : kvpairs)
		{
			m_key_value_pairs.insert({it.m_key, it.m_value});
		}
	};
	virtual ~AttributedQVariant() {};

	std::map<std::string, std::string> m_key_value_pairs;
	QVariant m_variant;
};

Q_DECLARE_METATYPE(AttributedQVariant);

template <class OutMapType>
void qviomap_from_qvar_or_die(OutMapType* map_out, const QVariant& var_in)
{
//	throwif(!var_in.isValid(), SerializationException("input QVariant is not valid"));
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

/// Ptrs to ISerializable-implementing members.
template <class MapType, class StringType, class RawMemberType>
void map_read_field_or_warn(const MapType& map, const StringType& key, RawMemberType member)
{
	if constexpr(std::is_convertible_v<RawMemberType, ISerializable*>)
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

//template <class MapType, class StringType, class MemberType,
//		  REQUIRES(!std::is_base_of_v<ISerializable, std::shared_ptr<MemberType>>)>
//void map_read_field_or_warn(const MapType& map, const StringType& key, MemberType member)
//{
//	static_assert (!std::is_base_of_v<ISerializable, MemberType>, "DEDUCTION FAILED");
//	if(QVariant qvar = map.value(key); qvar.isValid())
//	{
//M_WARNING("Does this need to transfer sharedness?");
//		member = qvar.value<MemberType>();
//	}
//	else
//	{
//		qWr() << "Couldn't read field:" << key;
//	}
//}

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

/**
 * Use a blocking call to blockingMappedReduce() to serialize the entries in
 * @a in_list to the output QVariantHomogenousList @a out_list.
 * @note Like it says on the tin, the map part is concurrent, so it has to be threadsafe.
 *
 * @tparam InListType  A container of pointers to ISerializable-derived objects.
 */
template <class InListType>
void list_blocking_map_reduce_push_back_or_die(QVariantHomogenousList& out_list, const InListType& in_list)
{
	if constexpr(std::is_base_of_v<ISerializable, typename InListType::value_type::element_type>)
	{
		struct mapped_reduce_helper
		{
			/**
			 * The Reduce callback.  Doesn't really need to be in this struct, but it was handy.
			 */
			static void add_to_list(QVariantHomogenousList& list, const QVariant& entry)
			{
				list.push_back(entry);
			};

			/**
			 * The mapping functor.
			 */
			using result_type = QVariant;
			result_type operator()(const typename InListType::value_type& incoming)
			{
				// Convert to a QVariant and return.
				QVariant qvar = incoming->toVariant();
				if(!qvar.isValid())
				{
					throw SerializationException("invalid QVariant conversion.");
				}

				return qvar;
			}
		};

		mapped_reduce_helper mapper;

		// Qt5.12 QFuture iterators are basically broken. STL-style iterators won't block.  Java-style will detach, and new results won't show up.
		// So we'll try blocking, which we're doing all this to avoid.
		// OrderedReduce == reduce func called in order of input sequence,
		// SequentialReduce == reduce func called by one thread at a time.
		auto initial_list_tag = out_list.get_list_tag();
		auto initial_list_item_tag = out_list.get_list_item_tag();
		out_list = QtConcurrent::blockingMappedReduced(in_list.cbegin(), in_list.cend(), mapper,
													   mapped_reduce_helper::add_to_list,
											QtConcurrent::OrderedReduce|QtConcurrent::SequentialReduce);
		// Reset the tag names.
		out_list.set_tag_names(initial_list_tag, initial_list_item_tag);
	}
	else
	{
		static_assert(dependent_false_v<InListType>, "No matching template for params: list_blocking_map_reduce_push_back_or_die");
	}
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
	static_assert(std::is_same_v<ListType, QVariantHomogenousList>
				  || std::is_same_v<ListType, QVariantList>,
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

/**
 * Use a blocking call to blockingMappedReduce() to serialize the entries in
 * @a in_list to the output QVariantHomogenousList @a out_list.
 * @note Like it says on the tin, the map part is concurrent, so it has to be threadsafe.
 *
 * @tparam InListType  A container of pointers to ISerializable-derived objects.
 */
template <class InListType, /*class OutListValueType,*/ /*template<typename>*/ class OutListType>
void list_blocking_map_reduce_read_all_entries_or_warn(const InListType& in_list, OutListType/*<OutListValueType>*/* p_out_list)
{
	static_assert(std::is_same_v<InListType, QVariantHomogenousList> || std::is_same_v<InListType, QVariantList>,
	              "InListType is not a recognized list type");

	using OutListValueType = typename OutListType::value_type;

	// We're trying to make this serial code concurrent:
	//		for(const QVariant& e : in_list)
	//		{
	//			Q_ASSERT(e.isValid());
	//			std::shared_ptr<LibraryEntry> libentry = std::make_shared<LibraryEntry>();
	//			libentry->fromVariant(e);
	//			m_lib_entries.push_back(libentry);
	//		}

	if constexpr(std::is_base_of_v<ISerializable, typename OutListType/*<OutListValueType>*/::value_type::element_type>)
	{
		// For this case, in_list should hold shared_ptr's to ISerializable's, e.g.:
		// QVariantList<std::shared_ptr<LibraryEntry>>

		// This will be e.g. std::shared_ptr<LibraryEntry>
		using OutListPtrType = typename OutListType/*<OutListValueType>*/::value_type;
		// This will be e.g. LibraryEntry:
		using OutElementType = typename OutListType/*<OutListValueType>*/::value_type::element_type;

		struct mapped_reduce_helper
		{
			/**
			 * The Reduce callback.  Doesn't really need to be in this struct, but it was handy.
			 */
			static void add_to_list(OutListType/*<OutListValueType>*/& out_list, const OutListPtrType& new_entry)
			{
				out_list.push_back(new_entry);
			};

			/**
			 * The mapping functor.
			 */
			using result_type = OutListPtrType;
			result_type operator()(const QVariant& incoming)
			{
				// Convert from a QVariant and return.
				throwif<SerializationException>(!incoming.isValid(), "invalid QVariant in read.");
				OutListPtrType ptr_to_new_element = std::make_shared<OutElementType>();
				ptr_to_new_element->fromVariant(incoming);
				return ptr_to_new_element;
			}
		};

		mapped_reduce_helper mapper;

		// Qt5.12 QFuture iterators are basically broken. STL-style iterators won't block.  Java-style will detach, and new results won't show up.
		// So we'll try blocking, which we're doing all this to avoid.
		// OrderedReduce == reduce func called in order of input sequence,
		// SequentialReduce == reduce func called by one thread at a time.

		OutListType/*<OutListValueType>*/ throwaway_list;
		throwaway_list = QtConcurrent::blockingMappedReduced(in_list.cbegin(), in_list.cend(), mapper,
		                                               mapped_reduce_helper::add_to_list,
		                                               QtConcurrent::OrderedReduce|QtConcurrent::SequentialReduce);

		std::move(throwaway_list.begin(), throwaway_list.end(), std::back_inserter(*p_out_list));
	}
	else
	{
		static_assert(dependent_false_v<InListType>, "No matching template for params: list_blocking_map_reduce_push_back_or_die");
	}
}

/// @}


#endif /* SRC_LOGIC_SERIALIZATION_ISERIALIZABLE_H_ */


