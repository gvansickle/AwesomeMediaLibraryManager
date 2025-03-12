/*
 * Copyright 2019, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
#include <vector>
#include <utility> // For std::pair<>.

// Qt
#include <QVariant>
#include <QMetaType>
#include <QDebug>

// Ours
#include <utils/QtHelpers.h>
#include <future/guideline_helpers.h>
#include <future/future_algorithms.h>
#include <utils/DebugHelpers.h>

//template <typename KeyType, typename ValueType>
//struct value_type_grvs
//{
//	const KeyType first;
//	ValueType second;
//};

// Fwd declarations.
template <class KeyType, class ValueType> class InsertionOrderedMap;
template <class KeyType, class ValueType> QDebug operator<<(QDebug out, const InsertionOrderedMap<KeyType, ValueType>& obj);

/**
 * A map which maintains the insertion order of its keys.  The main operational difference between this and
 * std::map is that iteration over [begin(), end()) will be done in insertion order, not key-sorted order.
  */
template <typename KeyType, typename ValueType>
class InsertionOrderedMap
{
public:
	/// @name Member types
	/// @{
	using key_type = KeyType;
	using mapped_type = ValueType;
	using value_type = std::pair<KeyType, ValueType>;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = ValueType&;
	using const_reference = const ValueType&;
	using underlying_container_type = std::deque<value_type>;
	using const_iterator = typename underlying_container_type::const_iterator;
	using iterator = typename underlying_container_type::iterator;
	/// @}

	int m_metatype_id = 0;
	std::string m_class;

private:
	using uc_size_type = typename underlying_container_type::size_type;

public:
	M_GH_RULE_OF_FIVE_DEFAULT_C21(InsertionOrderedMap);
	~InsertionOrderedMap() = default;

	/// Copy-through-QVariant constructor.
	explicit InsertionOrderedMap(const QVariant& variant)
	{
		Q_ASSERT(variant.isValid());
		using qvartype = InsertionOrderedMap<KeyType, ValueType>;
		Q_ASSERT(variant.canConvert< qvartype >());
		*this = variant.value< InsertionOrderedMap<KeyType, ValueType> >();
	}

	void insert(const KeyType& key, const ValueType& value)
	{
		insert(std::make_pair(key, value));
	}

	void insert(const value_type& key_val)
	{
		m_vector_of_elements.push_back(key_val);
		m_map_of_keys_to_vector_indices.insert(std::make_pair(key_val.first, m_vector_of_elements.size()-1));
	}

	template <template<typename> typename VectorLike>
	void insert_multi(const KeyType& key, const VectorLike<ValueType>& vector_of_values)
	{
		for(const auto& val : vector_of_values)
		{
			this->insert(key, val);
		}
	}

	void erase(const key_type& key)
	{
		// Remove/erase from vector/deque.
//		std::experimental::erase_if(this->m_vector_of_elements, [=](const auto& it){ return it->first == key; });
		auto it = std::find_if(this->m_vector_of_elements.begin(), this->m_vector_of_elements.end(),
											   [=](const auto& val){ return val.first == key; });
		if(it != this->m_vector_of_elements.end())
		{
			this->m_vector_of_elements.erase(it);
		}
		else
		{
			qWr() << "NO KEY FOUND TO ERASE:" << key;
		}
//		this->m_vector_of_elements.erase();
//				(std::remove_if(
//											 this->m_vector_of_elements.begin(),
//											 this->m_vector_of_elements.end(), [=](const key_type& key){ return ;}),
//										 this->m_vector_of_elements.end());
		// Remove/erase from map.
//		std::experimental::erase_if(this->m_map_of_keys_to_vector_indices, [=](const auto& it){ return it.first == key; });
		auto removed_ct = this->m_map_of_keys_to_vector_indices.erase(key);
		if(removed_ct < 1)
		{
			qWr() << "NO MAP KEY FOUND TO ERASE:" << key;
		}
	}

	void clear() noexcept
	{
		m_vector_of_elements.clear();
		m_map_of_keys_to_vector_indices.clear();
		m_attribute_map.clear();
	}

	const mapped_type& at(const KeyType& key) const
	{
		auto it = this->find(key);
		if(it == m_vector_of_elements.cend())
		{
			throw std::out_of_range(std::string("InsertionOrderedMap(): no such element at():") + key.toUtf8().toStdString());
		}
		return it->second;
	}

	iterator find(const key_type& key)
	{
		auto it_index = m_map_of_keys_to_vector_indices.find(key);
		if (it_index == m_map_of_keys_to_vector_indices.end())
		{
			return m_vector_of_elements.end();
		}

		Q_ASSERT(it_index->first == m_vector_of_elements[it_index->second].first);
		return m_vector_of_elements.begin() + it_index->second;
	}

