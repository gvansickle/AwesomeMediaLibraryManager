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
 * @file AMLMTagMap.h
 */
#ifndef SRC_LOGIC_AMLMTAGMAP_H_
#define SRC_LOGIC_AMLMTAGMAP_H_

// Std C++
#include <map>
#include <string>
#include <vector>

// Qt
#include <QMetaType>
#include <QVariant>
#include <QMultiMap>

// TagLib
#include <taglib/xiphcomment.h> //< Unfortunately needed here for "typedef Map< String, StringList > FieldListMap".

// Ours.
#include <future/guideline_helpers.h>
#include <utils/QtHelpers.h>
#include <logic/serialization/ISerializable.h>

using TagMap = std::map<std::string, std::vector<std::string>>;
Q_DECLARE_METATYPE(TagMap);

Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE(std::multimap);

/**
 * Multimap of std::string key/std::string value pairs.
 * @todo Pretty sure this really should be an insertion-ordered multimap.
 */
class AMLMTagMap final : public ISerializable
{

	/// @name Member Types
    ///@{
private:
	using Key = std::string;
	using T = std::string;

public:
	using underlying_container_type = std::multimap<Key, T>;
	using key_type = Key;
	using mapped_type = std::string;
	using value_type = std::pair<const Key, T>;
	using size_type = typename underlying_container_type::size_type;
	using const_iterator = typename underlying_container_type::const_iterator;
	using iterator = typename underlying_container_type::iterator;
    ///@}

	// Member functions.
public:
    M_GH_RULE_OF_FIVE_DEFAULT_C21(AMLMTagMap)
	~AMLMTagMap() override = default;

	/**
	 * Assignment from a TagMap.
	 */
	AMLMTagMap& operator=(const TagMap& tagmap);

	/**
	 * Assignment from a TagLib PropertyMap.
	 */
	AMLMTagMap& operator=(const TagLib::PropertyMap& prop_map);

	/**
	 * Assignment from a TagLib FieldListMap.
	 * For some reason, the key is either a TagLib String or ByteVector depending on the file format, hence templates.
	 * OggXiphComment: https://taglib.org/api/classTagLib_1_1Ogg_1_1XiphComment.html#aaad472ea2921a034bb8d7842a55459b9
	 */
	template <class StringLike>
	AMLMTagMap& operator=(const TagLib::Map<StringLike, TagLib::StringList>& taglib_field_list_map)
	{
		clear();

		// Iterate over key+value_vector pairs.
		for(const auto & it : taglib_field_list_map)
		{
			// Iterate over the value, which is a vector of values.
			for(const auto& valit : it.second)
			{
				std::string key;
				if constexpr(std::is_same_v<TagLib::ByteVector, decltype(it.first)>)
				{
					key = tostdstr(it.first.data());
				}
				else
				{
					key = tostdstr(it.first);
				}
				m_the_map.insert(std::make_pair(key, tostdstr(valit)));
			}
		}

		return *this;
	}

	friend
	bool operator==(const AMLMTagMap& lhs, const AMLMTagMap& rhs);

	/**
	 * Returns a reference to the first value of the matching key.
	 * Performs an insertion of a default-constructed value if such key does not already exist.
	 * @param key
	 * @return
	 */
	std::vector<mapped_type> operator[](const Key& key) __attribute__((deprecated));

    iterator insert(const value_type& value) { return m_the_map.insert(value); }

	/**
	 * Insert @a value under @a key.
	 * @note A two-parameter insert() is conspicuously absent from std::map/multimap and QMap.  I'm sure I'll
	 *       discover the reason after this interface is fully entrenched in the codebase.
	 */
    iterator insert(const key_type& key, const mapped_type& value) { return m_the_map.insert({key, value}); }

	iterator insert_if_empty(const key_type& key, const mapped_type& value);

	auto erase(auto it) { return m_the_map.erase(it); }

	/// @name Lookup.
	/// @{
	iterator find( const Key& x );
	const_iterator find( const Key& x ) const;
	bool contains( const Key& key ) const;
	size_type count(const Key& key) const;

	std::pair<iterator, iterator> equal_range(const Key& key)
	{
		return m_the_map.equal_range(key);
	}
	std::pair<const_iterator, const_iterator> equal_range(const Key& key) const
	{
		return m_the_map.equal_range(key);
	}

	/**
	 * Returns the list of all keys, in sorted order.
	 */
	std::vector<key_type> keys() const;

	std::vector<mapped_type> equal_range_vector(const Key& key) const;
	/// @}

	/// Size.
	underlying_container_type::size_type size() const { return m_the_map.size(); }
	bool empty() const { return m_the_map.empty(); }

	/*[[clang::reinitializes]]*/ void clear();

	iterator begin() { return m_the_map.begin(); }
	iterator end() { return m_the_map.end(); }
    const_iterator begin() const { return m_the_map.begin(); }
    const_iterator end() const { return m_the_map.end(); }
    const_iterator cbegin() const noexcept { return m_the_map.cbegin(); }
    const_iterator cend() const noexcept { return m_the_map.cend(); }

	template <class CallbackType>
	void foreach_pair(CallbackType&& t) const
	{
		std::pair<const_iterator, const_iterator> range;

		// Iterate through the multimap's elements by key.
		for(auto it = m_the_map.begin(); it != m_the_map.end(); it = range.second)
		{
			// Get the range of the current key
			range = m_the_map.equal_range(it->first);

			// Iterate over the values in the range.
			for(auto valit = range.first; valit != range.second; ++valit)
			{
				std::invoke(t, toqstr(valit->first), toqstr(valit->second));
			}
		}
	}

	void dump(const std::string& name) const;

	/**
	 * Merge the @a source map into this AMLMTagMap.
	 * @param source  Reference to the source AMLMTagMap.
	 */
	void merge(const AMLMTagMap& source);

	/// @name Serialization
	/// @{
	QTH_FRIEND_QDATASTREAM_OPS(AMLMTagMap);
	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;
	/// @}

	/// @name Debug
	/// @{
	QTH_DECLARE_FRIEND_QDEBUG_OP(AMLMTagMap);
	/// @}

private:

	underlying_container_type m_the_map;
};

bool operator==(const AMLMTagMap& lhs, const AMLMTagMap& rhs);


Q_DECLARE_METATYPE(AMLMTagMap);
Q_DECLARE_METATYPE(AMLMTagMap::underlying_container_type);
QTH_DECLARE_QDATASTREAM_OPS(AMLMTagMap);

/**
 * Helper function template which inserts <@a tagname, @a value> into @a map.
 * If value is integral, it gets converted to a string representation and that string in inserted into the map.
 * @tparam T
 * @tparam Stringlike
 * @param map            Any maplike with an .insert(std::string) member.
 * @param tagname
 * @param value
 */
template <class T, class Stringlike>
void AMLMTagMap_convert_and_insert(AMLMTagMap& map, const Stringlike& tagname, const T& value)
{
	if constexpr(std::is_integral_v<T>)
	{
		// It's an integral of some sort, convert to a string.
		map.insert(tagname, std::to_string(value));
	}
	else
	{
		map.insert(tagname, value);
	}
}

/**
 * A work-alike to std::erase_if for AMLMTagMap.
 */
template <class Predicate>
AMLMTagMap::size_type erase_if(AMLMTagMap& c, Predicate pred)
{
	auto original_size = c.size();
	auto it = c.begin();
	while (it != c.end())
	{
		if (pred(*it))
		{
			it = c.erase(it);
		}
		else
		{
			++it;
		}
	}
	return original_size - c.size();
}


#endif /* SRC_LOGIC_AMLMTAGMAP_H_ */
