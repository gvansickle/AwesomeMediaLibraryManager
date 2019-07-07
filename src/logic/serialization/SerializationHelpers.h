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
 * @file SerializationHelpers.h
 */
#ifndef SRC_LOGIC_SERIALIZATION_SERIALIZATIONHELPERS_H_
#define SRC_LOGIC_SERIALIZATION_SERIALIZATIONHELPERS_H_

#include <type_traits>

// Ours
#include <future/future_type_traits.hpp>
#include <future/cpp14_concepts.hpp>
#include <boost/callable_traits/function_type.hpp>
#include <logic/DirScanResult.h>
#include "ISerializable.h"

/**
 * Templates.  Unbelievable.
 */
template <class T>
using toVariant_t = decltype(std::declval<T>().toVariant());
template <class T>
using has_toVariant = future_detection::is_detected<toVariant_t, T>;

/// @name Some helper templates.
/// @{

/**
 *
 * @param member  The ISerializer-derived member variable to insert.
 */
template <class MapType, class StringType>
void map_insert_or_die(MapType& map, const StringType& key, const std::nullptr_t member)
{
	(void)map;
	(void)member;
	qDb() << "###### MIOD NULL, KEY:" << key;
	// Do nothing.
}

template <class MapType, class StringType>
void map_insert_or_die(MapType& map, const StringType& key, const ISerializable& member)
{
//	qDb() << "MIOD 2:" << key;
	map.insert(key, member.toVariant());
}

template <class MapType, class StringType, class ValueType,
		  REQUIRES(!std::is_base_of_v<ISerializable, ValueType>)>
void map_insert_or_die(MapType& map, const StringType& key, const ValueType& member)
{
//	qDb() << "MIOD 2b:" << key;
	map.insert(key, QVariant::fromValue(member));
}



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
 * @todo Does this handle an incoming QVariant correctly?
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



namespace AMLM
{

/// @name Read entries from a maplike type into the apropriate @a member.
/// @{

template <class MapType, class StringType, class RawMemberType>
void map_read_field_or_warn(const MapType& map, const StringType& key, RawMemberType member)
{
	// Regardless, get the qvar out of the map.
	QVariant qvar = map.value(key);
	if(!qvar.isValid())
	{
		qWr() << "Couldn't read value of key '" << key << "' from map:" << map;
		return;
	}

	if constexpr(std::is_convertible_v<RawMemberType, ISerializable*> || std::is_convertible_v<RawMemberType, IUUIDSerializable*>)
	{
		// Ptrs to ISerializable-implementing members.
		// This value should know how to read itself from a QVariant.
		member->fromVariant(qvar);
	}
	else if constexpr(std::is_convertible_v<RawMemberType, ISerializable&> || std::is_convertible_v<RawMemberType, IUUIDSerializable&>)
	{
		// Ref to an ISerializable.
		member.fromVariant(qvar);
	}
	else if constexpr(std::is_pointer_v<RawMemberType>)
	{
		*member = qvar.value<std::remove_pointer_t<RawMemberType>>();
	}
	else
	{
		/// @todo I think this is an assert.
		// Try to read the value as a de-pointered RawMemberType.
		Q_ASSERT(qvar.canConvert<RawMemberType>());
		member = qvar.value<RawMemberType>();
	}
}

/**
 * Specialization for ignoring member read list entries with a nullptr variable.
 * @param map
 * @param key
 * @param member
 */
template <class MapType, class StringType>
void map_read_field_or_warn(const MapType& map, const StringType& key, std::nullptr_t member)
{
	// Do nothing.
}

/// @}

} /* namespace AMLM */

#endif /* SRC_LOGIC_SERIALIZATION_SERIALIZATIONHELPERS_H_ */