	const_iterator find( const key_type& key ) const
	{
		auto it_index = m_map_of_keys_to_vector_indices.find(key);
		if(it_index == m_map_of_keys_to_vector_indices.end())
		{
			return m_vector_of_elements.cend();
		}

		Q_ASSERT(it_index->first == m_vector_of_elements[it_index->second].first);
		return m_vector_of_elements.cbegin() + it_index->second;
	}

	bool contains(const KeyType& key) const
	{
		auto it = this->find(key);
		if(it == m_vector_of_elements.cend())
		{
			// Didn't find it.
			return false;
		}
		return true;
	}

	const value_type value(const KeyType& key, const ValueType& default_value = ValueType()) const
	{
		auto cit = this->find(key);
		if(cit == this->cend())
		{
			// No such key.
			return default_value;
		}
		return cit->second;
	}

	/// @name Attribute access
	/// @{

	template <class VectorLike>
	void insert_attributes(const VectorLike& attr_list)
	{
		for(const auto& it : attr_list)
		{
			// Dup attributes get replaced.
			m_attribute_map.insert_or_assign(it.qualifiedName().toString().toStdString(), it.value().toString().toStdString());
		}
	}

	void insert_attributes(std::initializer_list<std::pair<std::string, std::string>> attr_list)
	{
		for(const auto& it : attr_list)
		{
			m_attribute_map.insert_or_assign(it.first, it.second);
		}
	}

	void set_attr(const std::string& key, const std::string& value)
	{
		m_attribute_map.insert_or_assign(key, value);
	}

	std::string get_attr(const std::string& key) const
	{
		auto it = m_attribute_map.find(key);
		if(it == m_attribute_map.end())
		{
			return std::string();
		}
		else
		{
			return it->second;
		}
	}

	std::string get_attr(const std::string& key, const std::string& default_val) const
	{
		std::string retval = default_val;

		try
		{
			retval = this->get_attr(key);
		}
		catch(...)
		{
			retval = default_val;
		}
		return retval;
	}


	using attr_map_type = std::map<std::string, std::string>;
	attr_map_type get_attrs() const
	{
		return m_attribute_map;
	}

	bool contains_attr(const std::string& attr_name)
	{
		return m_attribute_map.count(attr_name) > 0;
	}

	/// @}

	iterator begin() { return m_vector_of_elements.begin(); }
	const_iterator begin() const { return m_vector_of_elements.begin(); }
	const_iterator cbegin() const noexcept { return std::cbegin(m_vector_of_elements); };

	iterator end()	{ return m_vector_of_elements.end(); }
	const_iterator end() const { return m_vector_of_elements.end(); }
	const_iterator cend() const noexcept { return std::cend(m_vector_of_elements); };

	bool empty() const { return m_vector_of_elements.empty(); };
	size_t size() const { return m_vector_of_elements.size(); };

#if 1 // Qt5

	/// @name Debug
	/// @{
	/// @link See SO https://stackoverflow.com/a/4661372
//	QTH_DECLARE_FRIEND_QDEBUG_OP(InsertionOrderedMap<KeyType,ValueType>);
	friend QDebug operator<< <KeyType, ValueType>(QDebug out, const InsertionOrderedMap<KeyType, ValueType>& obj);

	/// @}


//	QTH_FRIEND_QDATASTREAM_OPS(InsertionOrderedMap);

	/**
	 * Conversion operator to a QVariant.
	 * @note This is deliberately not explicit so that it is a workalike to QMap wrt QVariants.
	 */
	operator QVariant() const
	{
		return QVariant::fromValue(*this);
	}

#endif // Qt5


private:

	/// Arbitrary "name" string.  Mainly for debug, happy path shouldn't try to use this for anything.
	std::string m_name {"[name_not_set]"};

	underlying_container_type m_vector_of_elements;
	// Map of keys to indexes in the vector.
	std::map<KeyType, uc_size_type> m_map_of_keys_to_vector_indices;

	/// Map of attributes.
	std::map<std::string, std::string> m_attribute_map;

};

template <typename KeyType, typename ValueType>
QDebug operator<<(QDebug dbg, const InsertionOrderedMap<KeyType, ValueType>& map)
{
    // Q_ASSERT(0);// TODO
	return dbg;
}

// Q_DECLARE_METATYPE(InsertionOrderedMap)

// Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(InsertionOrderedMap);
Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE(InsertionOrderedMap);
using QVariantInsertionOrderedMap = InsertionOrderedMap<QString, QVariant>;
// Q_DECLARE_METATYPE_TEMPLATE_2ARG(InsertionOrderedMap);
Q_DECLARE_METATYPE(QVariantInsertionOrderedMap);


#endif /* SRC_FUTURE_INSERTIONORDEREDMAP_H_ */
