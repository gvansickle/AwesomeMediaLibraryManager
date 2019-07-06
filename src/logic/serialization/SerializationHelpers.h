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

#include "ISerializable.h"


/**
 *
 * @param member  The ISerializer-derived member variable to insert.
 */
template <class MapType, class StringType>
void map_insert_or_die(MapType& map, const StringType& key, const std::nullptr_t member)
{
	// Do nothing.
}

template <class MapType, class StringType, class MemberType>
void map_insert_or_die(MapType& map, const StringType& key, const MemberType member)
{
	if constexpr(std::is_convertible_v<MemberType, ISerializable*> || std::is_convertible_v<MemberType, IUUIDSerializable*>)
	{
		map.insert( key , member->toVariant());
	}
	else if constexpr(std::is_convertible_v<MemberType, ISerializable&> || std::is_convertible_v<MemberType, IUUIDSerializable&>)
	{
		map.insert( key , member.toVariant());
	}
	else
	{
		map.insert( key , QVariant::fromValue<MemberType>( member ) );
	}

//	static_assert (!std::is_base_of_v<ISerializable, MemberType>, "DEDUCTION FAILED");
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
