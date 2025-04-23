/*
 * Copyright 2018, 2019, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/// @todo TEMP
#include "ScanResultsTreeModel.h"


// static
std::shared_ptr<AbstractTreeModelHeaderItem>
AbstractTreeModelHeaderItem::create(std::initializer_list<ColumnSpec> column_specs,
									   const std::shared_ptr<AbstractTreeModel>& parent_model)
{
    std::shared_ptr<AbstractTreeModelHeaderItem> new_item(new AbstractTreeModelHeaderItem(column_specs, parent_model));

    new_item->setColumnSpecs(column_specs);
    new_item->m_is_root = true;
    baseFinishCreate(new_item);

    return new_item;
}

AbstractTreeModelHeaderItem::AbstractTreeModelHeaderItem() : BASE_CLASS({}, nullptr)
{
    // Abs*HeaderItem can only be root.
    m_is_root = true;
}

AbstractTreeModelHeaderItem::AbstractTreeModelHeaderItem(std::initializer_list<ColumnSpec> column_specs,
                                                         const std::shared_ptr<AbstractTreeModel>& parent_model)
    : BASE_CLASS({}, parent_model)
{
	m_is_root = true;
	m_model = parent_model;
	setColumnSpecs(column_specs);
}

AbstractTreeModelHeaderItem::~AbstractTreeModelHeaderItem() = default;

void AbstractTreeModelHeaderItem::clear()
{
	// Reset this header item to completely empty, except for its place in the model (==root).
	// All children should have already been removed from the model by the model.
	Q_ASSERT(m_child_items.empty());

	BASE_CLASS::clear();
	m_item_data.clear();
}

bool AbstractTreeModelHeaderItem::setColumnSpecs(std::vector<ColumnSpec> column_specs)
{
// M_WARNING("TODO: NEEDS TO INSERT COLUMNS")
	Q_ASSERT_X(childCount() == 0, __PRETTY_FUNCTION__, "Model has children already");

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
	X(XMLTAG_HEADER_NUM_SECTIONS, header_num_sections) \
	X(XMLTAG_CHILD_NODE_LIST, child_node_list)

using strviw_type = QLatin1String;

/// Strings to use for the tags.
#define X(field_tag, member_field) static constexpr strviw_type field_tag ( # member_field );
	M_DATASTREAM_FIELDS(X);
#undef X
static constexpr strviw_type XMLTAG_HEADER_SECTION_LIST ("header_section_list");


QVariant AbstractTreeModelHeaderItem::toVariant() const
{
	InsertionOrderedMap<QString, QVariant> map;

	// Set some class meta-info.
	set_map_class_info(this, &map);

    // Serialize the data members of the base class.
    QVariant base_class = static_cast<const BASE_CLASS*>(this)->BASE_CLASS::toVariant();

    map_insert_or_die(map, "baseclass", base_class);

	return map;
}

void AbstractTreeModelHeaderItem::fromVariant(const QVariant &variant)
{
	InsertionOrderedMap<QString, QVariant> map;
	qviomap_from_qvar_or_die(&map, variant);

    // This is always a hidden root item.
    m_is_root = true;
    // m_uuincid = UUIncD::create(); // Note: This is done in the base class.

	// Now read in our children.  We need this HeaderItem to be in a model for that to work.
	Q_ASSERT(isRoot() && isInModel());

	// This needs to be in a model before we can requestAddXxx() anything.
	// By default, this HeaderItem *only* will already be in the model.
    auto model_ptr = std::dynamic_pointer_cast<ScanResultsTreeModel>(m_model.lock());
    Q_ASSERT(model_ptr);

	// auto parent_id = getId();
    // Q_ASSERT(parent_id != UUIncD::null());

    // Deserialize the data members of the base class.
    // This includes child items.
    auto iomap {InsertionOrderedMap<QString, QVariant>()};
    map_read_field_or_warn(map, "baseclass", &iomap);
    this->BASE_CLASS::fromVariant(iomap);
}

std::shared_ptr<AbstractHeaderSection> AbstractTreeModelHeaderItem::getHeaderSection(int column)
{
	// This is just a type conversion from the base class's vector<QVariant>.
	QVariant var = m_item_data.at(column);
	Q_ASSERT(var.canConvert<std::shared_ptr<AbstractHeaderSection>>());
	auto retval = var.value<std::shared_ptr<AbstractHeaderSection>>();

	return retval;
}



