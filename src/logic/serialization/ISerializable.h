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
#include <QUuid>
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
	/**
	 * Default constructor creates a new UUID.
	 */
	explicit ISerializable()
		: m_uuid_prefix("xmlid_"), m_uuid( QUuid::createUuid() ) {};
	virtual ~ISerializable() = default;

	/**
	 * Override in derived classes to serialize to a QVariantMap or QVariantList.
	 */
	virtual QVariant toVariant() const = 0;

	/**
	 * Override in derived classes to serialize from a QVariantMap or QVariantList.
	 */
	virtual void fromVariant(const QVariant& variant) = 0;

	/// Sort of clumsy way to deal with this it seems.
//	template <class MapType>
//	void AddUUIDToVariantMap(MapType* map) const
//	{
//		auto uuid = get_prefixed_uuid();
//		map->insert(QString("xml:id"), QVariant::fromValue(uuid));
//	}

	/**
	 * Remove any UUID from the @a map and set this's uuid with it.
	 * @tparam MapType
	 * @param map
	 */
//	template <class MapType>
//	void GetUUIDFromVariantMap(const MapType& map)
//	{
//		auto uuid = map.take("xml:id");
//		set_prefixed_uuid(uuid);
//	}

	explicit operator QVariant() const { return toVariant(); };

	bool isUuidNull() const { return m_uuid.isNull() || m_uuid_prefix.empty(); };

//	void set_prefix(std::string prefix)
//	{
//		Q_ASSERT(m_uuid_prefix.empty());
//		m_uuid_prefix = prefix;
//	}

	std::string get_prefix() const
	{
		return m_uuid_prefix;
	}

	/**
	 * Returns the prefix + UUID of this node as a string.
	 * @return
	 */
	std::string get_prefixed_uuid() const
	{
		if(m_uuid.isNull())
		{
			// No valid prefix + UUID set.
			return std::string();
		}
		return m_uuid_prefix + m_uuid.toString(QUuid::WithoutBraces).toStdString();
	}

	/**
	 * Split the incoming string into prefix and UUID, and sets it to the incoming value.
	 * @param uuid_string
	 */
	void set_prefixed_uuid(const std::string& uuid_string)
	{
		m_uuid_prefix = "xmlid_"; ///< @todo
		m_uuid = QUuid::fromString(toqstr(uuid_string).remove(toqstr(m_uuid_prefix)));
		Q_ASSERT(!m_uuid.isNull());
	}

	/// EXPERIMENTAL ORM
//	int m_int_uuid;
	/// The GUID used to ID this item in the database etc.
	QUuid m_uuid;
	/// The prefix to prepend if XML, since xml:id's need to start with a non-digit character.
	std::string m_uuid_prefix {"xmlid_"};

};

Q_DECLARE_METATYPE(ISerializable*);
//Q_DECLARE_METATYPE(ISerializable&);
Q_DECLARE_INTERFACE(ISerializable, "ISerializable") // define this out of namespace scope

class IUUIDSerializable : public virtual ISerializable
{
public:
	M_GH_RULE_OF_FIVE_DEFAULT_C21(IUUIDSerializable);
	explicit IUUIDSerializable(std::string uuid_prefix)
		: m_uuid_prefix(uuid_prefix), m_uuid((!uuid_prefix.empty())?QUuid::createUuid():QUuid("null")) {};
	~IUUIDSerializable() override = default;

	bool isUuidNull() const { return m_uuid.isNull() || m_uuid_prefix.empty(); };

//	std::string get_prefixed_uuid() const
//	{
//		return m_uuid_prefix + m_uuid.toString(QUuid::WithoutBraces).toStdString();
//	}

//	void set_prefixed_uuid(std::string uuid_string)
//	{
//		Q_ASSERT(!m_uuid_prefix.empty());
//		m_uuid = QUuid::fromString(toqstr(uuid_string).remove(toqstr(m_uuid_prefix)));
//		Q_ASSERT(m_uuid.isNull());
//	}

protected:

	std::string m_uuid_prefix;
	QUuid m_uuid;
};

Q_DECLARE_METATYPE(IUUIDSerializable*);
Q_DECLARE_INTERFACE(IUUIDSerializable, "IUUIDSerializable") // define this out of namespace scope



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

	SerializableQVariantList(const QString& list_tag, const QString& list_item_tag)
		: QVariantHomogenousList(list_tag, list_item_tag)
		{};

	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;
};

Q_DECLARE_METATYPE(SerializableQVariantList);


/// @}


#endif /* SRC_LOGIC_SERIALIZATION_ISERIALIZABLE_H_ */


