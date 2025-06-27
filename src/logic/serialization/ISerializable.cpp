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
 * @file ISerializable.h
 */

#include "ISerializable.h"

// C++
#include <utility> // for pair.

// Qt
#include <QDebug>
#include <QMetaType>

// Ours, Qt/KF related.
#include <utils/DebugHelpers.h>
#include <utils/RegisterQtMetatypes.h>

// Ours
#include <future/InsertionOrderedMap.h>

using std_pair_QString_QVariant = std::pair<const QString, QVariant>;
Q_DECLARE_METATYPE(std_pair_QString_QVariant);

AMLM_QREG_CALLBACK([](){
	qIn() << "Registering InsertionOrderedMap<QString, QVariant>";
	qRegisterMetaType<InsertionOrderedMap<QString, QVariant>>();
	qRegisterMetaType<std_pair_QString_QVariant>();
	qRegisterMetaType<SerializableQVariantList>();
});


QVariant SerializableQVariantList::toVariant() const
{
	Q_ASSERT(!m_list_tag.isNull());
	Q_ASSERT(!m_list_tag.isEmpty());
	Q_ASSERT(!m_list_item_tag.isNull());
	Q_ASSERT(!m_list_item_tag.isEmpty());

	InsertionOrderedMap<QString, QVariant> map;
	// Return a QMap with a single QVariant(SerializableQVariantList) item.

    SerializableQVariantList list(m_list_tag, m_list_item_tag);
    for(const auto& element : m_the_list)
    {
        list.push_back(element);
    }

	map.insert(m_list_tag, list);
	return map;
}

void SerializableQVariantList::fromVariant(const QVariant& variant)
{
	Q_ASSERT(!m_list_tag.isNull());
	Q_ASSERT(!m_list_tag.isEmpty());
	Q_ASSERT(!m_list_item_tag.isNull());
	Q_ASSERT(!m_list_item_tag.isEmpty());

	Q_ASSERT((variant.canConvert<InsertionOrderedMap<QString, QVariant>>()));

	InsertionOrderedMap<QString, QVariant> map = variant.value<InsertionOrderedMap<QString, QVariant>>();

	Q_ASSERT(map.contains(m_list_tag));
    // qDb() << "list type:" << map.at(m_list_tag).metaType();
    // Q_ASSERT(map.at(m_list_tag).canConvert<SerializableQVariantList>());
    Q_ASSERT(map.at(m_list_tag).canConvert<QVariantHomogenousList>());

    auto qvl = map.at(m_list_tag).value<QVariantHomogenousList>();

	for(const QVariant& entry : qvl)
	{
		this->push_back(entry);
	}
}
