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
 * @file AbstractTreeModelItem.cpp
 * Implementation of AbstractTreeModelItem.
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
#include <logic/serialization/SerializationHelpers.h>

/// TEMP
#include "ScanResultsTreeModel.h"

// static
std::shared_ptr<AbstractTreeModelHeaderItem>
AbstractTreeModelHeaderItem::construct(const QVector<QVariant>& data, const std::shared_ptr<AbstractTreeModel>& model, bool isRoot, UUIncD id)
{
	std::shared_ptr<AbstractTreeModelHeaderItem> self(new AbstractTreeModelHeaderItem(data, model, isRoot, id));

	baseFinishConstruct(self);
	Q_ASSERT(self->isInModel());
	return self;
}

AbstractTreeModelHeaderItem::AbstractTreeModelHeaderItem(const QVector<QVariant>& data, const std::shared_ptr<AbstractTreeModel>& parent_model, bool isRoot, UUIncD id)
	: BASE_CLASS(data, parent_model, isRoot, id)
{

}

AbstractTreeModelHeaderItem::~AbstractTreeModelHeaderItem()
{
}

bool AbstractTreeModelHeaderItem::setColumnSpecs(std::initializer_list<QString> column_specs)
{
	M_WARNING("TODO This should take a list of ColumnSpecs, NEEDS TO INSERT COLUMNS");
	Q_ASSERT_X(childCount() == 0, __PRETTY_FUNCTION__, "Model has children already");
#warning "INSERT COLUMNS"
	std::copy(column_specs.begin(), column_specs.end(), std::back_inserter(m_item_data));
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

using strviw_type = QLatin1Literal;

/// Strings to use for the tags.
#define X(field_tag, member_field) static const strviw_type field_tag ( # member_field );
	M_DATASTREAM_FIELDS(X);
#undef X


QVariant AbstractTreeModelHeaderItem::toVariant() const
{
	QVariantInsertionOrderedMap map;

	// Overwrite any class info added by the above.
	set_map_class_info(this, &map);

	QVariantHomogenousList header_section_list("header_section_list", "section");

	// Header info.
	/// @todo Or is some of this really model info?  Children are.
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
	map_insert_or_die(map, "header_section_list", header_section_list);

	QVariantHomogenousList child_var_list(XMLTAG_CHILD_NODE_LIST, "child");
	for(auto& it : m_child_items)
	{
		list_push_back_or_die(child_var_list, it->toVariant());
	}
	map_insert_or_die(map, XMLTAG_CHILD_NODE_LIST, child_var_list);

	return map;
}

void AbstractTreeModelHeaderItem::fromVariant(const QVariant &variant)
{
	QVariantInsertionOrderedMap map;
	qviomap_from_qvar_or_die(&map, variant);

	// Read the number of header sections...
	int header_num_sections = 0;
	map_read_field_or_warn(map, XMLTAG_HEADER_NUM_SECTIONS, &header_num_sections);
	QVariantHomogenousList header_section_list("header_section_list", "section");
	header_section_list = map.value("header_section_list").value<QVariantHomogenousList>();

	AMLM_ASSERT_EQ(header_num_sections, header_section_list.size());

	// ... and insert that many default-constructed columns to this HeaderItem.
	// Note that the AbstractTreeModel forwards it's insertColumns() call to here, but it handles the begin/end signaling.
	// So... I think we need to go through that mechanism if we're already in a model.
	// But... we're being deserialized here, so will we have a model yet?
	Q_ASSERT(isInModel());
	Q_ASSERT(!m_model.expired());

//	insertColumns(0, header_num_sections);

	int section_index = 0;
	for(const QVariant& e : header_section_list)
	{
//		setData(section_index, e);
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

	/// @todo This is a QVariantList containing <item>/QVariantMap's, each of which
	/// contains a single <scan_res_tree_model_item type="QVariantMap">, which in turn
	/// contains a single <dirscanresult>/QVariantMap.
	QVariantHomogenousList child_list = map.value(XMLTAG_CHILD_NODE_LIST).value<QVariantHomogenousList>();
	Q_ASSERT(child_list.size() > 0);

//	std::vector<std::shared_ptr<AbstractTreeModelItem>> temp_items;
//	childrenFromVariant(child_list);
	for(const QVariant& child_variant : child_list)
	{
		qDb() << "READING CHILD ITEM INTO HEADERITEM:" << child_variant;

//		std::shared_ptr<AbstractTreeModelItem> new_child_item = model_ptr->make_item_from_variant(child);
//		bool ok = appendChild(new_child_item);
//		Q_ASSERT(ok);
		auto id = model_ptr->requestAddScanResultsTreeModelItem(child_variant, parent_id);
		Q_ASSERT(id != UUIncD::null());
		auto new_child = model_ptr->getItemById(id);
		Q_ASSERT(new_child);
//		new_child->fromVariant(variant);
	}
}

std::shared_ptr<AbstractHeaderSection> AbstractTreeModelHeaderItem::getHeaderSection(int column)
{
	// This is just a type conversion from the base class's vector<QVariant>.
	QVariant var = m_item_data.at(column);
	Q_ASSERT(var.canConvert<std::shared_ptr<AbstractHeaderSection>>());
	auto retval = var.value<std::shared_ptr<AbstractHeaderSection>>();

	return retval;
}



