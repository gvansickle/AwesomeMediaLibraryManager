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

ScanResultsTreeModelItem::ScanResultsTreeModelItem(const QVariant& variant, const std::shared_ptr<AbstractTreeModelItem>& parent, UUIncD id)
	: BASE_CLASS({}, parent, id)
{
//	M_WARNING("TODO: DECODE VARIANT");
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
	/*X(XMLTAG_NUM_COLUMNS, num_columns, (qulonglong)m_item_data.size())*/ \
	/*X(XMLTAG_ITEM_DATA_SIZE, item_data_size, (qulonglong)m_item_data.size())*/ \
	/*X(XMLTAG_NUM_CHILDREN, num_children, (qulonglong)m_child_items.size())*/ \
	X(XMLTAG_CHILD_NODE_LIST, child_node_list, nullptr)


/// Strings to use for the tags.
using strviw_type = QLatin1Literal;

///// Strings to use for the tags.
#define X(field_tag, tag_string, var_name) static const strviw_type field_tag ( # tag_string );
	M_DATASTREAM_FIELDS(X);
#undef X


QVariant ScanResultsTreeModelItem::toVariant() const
{
	QVariantInsertionOrderedMap map;

	// Overwrite any class info added by the above.
//	set_map_class_info(this, &map);
	set_map_class_info(std::string("ScanResultsTreeModelItem"), &map);

	// Set the xml:id.
	map.insert_attributes({{"xml:id", get_prefixed_uuid()}});

	/// @todo Will be more fields, justifying the map vs. value?
	/// @todo Need the parent here too?  Probably needs to be handled by the parent, but maybe for error detection.

	map_insert_or_die(map, XMLTAG_DIRSCANRESULT, m_dsr.toVariant());

#define X(field_tag, tag_string, var_name) map_insert_or_die(map, field_tag, var_name);
	M_DATASTREAM_FIELDS(X);
#undef X

	QVariantHomogenousList child_var_list(XMLTAG_CHILD_NODE_LIST, "child");
	for(auto& it : m_child_items)
	{
		list_push_back_or_die(child_var_list, it->toVariant());
	}
	map_insert_or_die(map, XMLTAG_CHILD_NODE_LIST, child_var_list);

	return map;
}

void ScanResultsTreeModelItem::fromVariant(const QVariant &variant)
{
	QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();

	// Overwrite any class info added by the above.
//	dump_map_class_info(this, &map);

	auto uuid = map.get_attr("xml:id", "");
	set_prefixed_uuid(uuid);

#define X(field_tag, tag_string, var_name) map_read_field_or_warn(map, field_tag, var_name);
	M_DATASTREAM_FIELDS(X);
#undef X

	map_read_field_or_warn(map, XMLTAG_DIRSCANRESULT, &m_dsr);

	QVariantHomogenousList child_var_list(XMLTAG_CHILD_NODE_LIST, "child");
	child_var_list = map.value(XMLTAG_CHILD_NODE_LIST).value<QVariantHomogenousList>();
	Q_ASSERT(child_var_list.size() > 0);

	append_children_from_variant<SRTMItem_LibEntry>(this, child_var_list);

#if 0////
	auto model_ptr_base = m_model.lock();
	Q_ASSERT(model_ptr_base);
	auto model_ptr = std::dynamic_pointer_cast<ScanResultsTreeModel>(model_ptr_base);
	auto parent_id = getId();

	/// NEEDS TO BE IN MODEL HERE.
	Q_ASSERT(isInModel());

	std::vector<std::shared_ptr<AbstractTreeModelItem>> new_child_item_vec;
	for(const QVariant& child_variant : child_list)
	{
		qDb() << "READING CHILD ITEM INTO ScanResultsTreeModelItem:" << child_variant.typeName();

		auto id = model_ptr->requestAddSRTMLibEntryItem(child_variant, parent_id);
		auto new_child = model_ptr->getItemById(id);
		Q_ASSERT(new_child);
	}
#endif
}

//std::shared_ptr<ScanResultsTreeModel> ScanResultsTreeModelItem::getTypedModel() const
//{
//	return std::dynamic_pointer_cast<ScanResultsTreeModel>(m_model.lock());
//}

void ScanResultsTreeModelItem::setDirscanResults(const DirScanResult& dsr)
{
	m_dsr = dsr;
}


