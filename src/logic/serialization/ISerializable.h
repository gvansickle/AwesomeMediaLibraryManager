/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

//Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE(InsertionOrderedMap);
//Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(InsertionOrderedMap);

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
	SerializableQVariantList() = default;
	SerializableQVariantList(const SerializableQVariantList& other) = default;
	SerializableQVariantList(const QString& list_tag, const QString& list_item_tag)
	{
		set_tag_names(list_tag, list_item_tag);
	}
	~SerializableQVariantList() override = default;

	SerializableQVariantList& operator=(SerializableQVariantList other)
	{
		std::swap(*this, other);
		return *this;
	}

//	void set_tag_names(const QString& list_tag, const QString& list_item_tag)
//	{
//		m_list_tag = list_tag;
//		m_list_item_tag = list_item_tag;
//	}

	const QString& get_list_tag() const
	{
		return m_list_tag;
	}

	const QString& get_list_item_tag() const
	{
		return m_list_item_tag;
	}

	QVariant toVariant() const override
	{
		Q_ASSERT(!m_list_tag.isNull());
		Q_ASSERT(!m_list_tag.isEmpty());
		Q_ASSERT(!m_list_item_tag.isNull());
		Q_ASSERT(!m_list_item_tag.isEmpty());

		QVariantMap map;
		// Return a QMap with a single QVariant(SerializableQVariantList) item.
		map.insert(m_list_tag, QVariant::fromValue(*this));
		return map;
	}

	void fromVariant(const QVariant& variant) override
	{
		Q_ASSERT(!m_list_tag.isNull());
		Q_ASSERT(!m_list_tag.isEmpty());
		Q_ASSERT(!m_list_item_tag.isNull());
		Q_ASSERT(!m_list_item_tag.isEmpty());

		QVariantMap map = variant.toMap();
		QVariantList qvl = map.value(m_list_tag).toList();

		for(const auto& e : qvl)
		{
			this->push_back(e);
		}
	}

//private:
//	QString m_list_tag;
//	QString m_list_item_tag;
};

Q_DECLARE_METATYPE(SerializableQVariantList);

#endif /* SRC_LOGIC_SERIALIZATION_ISERIALIZABLE_H_ */


