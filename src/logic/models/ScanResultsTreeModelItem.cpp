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
 * @file ScanResultsTreeModelItem.cpp
 */

#include "ScanResultsTreeModelItem.h"

// Std C++
#include <memory>

// Qt5
#include <QXmlStreamReader>

// Ours
#include <utils/DebugHelpers.h>
#include <logic/DirScanResult.h>
#include "ScanResultsTreeModelXMLTags.h"


ScanResultsTreeModelItem::ScanResultsTreeModelItem(const DirScanResult& dsr, AbstractTreeModelItem* parent)
	: AbstractTreeModelItem(parent)
{
	m_dsr = dsr;
}

ScanResultsTreeModelItem::~ScanResultsTreeModelItem()
{
}

QVariant ScanResultsTreeModelItem::data(int column) const
{
	/// Map column and @todo role to the corresponding data.

	switch(column)
	{
	case 0:
		return toqstr(m_dsr.getDirProps());
	case 1:
		return QUrl(m_dsr.getMediaExtUrl());
	case 2:
		return QUrl(m_dsr.getSidecarCuesheetExtUrl());
	default:
		qWr() << "data() request for unknown column:" << column;
		break;
	}

	return QVariant();
}

int ScanResultsTreeModelItem::columnCount() const
{
	return 3;
}


QVariant ScanResultsTreeModelItem::toVariant() const
{
	QVariantMap map;

	/// @todo Will be more fields, justifying the map vs. value?
	/// @todo Need the parent here too?  Probably needs to be handled by the parent, but maybe for error detection.

	map.insert(SRTMItemTagToXMLTagMap[SRTMItemTag::DIRSCANRESULT], m_dsr.toVariant());

	return map;
}

void ScanResultsTreeModelItem::fromVariant(const QVariant &variant)
{
	QVariantMap map = variant.toMap();

	auto dsr_in_variant = map.value(SRTMItemTagToXMLTagMap[SRTMItemTag::DIRSCANRESULT]);
	m_dsr.fromVariant(dsr_in_variant);

	/// @todo Make a list or something.
	auto kv_pair_in_variant = map.value(SRTMItemTagToXMLTagMap[SRTMItemTag::TEST_PAIR_0]);
	m_dsr.fromVariant(kv_pair_in_variant);

}

AbstractTreeModelItem *
ScanResultsTreeModelItem::do_create_default_constructed_child_item(AbstractTreeModelItem *parent, int num_columns)
{
	SRTMItem_LibEntry* child_item;

	child_item = new SRTMItem_LibEntry(parent);

	return child_item;
}

bool ScanResultsTreeModelItem::derivedClassSetData(int column, const QVariant& value)
{
	// We have at the moment only a DirScanResult, not sure we need to set data by column.
	return true;
}

bool ScanResultsTreeModelItem::derivedClassInsertColumns(int insert_before_column, int num_columns)
{
	/// @todo Again only a DirScanResult.  Not sure what to do here.
	/// Qt5 TreeModel has this:
	///   bool TreeModel::insertColumns(int position, int columns, const QModelIndex &parent)
	///		beginInsertColumns(parent, position, position + columns - 1);
	///		success = rootItem->insertColumns(position, columns);
	///		endInsertColumns();
	/// So it's up to the root item to decide what to do, not the model.
	/// The root item calls child items and they add/remove QVariant's as required.

	return true;
}

bool ScanResultsTreeModelItem::derivedClassRemoveColumns(int first_column_to_remove, int num_columns)
{
	/// @todo Again only a DirScanResult.  Not sure what to do here.
	/// Qt5 TreeModel has this:
	///   bool TreeModel::insertColumns(int position, int columns, const QModelIndex &parent)
	///		beginInsertColumns(parent, position, position + columns - 1);
	///		success = rootItem->insertColumns(position, columns);
	///		endInsertColumns();
	/// So it's up to the root item to decide what to do, not the model.

	return true;
}


/////////// @todo SRTMItem_LibEntry

bool SRTMItem_LibEntry::derivedClassSetData(int column, const QVariant& value)
{
	return ScanResultsTreeModelItem::derivedClassSetData(column, value);
}

bool SRTMItem_LibEntry::derivedClassInsertColumns(int insert_before_column, int num_columns)
{
	return ScanResultsTreeModelItem::derivedClassInsertColumns(insert_before_column, num_columns);
}

bool SRTMItem_LibEntry::derivedClassRemoveColumns(int first_column_to_remove, int num_columns)
{
	return ScanResultsTreeModelItem::derivedClassRemoveColumns(first_column_to_remove, num_columns);
}

QVariant SRTMItem_LibEntry::data(int column) const
{
	switch(column)
	{
		case 0:
			return QVariant::fromValue(m_key);
			break;
		case 1:
			return QVariant::fromValue(m_val);
			break;
		default:
			return QVariant();
			break;
	}
}

int SRTMItem_LibEntry::columnCount() const
{
	return 2;
}

QVariant SRTMItem_LibEntry::toVariant() const
{
	QVariantMap map;

	/// @todo Will be more fields, justifying the map vs. value?
	/// @todo Need the parent here too?  Probably needs to be handled by the parent, but maybe for error detection.

	map.insert("TEST_COL0", QVariant::fromValue(m_key));
	map.insert("TEST_COL1", QVariant::fromValue(m_val));

	return map;
}

void SRTMItem_LibEntry::fromVariant(const QVariant& variant)
{
	QVariantMap map = variant.toMap();

}

/////////// @todo SRTMItem_LibEntry
