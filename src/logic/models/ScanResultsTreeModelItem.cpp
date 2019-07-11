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
#include "ScanResultsTreeModel.h"
#include <LibraryEntry.h>
#include <serialization/SerializationHelpers.h>

std::shared_ptr<ScanResultsTreeModelItem> ScanResultsTreeModelItem::construct(const DirScanResult& dsr, const std::shared_ptr<ScanResultsTreeModel> model, bool is_root)
{
	std::shared_ptr<ScanResultsTreeModelItem> self(new ScanResultsTreeModelItem(dsr, model, is_root));
	baseFinishConstruct(self);
	return self;
}

std::shared_ptr<ScanResultsTreeModelItem> ScanResultsTreeModelItem::construct(const QVariant& variant,
																			  std::shared_ptr<ScanResultsTreeModel> model, bool is_root)
{
	std::shared_ptr<ScanResultsTreeModelItem> self(new ScanResultsTreeModelItem(model, is_root));
	baseFinishConstruct(self);
	self->fromVariant(variant);

	return self;
}

ScanResultsTreeModelItem::ScanResultsTreeModelItem(const DirScanResult& dsr, const std::shared_ptr<ScanResultsTreeModel> model, bool is_root)
	: BASE_CLASS(model, is_root), m_dsr(dsr)
{
}

ScanResultsTreeModelItem::ScanResultsTreeModelItem(std::shared_ptr<ScanResultsTreeModel> model, bool is_root)
	: BASE_CLASS(model, is_root)
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
	set_map_class_info(this, &map);

	/// @todo Will be more fields, justifying the map vs. value?
	/// @todo Need the parent here too?  Probably needs to be handled by the parent, but maybe for error detection.

	map_insert_or_die(map, XMLTAG_DIRSCANRESULT, m_dsr);

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
//	set_map_class_info(this, &map);

#define X(field_tag, tag_string, var_name) AMLM::map_read_field_or_warn(map, field_tag, var_name);
	M_DATASTREAM_FIELDS(X);
#undef X

	AMLM::map_read_field_or_warn(map, XMLTAG_DIRSCANRESULT, &m_dsr);

	QVariantHomogenousList child_list = map.value(XMLTAG_CHILD_NODE_LIST).value<QVariantHomogenousList>();
#if 0
	auto model_ptr = getTypedModel();
	for(const QVariant& child : child_list)
	{
		qDb() << "READING CHILD ITEM INTO ScanResultsTreeModelItem:" << child.typeName();

//		int id = qMetaTypeId(child.type());
//		qDb() << "...QMetaType:" << id << QMetaType::typeName(id);

		std::shared_ptr<AbstractTreeModelItem> new_child_item = model_ptr->make_item_from_variant(child);
//		bool ok = appendChild(new_child_item);
//		Q_ASSERT(ok);
//		model_ptr->requestAddTreeModelItem(child, getId());
	}
#endif
/////////////
	auto model_ptr = m_model.lock();
	Q_ASSERT(model_ptr);
	auto parent_id = getId();
	Q_ASSERT(isInModel());

	/// NEEDS TO BE IN MODEL HERE.

	std::vector<std::shared_ptr<AbstractTreeModelItem>> new_child_item_vec;
	for(const QVariant& child : child_list)
	{
		qDb() << "READING CHILD ITEM INTO ScanResultsTreeModelItem:" << child.typeName();

//		std::shared_ptr<AbstractTreeModelItem> new_child_item = model_ptr->make_item_from_variant(child);
//		new_child_item_vec.push_back(new_child_item);
//		bool ok = appendChild(new_child_item);
//		Q_ASSERT(ok);
		model_ptr->requestAddTreeModelItem(child, parent_id);
//		std::dynamic_pointer_cast<ScanResultsTreeModel>(model_ptr)->requestAddExistingTreeModelItem(new_child_item, parent_id);
	}
	/// @todo WHY DOES THIS ASSERT
//	appendChildren(new_child_item_vec);
}

//std::shared_ptr<ScanResultsTreeModel> ScanResultsTreeModelItem::getTypedModel() const
//{
//	return std::dynamic_pointer_cast<ScanResultsTreeModel>(m_model.lock());
//}

void ScanResultsTreeModelItem::setDirscanResults(const DirScanResult& dsr)
{
	m_dsr = dsr;
}


