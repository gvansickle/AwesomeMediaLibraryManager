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
 * @file AMLMTagMap.h
 */
#ifndef SRC_LOGIC_AMLMTAGMAP_H_
#define SRC_LOGIC_AMLMTAGMAP_H_

// Std C++
#include <map>
#include <string>
#include <vector>

// Qt5
#include <QMetaType>
#include <QVariant>

// Ours.
#include <future/guideline_helpers.h>
#include <logic/serialization/ISerializable.h>

using TagMap = std::map<std::string, std::vector<std::string>>;
//Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE(std::map);
//Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(std::vector);
//Q_DECLARE_METATYPE(std::string);
Q_DECLARE_METATYPE(TagMap);

/*
 *
 */
class AMLMTagMap : public virtual ISerializable
{

	/// Member Types
private:
	using Key = std::string;
	using T = std::string;
	using underlying_container_type = std::multimap<Key, T>;

public:
	using key_type = Key;
	using mapped_type = T;
	using value_type = std::pair<const Key, T>;
	using const_iterator = typename underlying_container_type::const_iterator;
	using iterator = typename underlying_container_type::iterator;

	/// Member functions.
public:
	M_GH_RULE_OF_ZERO(AMLMTagMap);

	AMLMTagMap& operator=(const TagMap& tagmap);

	/**
	 * Returns a reference to the first value of the matching key.
	 * Performs an insertion of a default-constructed value if such key does not already exist.
	 * @param key
	 * @return
	 */
	std::vector<mapped_type> operator[](const Key& key) __attribute__((deprecated));

	iterator insert(const value_type& value) { return m_the_map.insert(value); };

	/// @name Lookup.
	iterator find( const Key& x );
	const_iterator find( const Key& x ) const;

	std::pair<iterator, iterator> equal_range(const Key& key)
	{
		return m_the_map.equal_range(key);
	}
	std::pair<const_iterator, const_iterator> equal_range(const Key& key) const
	{
		return m_the_map.equal_range(key);
	}

	std::vector<mapped_type> equal_range_vector(const Key& key) const;


	iterator begin() { return m_the_map.begin(); }
	iterator end() { return m_the_map.end(); }
	const_iterator begin() const { return m_the_map.begin(); };
	const_iterator end() const { return m_the_map.end(); };
	const_iterator cbegin() const noexcept { return m_the_map.cbegin(); };
	const_iterator cend() const noexcept { return m_the_map.cend(); };

	/// @name Serialization
	/// @{
	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;
	/// @}

private:

	underlying_container_type m_the_map;
};

Q_DECLARE_METATYPE(AMLMTagMap);

#endif /* SRC_LOGIC_AMLMTAGMAP_H_ */
