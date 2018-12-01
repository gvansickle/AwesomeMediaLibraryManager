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
 * @file TreeModelRootItem.cpp
 */

#include "TreeModelRootItem.h"

// Ours.
#include "AbstractTreeModel.h"
#include "AbstractTreeModelItem.h"
#include "AbstractTreeModelHeaderItem.h"


TreeModelRootItem::TreeModelRootItem(AbstractTreeModel* parent_model,
									 AbstractTreeModelItem* parent_item)
{

}

TreeModelRootItem::~TreeModelRootItem()
{
}

//QVariant TreeModelRootItem::data(int column) const
//{
//	Q_ASSERT_X(0, __PRETTY_FUNCTION__, "I don't think we should be getting here");
//	return QVariant();
//}

//int TreeModelRootItem::columnCount() const
//{
////	Q_CHECK_PTR(m_horizontal_header_item);
////	return m_horizontal_header_item->columnCount();
//}

//QVariant TreeModelRootItem::toVariant() const
//{
//	QVariantMap map;
//
//	// The horizontal header item.
//	map.insert(/*SRTMTagToXMLTagMap[SRTMTag::ROOT_ITEM]*/ "header_horz", m_horizontal_header_item->toVariant());
//
//	return map;
//}
//
//void TreeModelRootItem::fromVariant(const QVariant& variant)
//{
//	QVariantMap map = variant.toMap();
//
//	/// @note This is a QVariantMap, contains abstract_tree_model_header as a QVariantList.
//	m_horizontal_header_item = new AbstractTreeModelHeaderItem();
//	m_horizontal_header_item->fromVariant(map.value(/*SRTMTagToXMLTagMap[SRTMTag::ROOT_ITEM]*/ "header_horz"));
//}

//ScanResultsTreeModelItem* TreeModelRootItem::create_default_constructed_child_item(AbstractTreeModelItem* parent)
//{
//	/// @todo Should they be parented out the gate like this?
//	return new ScanResultsTreeModelItem(parent);
//}

//bool TreeModelRootItem::derivedClassSetData(int column, const QVariant& value)
//{
//	M_WARNING("TODO");
//	return true;
//}
//
//bool TreeModelRootItem::derivedClassInsertColumns(int insert_before_column, int num_columns)
//{
//	M_WARNING("TODO");
//	return true;
//}
//
//bool TreeModelRootItem::derivedClassRemoveColumns(int first_column_to_remove, int num_columns)
//{
//	M_WARNING("TODO");
//	return true;
//}

