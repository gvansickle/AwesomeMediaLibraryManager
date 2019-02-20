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

// Qt5
#include <QMetaType>
#include <QVariant>

// Ours.
#include <future/guideline_helpers.h>
#include <logic/serialization/ISerializable.h>

using TagMap = std::map<std::string, std::vector<std::string>>;

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

	template <typename T>
	AMLMTagMap& operator=(const T& map)
	{
		/// @todo
		Q_ASSERT(0);
		return *this;
	}

	mapped_type& operator[](const Key& key)
	{
		iterator it = m_the_map.find(key);
		if(it != m_the_map.end())
		{
			return it->second;
		}
	};
	mapped_type& operator[](const Key& key) const;

	iterator insert(const value_type& value) { return m_the_map.insert(value); };

	iterator find( const Key& x );

	const_iterator find( const Key& x ) const;

	template< class K >
	iterator find( const K& x );

	template< class K >
	const_iterator find( const K& x ) const;

	iterator begin() { return std::begin(m_the_map); }
	iterator end() { return std::end(m_the_map); }
	const_iterator cbegin() const { return std::cbegin(m_the_map); };
	const_iterator cend() const { return std::cend(m_the_map); };

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
