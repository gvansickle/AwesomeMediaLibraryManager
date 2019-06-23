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


// static
std::shared_ptr<AbstractTreeModelHeaderItem>
AbstractTreeModelHeaderItem::construct(const QVector<QVariant>& data, const std::shared_ptr<AbstractTreeModel>& model, bool isRoot, UUIncD id)
{
	std::shared_ptr<AbstractTreeModelHeaderItem> self(new AbstractTreeModelHeaderItem(data, model, isRoot, id));

	baseFinishConstruct(self);

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
	std::copy(column_specs.begin(), column_specs.end(), std::back_inserter(m_column_specs));
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
		return m_column_specs.at(column);
	}
	return QVariant();
}

int AbstractTreeModelHeaderItem::columnCount() const
{
	return m_column_specs.size();
}

#define M_DATASTREAM_FIELDS(X) \
	X(XMLTAG_HEADER_NUM_SECTIONS, header_num_sections)

using strviw_type = QLatin1Literal;

/// Strings to use for the tags.
#define X(field_tag, member_field) static const strviw_type field_tag ( # member_field );
	M_DATASTREAM_FIELDS(X);
#undef X


QVariant AbstractTreeModelHeaderItem::toVariant() const
{
	QVariantInsertionOrderedMap map;
	QVariantHomogenousList header_section_list("header_section_list", "section");

	// Header info.
	/// @todo Or is some of this really model info?  Children are.
	map.insert(XMLTAG_HEADER_NUM_SECTIONS, columnCount());
	for(int i = 0; i < columnCount(); ++i)
	{
		header_section_list.push_back(data(i));
	}
	map.insert("header_section_list", header_section_list);

	qDb() << M_NAME_VAL(childCount());
	map.insert("num_child_items", childCount());

	// Create a list of our children.
	QVariantHomogenousList child_list("child_list", "child");
	child_list.clear();
	for(int i = 0; i < childCount(); ++i)
	{
		const std::shared_ptr<AbstractTreeModelItem> child = this->child(i);
		child_list.push_back(child->toVariant());
//		list_push_back_or_warn(child_list, "child", child);
	}

	// Add list of child tree items to our QVariantMap.
	map_insert_or_die(map, "child_node_list", child_list);

	return map;
}

void AbstractTreeModelHeaderItem::fromVariant(const QVariant &variant)
{
	QVariantInsertionOrderedMap map;
	qviomap_from_qvar_or_die(&map, variant);

	QVariantHomogenousList header_section_list("header_section_list", "section");
	header_section_list = map.value("header_section_list").value<QVariantHomogenousList>();

	// Read the number of header sections...
	auto header_num_sections = map.value(XMLTAG_HEADER_NUM_SECTIONS).toInt();
	// ... and insert that many default-constructed columns to this HeaderItem.
	// Note that the AbstractTreeModel forwards it's insertColumns() call to here, but it handles the begin/end signaling.
	// So... I think we need to go through that mechanism if we're already in a model.
	// But... we're being deserialized here, so will we have a model yet?
M_WARNING("NEED TO GO THROUGH MODEL HERE?");
	insertColumns(0, header_num_sections);

	qDb() << "READING HEADER SECTION LIST," << header_num_sections << "COLUMNS:"  << header_section_list;

	int section_index = 0;
	for(const QVariant& e : header_section_list)
	{
		setData(section_index, e);
		section_index++;
	}

	/// @todo This is a QVariantList containing <item>/QVariantMap's, each of which
	/// contains a single <scan_res_tree_model_item type="QVariantMap">, which in turn
	/// contains a single <dirscanresult>/QVariantMap.
	QVariantHomogenousList child_list = map.value("child_node_list").value<QVariantHomogenousList>();


	// We'll break this into two phases:
	/// @note Maybe not.  This node isn't in a model here yet.
	/// Unless we can't parent the child here and it has to be appended through the model, but I think we can.
	// 1. Deserialize the list into new, unparented ScanResultsTreeModelItems.
	// 2. Add them as children of this model.
	// This buys us a few things:
	// 1. We could possibly do step 1 in a non-GUI thread.
	// 2. We can add the children in a single batch vs. one at a time, avoiding the model/view signaling overhead.
	// It does however burn more RAM.
	std::vector<std::shared_ptr<AbstractTreeModelItem>> temp_items;
	for(const QVariant& child : child_list)
	{
		qDb() << "READING CHILD ITEM:" << child;
//		auto child_item = this->create_default_constructed_child_item(this, columnCount());
		auto child_item = this->appendChild();
		child_item->fromVariant(child);
		// Save it off temporarily.
		temp_items.push_back(std::move(child_item));
	}

	// Append the children we read in to our list all in one batch.
	this->appendChildren(std::move(temp_items));
}

bool AbstractTreeModelHeaderItem::derivedClassSetData(int column, const QVariant& value)
{
	// We're the header, we should never have the Abstract Model's setData() called on us,
	// but this is the AbstractTreeModel*Item*'s setData(), and we're calling it in at least fromVariant() above.

	/// @todo Take ColumnSpecs instead.
	m_column_specs.at(column) = value.toString();

	return false;
}

bool AbstractTreeModelHeaderItem::derivedClassInsertColumns(int insert_before_column, int num_columns)
{
	// vector.insert(pos, size, ...):
	// - pos has the same definition as we're exposing here, it's the insert-before point.  Can be the end() iterator.
	/// @todo Again, convert to default constructed ColumnSpecs.
	m_column_specs.insert(m_column_specs.cbegin() + insert_before_column, num_columns, QString());

	return true;
}

bool AbstractTreeModelHeaderItem::derivedClassRemoveColumns(int first_column_to_remove, int num_columns)
{
	m_column_specs.erase(m_column_specs.cbegin() + first_column_to_remove,
			m_column_specs.cbegin() + first_column_to_remove + num_columns);

	return true;
}

