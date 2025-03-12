/*
 * Copyright 2018, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
#include "AbstractTreeModelHeaderItem.h"
#include "ScanResultsTreeModel.h"
#include "SRTMItemLibEntry.h"
#include <LibraryEntry.h>
#include <serialization/SerializationHelpers.h>


ScanResultsTreeModelItem::ScanResultsTreeModelItem(const DirScanResult& dsr, const std::shared_ptr<AbstractTreeModelItem>& parent, UUIncD id)
	: BASE_CLASS({}, parent, id), m_dsr(dsr)
{
}

ScanResultsTreeModelItem::ScanResultsTreeModelItem(const std::shared_ptr<AbstractTreeModelItem>& parent, UUIncD id)
	: BASE_CLASS({}, parent, id)
{
}

ScanResultsTreeModelItem::~ScanResultsTreeModelItem()
{
}

QVariant ScanResultsTreeModelItem::data(int column, int role) const
{
	// Map column and role to the corresponding data.

	if((role != Qt::ItemDataRole::DisplayRole) && (role != Qt::ItemDataRole::EditRole))
	{
		return QVariant();
	}

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
		M_TODO("This should just return QVariant() I think");
		return BASE_CLASS::data(column, role);
		break;
	}

	return QVariant();
}

int ScanResultsTreeModelItem::columnCount() const
{
	return 3;
}


#define M_DATASTREAM_FIELDS(X) \
	/* TAG_IDENTIFIER, tag_string, member_field, var_name */ \
	X(XMLTAG_DIRSCANRESULT, m_dsr, nullptr) \
	X(XMLTAG_CHILD_ITEM_MAP, child_item_map, nullptr) \
	X(XMLTAG_ITEM_DATA_LIST, item_data_list, nullptr)

#define M_DATASTREAM_FIELDS_CONTSIZES(X) \
	X(XMLTAG_NUM_COLUMNS, num_columns, m_item_data) \
	X(XMLTAG_ITEM_DATA_LIST_SIZE, item_data_list_size, m_item_data) \
	X(XMLTAG_NUM_CHILDREN, num_children, m_child_items)

/// Strings to use for the tags.
using strviw_type = QLatin1String;

///// Strings to use for the tags.
#define X(field_tag, tag_string, var_name) static const strviw_type field_tag ( # tag_string );
	M_DATASTREAM_FIELDS(X);
	M_DATASTREAM_FIELDS_CONTSIZES(X);
#undef X

QVariant ScanResultsTreeModelItem::toVariant() const
{
	InsertionOrderedMap<QString, QVariant> map;

	// Add class info to the map we'll return.
	set_map_class_info(this, &map);

	// Set the xml:id.
	map.insert_attributes({{"xml:id", get_prefixed_uuid()}});

	// Add the m_item_data QVariants to the map under key XMLTAG_ITEM_DATA_LIST/"<item_data_list>".
	item_data_to_variant(&map);

	map_insert_or_die(map, XMLTAG_DIRSCANRESULT, m_dsr);

// #define X(field_tag, tag_string, var_name) map_insert_or_die(map, field_tag, var_name);
// 	M_DATASTREAM_FIELDS(X);
// #undef X

	// Serialize out Child nodes.
	children_to_variant(&map);

	return map;
}

void ScanResultsTreeModelItem::fromVariant(const QVariant &variant)
{
	InsertionOrderedMap<QString, QVariant> map = variant.value<InsertionOrderedMap<QString, QVariant>>();

	// Overwrite any class info added by the above.
//	dump_map_class_info(this, &map);

	auto uuid = map.get_attr("xml:id", "");
	set_prefixed_uuid(uuid);

// #define X(field_tag, tag_string, var_name) map_read_field_or_warn(map, field_tag, var_name);
// 	M_DATASTREAM_FIELDS(X);
// #undef X

	// Read in this class's additional fields.
	map_read_field_or_warn(map, XMLTAG_DIRSCANRESULT, &m_dsr);

	QVariantHomogenousList child_var_list(XMLTAG_CHILD_NODE_LIST, "child");
	child_var_list = map.at(XMLTAG_CHILD_NODE_LIST).value<QVariantHomogenousList>();
	Q_ASSERT(child_var_list.size() > 0);

	append_children_from_variant<SRTMItem_LibEntry>(this, child_var_list);

#if 0////
	auto model_ptr_base = m_model.lock();
	Q_ASSERT(model_ptr_base);
	auto model_ptr = std::dynamic_pointer_cast<ScanResultsTreeModel>(model_ptr_base);
	auto parent_id = getId();

	/// NEEDS TO BE IN MODEL HERE.
	Q_ASSERT(isInModel());
	auto model_ptr = m_model.lock();
	Q_ASSERT(model_ptr);

	InsertionOrderedStrVarMap child_map;// (XMLTAG_CHILD_ITEM_MAP, "child");
	child_map = map.value(XMLTAG_CHILD_ITEM_MAP).value<InsertionOrderedStrVarMap>();
	qDb() << M_ID_VAL(child_map.size());

	AMLM_ASSERT_EQ(num_children, child_map.size());

//	if(num_children > 0)
//	{
		children_from_str_var_map(child_map);
//	}

	qDb() << M_ID_VAL(child_map.size());
}

//void ScanResultsTreeModelItem::clear()
//{
////	m_dsr.clear();
//}

void ScanResultsTreeModelItem::setDirscanResults(const DirScanResult& dsr)
{
	m_dsr = dsr;
}


