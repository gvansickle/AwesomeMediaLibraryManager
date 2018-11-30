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
#include "AbstractTreeModelHeaderItem.h"

// Ours
#include <logic/xml/XmlObjects.h>
#include "ScanResultsTreeModelItem.h"


AbstractTreeModelHeaderItem::AbstractTreeModelHeaderItem(QVector<QVariant> x, AbstractTreeModelItem *parentItem)
	: AbstractTreeModelItem(parentItem)
{
#warning "TODO This should take a list of AbsHeaderSections"
//	m_item_data = x;
	m_column_specs = x;
}

AbstractTreeModelHeaderItem::~AbstractTreeModelHeaderItem()
{
}

QVariant AbstractTreeModelHeaderItem::toVariant() const
{
	QVariantMap map;
	QVariantList list;

	// Header info.
	/// @todo Or is some of this really model info?
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

	auto header_num_sections = map.value("header_num_sections").toInt();
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
	QVector<AbstractTreeModelItem*> temp_items;
	for(const QVariant& child : child_list)
	{
		qDb() << "READING CHILD ITEM:" << child;
		ScanResultsTreeModelItem* child_item = this->create_default_constructed_child_item(this);
		child_item->fromVariant(child);
		// Save it off temporarily.
		temp_items.push_back(child_item);
	}

	// Append the children we read in to our list.
	this->appendChildren(temp_items);
}

ScanResultsTreeModelItem*
AbstractTreeModelHeaderItem::create_default_constructed_child_item(AbstractTreeModelItem *parent)
{
	ScanResultsTreeModelItem* child_item;

	child_item = new ScanResultsTreeModelItem(parent);

	return child_item;
}

