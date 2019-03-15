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
// None yet.

// Qt5
#include <QMetaType>
#include <QVariant>

// Ours
#include <future/InsertionOrderedMap.h>
#include <future/QVariantHomogenousList.h>


using QVariantInsertionOrderedMap = InsertionOrderedMap<QString, QVariant>;
Q_DECLARE_METATYPE(QVariantInsertionOrderedMap);

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


/**
 * @todo Maybe factor out a QVariantList with tag names only.
 */
class SerializableQVariantList : public QVariantHomogenousList, public virtual ISerializable
{
public:
	M_GH_RULE_OF_FIVE_DEFAULT_C21(SerializableQVariantList);

	SerializableQVariantList(const QString& list_tag, const QString& list_item_tag)
	{
		set_tag_names(list_tag, list_item_tag);
	}
	~SerializableQVariantList() override = default;

	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;
};

Q_DECLARE_METATYPE(SerializableQVariantList);


/// @name Some helper templates.
/// @{

template <class MapType, class StringType, class MemberType>
void map_insert_or_die(MapType& map, const StringType& key, const MemberType& member)
{
//	static_assert(qMetaTypeId<MemberType>() != 0, "");
	// InsertionOrderedMap<>::insert() currently returns void.
//	using iterator_type = typename MapType::iterator;
	/*iterator_type it =*/ map.insert( key , QVariant::fromValue<MemberType>( member ) );
//	if(it == map.end())
//	{
//		// Insertion failed for some reason.
//		throw QException();
//	}
}

//template <class MapType, class StringType, class MemberType>
//void map_insert_or_die(MapType& map, const StringType& key, const MemberType& member)
//{
//	using iterator_type = typename MapType::iterator;
//	iterator_type it = map.insert( key , member );
//	if(it == map.end())
//	{
//		// Insertion failed for some reason.
//		throw QException();
//	}
//}

/// @}


#endif /* SRC_LOGIC_SERIALIZATION_ISERIALIZABLE_H_ */


