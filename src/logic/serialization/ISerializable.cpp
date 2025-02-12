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
 * @file ISerializable.h
 */

#include "ISerializable.h"

// Qt5
#include <QDebug>
#include <QMetaType>

// Ours, Qt5/KF5 related.
#include <utils/DebugHelpers.h>
#include <utils/RegisterQtMetatypes.h>

// Ours
#include <future/InsertionOrderedMap.h>

using std_pair_QString_QVariant = std::pair<const QString, QVariant>;
Q_DECLARE_METATYPE(std_pair_QString_QVariant);

AMLM_QREG_CALLBACK([](){
	qIn() << "Registering InsertionOrderedMap<QString, QVariant> alias QVariantInsertionOrderedMap";
	qRegisterMetaType<InsertionOrderedMap<QString, QVariant>>();
	qRegisterMetaType<std_pair_QString_QVariant>();
//	qRegisterMetaType<InsertionOrderedMap<QString, QVariant>>("QVariantInsertionOrderedMap");
//	AMLMRegisterQFlagQStringConverters<DirScanResult::DirPropFlags>();
});


QVariant SerializableQVariantList::toVariant() const
{
	Q_ASSERT(!m_list_tag.isNull());
	Q_ASSERT(!m_list_tag.isEmpty());
	Q_ASSERT(!m_list_item_tag.isNull());
	Q_ASSERT(!m_list_item_tag.isEmpty());

	InsertionOrderedMap<QString, QVariant> map;
	// Return a QMap with a single QVariant(QVariantHomogenousList) item.
	/// @note Slicing warning, but this is ok here.
	QVariantHomogenousList list = *this;

	map.insert(m_list_tag, QVariant::fromValue(list));
	return map;
}

void SerializableQVariantList::fromVariant(const QVariant& variant)
{
	Q_ASSERT(!m_list_tag.isNull());
	Q_ASSERT(!m_list_tag.isEmpty());
	Q_ASSERT(!m_list_item_tag.isNull());
	Q_ASSERT(!m_list_item_tag.isEmpty());

	InsertionOrderedMap<QString, QVariant> map = variant.value<InsertionOrderedMap<QString, QVariant>>();

	Q_ASSERT(map.contains(m_list_tag));
//	qDb() << M_ID_VAL(m_list_tag);
	Q_ASSERT(QMetaType::canConvert(QMetaType::fromType<decltype(map.at(m_list_tag))>(),QMetaType::fromType<QVariantHomogenousList>()));

	QVariantHomogenousList qvl = map.at(m_list_tag).value<QVariantHomogenousList>();

	for(const auto& e : qvl)
	{
		this->push_back(e);
	}
}
