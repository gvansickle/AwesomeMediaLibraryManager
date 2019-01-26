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
 * @file InsertionOrderedMap.h
 */
#ifndef SRC_FUTURE_INSERTIONORDEREDMAP_H_
#define SRC_FUTURE_INSERTIONORDEREDMAP_H_

#include <vector>
#include <map>
#include <tuple>

/**
 * A map which maintains the insertion order of its keys.  The only operational difference between this and
 * std::map is that iteration over [begin(), end()) will be done in insertion order, not key-sorted order.
 *
 */
template <typename KeyType, typename ValueType>
class InsertionOrderedMap
{

public:
	/// @name Member types
	/// @{
	using underlying_container_type = std::vector<std::tuple<KeyType, ValueType>>;
	using const_iterator = typename underlying_container_type::const_iterator;
	using iterator = typename underlying_container_type::iterator;
	/// @}

public:
	InsertionOrderedMap() = default;
	virtual ~InsertionOrderedMap() = default;

protected:

	std::vector<std::tuple<KeyType, ValueType>> m_vector_of_elements;
	std::map<KeyType, ValueType> m_map_of_elements;

};

#endif /* SRC_FUTURE_INSERTIONORDEREDMAP_H_ */
