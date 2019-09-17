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
 * @file AbstractTreeModelHeaderItem.cpp
 * Implementation of AbstractTreeModelHeaderItem.
 *
 * This class is heavily adapted from at least the following:
 * - The "Editable Tree Model Example" shipped with Qt5.
 * - KDenLive's TreeItem class.
 * - My own original work.
 * - Hundreds of nuggets of information from all over the Internet.
 */

#include "AbstractTreeModelHeaderItem.h"

// Std C++
#include <memory>

// Ours
#include <logic/serialization/XmlObjects.h>
#include "AbstractTreeModel.h"
#include "AbstractHeaderSection.h"
#include <serialization/QVariantHomogenousList.h>
#include <logic/serialization/SerializationHelpers.h>

/// TEMP
#include "ScanResultsTreeModel.h"

AbstractTreeModelHeaderItem::AbstractTreeModelHeaderItem(std::vector<ColumnSpec> column_specs,
                                                         const std::shared_ptr<AbstractTreeModel>& parent_model, UUIncD id)
	: BASE_CLASS({}, nullptr, id)//, m_is_root(true) //, m_column_specs(column_specs)
{
	m_is_root = true;
	m_model = parent_model;
	setColumnSpecs(column_specs);
}

AbstractTreeModelHeaderItem::~AbstractTreeModelHeaderItem()
{
}

void AbstractTreeModelHeaderItem::clear()
{
#if 0
	// Reset this header item to completely empty, except for its place in the model.
	// Note that we can't defer to the base class here because it should be cleaning up its parent, which we don't have.
	// All children should have already been removed from the model by the model.
	AMLM_ASSERT_X(m_child_items.size() == 0, "clear() called with unremoved children");
#else
	for(const auto& child : m_child_items)
	{
//		qDb() << "Removing child:";// << *child << "from" << this;
		this->removeChild(child);
	}
#endif
	m_child_items.clear();
	m_item_data.clear();
}

bool AbstractTreeModelHeaderItem::setColumnSpecs(std::initializer_list<ColumnSpec> column_specs)
{
	M_WARNING("TODO: NEED TO INSERT COLUMNS?");
	Q_ASSERT_X(childCount() == 0, __PRETTY_FUNCTION__, "Model has children already");
#warning "INSERT COLUMNS?"

	std::vector<ColumnSpec> temp_item_data;

	for(auto& it : column_specs)
	{
		temp_item_data.push_back(it);
	}

	this->setColumnSpecs(temp_item_data);
	return true;
}

bool AbstractTreeModelHeaderItem::setColumnSpecs(std::vector<ColumnSpec> column_specs)
{
	m_item_data.clear();

	for(auto& it : column_specs)
	{
		m_item_data.push_back(it.m_display_name);
	}
	return true;
}

QVariant AbstractTreeModelHeaderItem::data(int column, int role) const
{
	if((role != Qt::ItemDataRole::DisplayRole) && (role != Qt::ItemDataRole::EditRole))
	{
		return QVariant();
	}

	if(column < columnCount())
	{
		return m_item_data.at(column);
	}
	return QVariant();
}

#define M_DATASTREAM_FIELDS(X) \
	X(XMLTAG_HEADER_NUM_SECTIONS, header_num_sections, nullptr) \
	X(XMLTAG_CHILD_ITEM_MAP, child_item_map, nullptr)

#define M_DATASTREAM_FIELDS_CONTSIZES(X) \
	/*X(XMLTAG_NUM_COLUMNS, num_columns, m_item_data)*/ \
	X(XMLTAG_ITEM_DATA_SIZE, item_data_size, m_item_data) \
	X(XMLTAG_NUM_CHILDREN, num_children, m_child_items)

using strviw_type = QLatin1Literal;

/// Strings to use for the tags.
#define X(field_tag, member_field, var_name) static const strviw_type field_tag ( # member_field );
	M_DATASTREAM_FIELDS(X);
	M_DATASTREAM_FIELDS_CONTSIZES(X);
#undef X
static const strviw_type XMLTAG_HEADER_SECTION_LIST ("header_section_list");


QVariant AbstractTreeModelHeaderItem::toVariant() const
{
	InsertionOrderedStrVarMap map;

	// Set some class meta-info.
	set_map_class_info(this, &map);

#define X(field_tag, tag_string, var_name) map_insert_or_die(map, field_tag, var_name);
	M_DATASTREAM_FIELDS(X);
#undef X
#define X(field_tag, tag_string, var_name) map_insert_or_die(map, field_tag, (qulonglong)(var_name).size());
	M_DATASTREAM_FIELDS_CONTSIZES(X);
#undef X

	QVariantHomogenousList header_section_list(XMLTAG_HEADER_SECTION_LIST, "section");

	// Header info.
	map_insert_or_die(map, XMLTAG_HEADER_NUM_SECTIONS, columnCount());
	for(int i = 0; i < columnCount(); ++i)
	{
		QVariant section = data(i);
		/// @todo Hopefully temp validity checking and replacement here.
		if(!section.isValid())
		{
			section = QString("[empty]");
		}
		header_section_list.push_back(section);
	}
	map_insert_or_die(map, XMLTAG_HEADER_SECTION_LIST, header_section_list);

#warning "Looks like we're not in a ScanResultsTreeModel here, but an AbstractTreeModel."
	Q_ASSERT(isInModel());

	// Serialize out Child nodes.
	children_to_variant(&map);

	return map;
}

void AbstractTreeModelHeaderItem::fromVariant(const QVariant &variant)
{
	InsertionOrderedStrVarMap map;
	qviomap_from_qvar_or_die(&map, variant);

	// Read the number of header sections...
	int header_num_sections = 0;
	map_read_field_or_warn(map, XMLTAG_HEADER_NUM_SECTIONS, &header_num_sections);

	// Read the header sections.
	QVariantHomogenousList header_section_list(XMLTAG_HEADER_SECTION_LIST, "section");
	header_section_list = map.value(XMLTAG_HEADER_SECTION_LIST).value<QVariantHomogenousList>();

	AMLM_ASSERT_EQ(header_num_sections, header_section_list.size());

	// ... and insert that many default-constructed columns to this HeaderItem.
	// Note that the AbstractTreeModel forwards it's insertColumns() call to here, but it handles the begin/end signaling.
	// So... I think we need to go through that mechanism if we're already in a model.
	// But... we're being deserialized here, so will we have a model yet?
	Q_ASSERT(isInModel());
	Q_ASSERT(!m_model.expired());


	int section_index = 0;
	for(const QVariant& e : header_section_list)
	{
//		setData(section_index, e);
		m_item_data.push_back(e);
		section_index++;
	}

	// Now read in our children.  We need this HeaderItem to be in a model for that to work.
	Q_ASSERT(isInModel());

	// Currently, this needs to be in a model before we can add any child nodes.
	// By default, this HeaderItem *only* will already be in the model.
	auto model_ptr = std::dynamic_pointer_cast<AbstractTreeModel>(m_model.lock());
	Q_ASSERT(model_ptr);

	auto parent_id = getId();
	Q_ASSERT(parent_id != UUIncD::null());

	qDb() << "START: HeaderItem: Trying to read in child items";
	children_from_str_var_map(map);
	qDb() << "END: HeaderItem: Trying to read in child items";

}





