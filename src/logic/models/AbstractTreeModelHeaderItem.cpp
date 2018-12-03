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
 * @file AbstractTreeModelHeaderItem.cpp
 */

#include <logic/serialization/XmlObjects.h>
#include "AbstractTreeModelHeaderItem.h"

// Ours


AbstractTreeModelHeaderItem::AbstractTreeModelHeaderItem(AbstractTreeModelItem* parentItem)
	: AbstractTreeModelItem(parentItem)
{
	m_parent_model = nullptr;
}


AbstractTreeModelHeaderItem::AbstractTreeModelHeaderItem(AbstractTreeModel *parent_model,
														 AbstractTreeModelItem *parentItem)
	: AbstractTreeModelItem(parentItem)
{

	// Save the pointer to the parent_model.
	m_parent_model = parent_model;
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

QVariant AbstractTreeModelHeaderItem::data(int column) const
{
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

QVariant AbstractTreeModelHeaderItem::toVariant() const
{
	QVariantMap map;
	QVariantList list;

	// Header info.
	/// @todo Or is some of this really model info?  Children are.
	map.insert("header_num_sections", columnCount());
	for(int i = 0; i < columnCount(); ++i)
	{
		list.push_back(data(i));
	}
	map.insert("header_section_list", list);

	qDb() << M_NAME_VAL(childCount());
	map.insert("num_child_items", childCount());

	// Create a QVariantList of our children.
	list.clear();
	for(int i = 0; i < childCount(); ++i)
	{
		const AbstractTreeModelItem* child = this->child(i);
		list.push_back(child->toVariant());
	}

	// Add list of child tree items to our QVariantMap.
	map.insert("child_node_list", list);

	return map;
}

void AbstractTreeModelHeaderItem::fromVariant(const QVariant &variant)
{
	QVariantMap map = variant.toMap();

	QVariantList header_section_list = map.value("header_section_list").toList();

	// Read the number of header sections...
	auto header_num_sections = map.value("header_num_sections").toInt();
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
	QVariantList child_list = map.value("child_node_list").toList();


	// We'll break this into two phases:
	/// @note Maybe not.  This node isn't in a model here yet.
	/// Unless we can't parent the child here and it has to be appended through the model, but I think we can.
	// 1. Deserialize the list into new, unparented ScanResultsTreeModelItems.
	// 2. Add them as children of this model.
	// This buys us a few things:
	// 1. We could possibly do step 1 in a non-GUI thread.
	// 2. We can add the children in a single batch vs. one at a time, avoiding the model/view signaling overhead.
	// It does however burn more RAM.
	std::vector<AbstractTreeModelItem*> temp_items;
	for(const QVariant& child : child_list)
	{
		qDb() << "READING CHILD ITEM:" << child;
		ScanResultsTreeModelItem* child_item = this->create_default_constructed_child_item(this);
		child_item->fromVariant(child);
		// Save it off temporarily.
		temp_items.push_back(child_item);
	}

	// Append the children we read in to our list all in one batch.
	this->appendChildren(temp_items);
}

ScanResultsTreeModelItem*
AbstractTreeModelHeaderItem::create_default_constructed_child_item(AbstractTreeModelItem *parent)
{
	ScanResultsTreeModelItem* child_item;

	child_item = new ScanResultsTreeModelItem(parent);

	return child_item;
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

