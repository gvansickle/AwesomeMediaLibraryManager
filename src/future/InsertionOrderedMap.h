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

// Std C++
#include <stdexcept>
#include <deque>
#include <map>
#include <tuple>

// Qt5
#include <utils/QtHelpers.h>
#include <QVariant>

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
	using key_type = KeyType;
	using mapped_type = ValueType;
	using value_type = std::pair<const KeyType, ValueType>;
	using underlying_container_type = std::deque<value_type>;
	using const_iterator = typename underlying_container_type::const_iterator;
	using iterator = typename underlying_container_type::iterator;
	/// @}

private:
	using uc_size_type = typename underlying_container_type::size_type;

public:
	InsertionOrderedMap() = default;
	InsertionOrderedMap(const InsertionOrderedMap& other) = default;
	virtual ~InsertionOrderedMap() = default;

	void insert(const KeyType key, const ValueType value)
	{
		insert(std::make_pair(key, value));
	}

	void insert(const value_type& key_val)
	{
		m_vector_of_elements.push_back(key_val);
		m_map_of_keys_to_vector_indices.insert(std::make_pair(key_val.first, m_vector_of_elements.size()-1));
	}

	const mapped_type& at(const KeyType& key) const
	{
		auto it = this->find(key);
		if(it == m_vector_of_elements.cend())
		{
			throw std::out_of_range(std::string("InsertionOrderedMap(): no such element at():")/* + std::to_string(key)*/);
		}
		return it->second;
	}

	const_iterator find( const KeyType& key ) const
	{
		auto it_index = m_map_of_keys_to_vector_indices.find(key);
		if(it_index == m_map_of_keys_to_vector_indices.end())
		{
			return m_vector_of_elements.cend();
		}

		Q_ASSERT(it_index->first == m_vector_of_elements[it_index->second].first);
		return m_vector_of_elements.cbegin() + it_index->second;
	};

	const ValueType value(const KeyType& key, const ValueType& default_value = ValueType()) const
	{
		auto cit = this->find(key);
		if(cit == this->cend())
		{
			// No such key.
			return default_value;
		}
		return cit->second;
	}

	const_iterator cbegin() const { return std::cbegin(m_vector_of_elements); };
	const_iterator begin() const { return this->cbegin(); }
	const_iterator cend() const { return std::cend(m_vector_of_elements); };
	const_iterator end() const { return this->cend(); }

#if 1 // Qt5
//	QTH_FRIEND_QDATASTREAM_OPS(InsertionOrderedMap);

	// Conversion operator to a QVariant.
	operator QVariant() const
	{
		return QVariant::fromValue(*this);
//		QVariant retval{m_vector_of_elements};
///// @todo Correct?
////		retval = m_vector_of_elements;
//
//		return retval;
	}

#endif // Qt5



protected:

	underlying_container_type m_vector_of_elements;
	// Map of keys to indexes in the vector.
	std::map<KeyType, uc_size_type> m_map_of_keys_to_vector_indices;

};

#if 1 // Qt5
Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE(InsertionOrderedMap);

#endif // Qt5

#endif /* SRC_FUTURE_INSERTIONORDEREDMAP_H_ */
