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

#if 1
// static
std::shared_ptr<AbstractTreeModelHeaderItem>
AbstractTreeModelHeaderItem::create(std::initializer_list<ColumnSpec> column_specs,
									   const std::shared_ptr<AbstractTreeModel>& parent_model, UUIncD id)
{
    std::shared_ptr<AbstractTreeModelHeaderItem> new_item(new AbstractTreeModelHeaderItem(column_specs, parent_model, UUIncD::create()));

    new_item->setColumnSpecs(column_specs);
    new_item->m_is_root = true;
    baseFinishCreate(new_item);
    return new_item;
}
#endif

AbstractTreeModelHeaderItem::AbstractTreeModelHeaderItem() : BASE_CLASS({}, nullptr)
{
    // Abs*HeaderItem can only be root.
    m_is_root = true;
}

AbstractTreeModelHeaderItem::AbstractTreeModelHeaderItem(std::initializer_list<ColumnSpec> column_specs,
                                                         const std::shared_ptr<AbstractTreeModel>& parent_model, UUIncD id)
    : BASE_CLASS({}, parent_model, id)//, m_is_root(true) //, m_column_specs(column_specs)
{
	m_is_root = true;
	m_model = parent_model;
	setColumnSpecs(column_specs);
}

AbstractTreeModelHeaderItem::~AbstractTreeModelHeaderItem() = default;

void AbstractTreeModelHeaderItem::clear()
{
	// Reset this header item to completely empty, except for its place in the model.
	// All children should have already been removed from the model by the model.
	Q_ASSERT(m_child_items.empty());

//	m_column_specs.clear();
//	BASE_CLASS::clear();
	m_item_data.clear();
//	m_num_columns = 0;
//	m_num_parent_columns = -1;
}

bool AbstractTreeModelHeaderItem::setColumnSpecs(std::initializer_list<ColumnSpec> column_specs)
{
    M_WARNING("TODO This should take a list of ColumnSpecs, NEEDS TO INSERT COLUMNS")
	Q_ASSERT_X(childCount() == 0, __PRETTY_FUNCTION__, "Model has children already");
//	m_column_specs.clear();
	m_item_data.clear();
//	std::copy(column_specs.begin(), column_specs.end(), std::back_inserter(m_column_specs));
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

    // Serialize the data members of the base class.
    QVariant base_class = static_cast<const BASE_CLASS*>(this)->BASE_CLASS::toVariant();

    map_insert_or_die(map, "baseclass", base_class);

	return map;
}

void AbstractTreeModelHeaderItem::fromVariant(const QVariant &variant)
{
	InsertionOrderedMap<QString, QVariant> map;
	qviomap_from_qvar_or_die(&map, variant);

	// Read the number of header sections...
	int header_num_sections = 0;
	map_read_field_or_warn(map, XMLTAG_HEADER_NUM_SECTIONS, &header_num_sections);

	// Read the header sections.
	QVariantHomogenousList header_section_list(XMLTAG_HEADER_SECTION_LIST, "section");
	header_section_list = map.at(XMLTAG_HEADER_SECTION_LIST).value<QVariantHomogenousList>();

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
        // setData(section_index, e);
        m_item_data.push_back(e);
		section_index++;
	}

	// Now read in our children.  We need this HeaderItem to be in a model for that to work.
    Q_ASSERT(isInModel());

	// This needs to be in a model before we can requestAddXxx() anything.
	// By default, this HeaderItem *only* will already be in the model.
    auto model_ptr = std::dynamic_pointer_cast<ScanResultsTreeModel>(m_model.lock());
    Q_ASSERT(model_ptr);

	auto parent_id = getId();
	Q_ASSERT(parent_id != UUIncD::null());

    // Deserialize the data members of the base class.
    auto iomap {InsertionOrderedMap<QString, QVariant>()};
    map_read_field_or_warn(map, "baseclass", &iomap);
    static_cast<BASE_CLASS*>(this)->BASE_CLASS::fromVariant(iomap);

#if 0
	/// @todo This is a QVariantList containing <item>/QVariantMap's, each of which
	/// contains a single <scan_res_tree_model_item type="QVariantMap">, which in turn
	/// contains a single <dirscanresult>/QVariantMap.
	QVariantHomogenousList child_var_list(XMLTAG_CHILD_NODE_LIST, "child");
	child_var_list = map.at(XMLTAG_CHILD_NODE_LIST).value<QVariantHomogenousList>();
	Q_ASSERT(!child_var_list.empty());
	qDb() << "Number of children read:" << child_var_list.size();

#if 0///
    // append_children_from_variant<decltype(m_model), std::shared_ptr<ScanResultsTreeModelItem>/*, AbstractTreeModelHeaderItem*/>(m_model, this, child_var_list);
	append_children_from_variant(m_model, this, child_var_list);
#else
	auto starting_childcount = childCount();

	for(const QVariant& child_variant : child_var_list)
	{
        qDb() << "READING CHILD ITEM INTO HEADERITEM:" << child_variant << "TYPE:" << child_variant.metaType().name();

        auto new_child = std::make_shared<ScanResultsTreeModelItem>(model_ptr, UUIncD::create());
		Q_ASSERT(new_child);
		/// @note Cuurently we need to add the empty item to the model before reading it in, so that
		/// its children will be set up correctly model-wise.  This is almost certainly more efficient anyway.
		this->appendChild(new_child);
		new_child->fromVariant(child_variant);

//		std::shared_ptr<AbstractTreeModelItem> new_child_item = model_ptr->make_item_from_variant(child);
//		bool ok = appendChild(new_child_item);
//		Q_ASSERT(ok);
//		auto id = model_ptr->requestAddScanResultsTreeModelItem(child_variant, parent_id);
//		Q_ASSERT(id != UUIncD::null());
//		auto new_child = model_ptr->getItemById(id);
//		Q_ASSERT(new_child);
//		new_child->fromVariant(variant);
	}

	AMLM_ASSERT_EQ(starting_childcount+child_var_list.size(),childCount());
#endif
#endif
}

std::shared_ptr<AbstractHeaderSection> AbstractTreeModelHeaderItem::getHeaderSection(int column)
{
	// This is just a type conversion from the base class's vector<QVariant>.
	QVariant var = m_item_data.at(column);
	Q_ASSERT(var.canConvert<std::shared_ptr<AbstractHeaderSection>>());
	auto retval = var.value<std::shared_ptr<AbstractHeaderSection>>();

	return retval;
}



