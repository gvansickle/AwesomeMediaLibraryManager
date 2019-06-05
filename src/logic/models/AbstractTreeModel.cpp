/**
 * Adapted from the "Editable Tree Model Example" shipped with Qt5.
 */

/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

// This class's header.
#include "AbstractTreeModel.h"

// Std C++
#include <functional>

// Qt5
#include <QtWidgets>

// KF5
#include <KItemViews/KCategorizedSortFilterProxyModel>

// Ours
#include "AbstractTreeModelItem.h"
#include "AbstractTreeModelHeaderItem.h"
#include <utils/DebugHelpers.h>
#include <logic/serialization/XmlSerializer.h>
#include "ScanResultsTreeModel.h"


AbstractTreeModel::AbstractTreeModel(QObject* parent) : QAbstractItemModel(parent)
{
	auto horizontal_header_item = new AbstractTreeModelHeaderItem();

	m_root_item = std::make_shared<AbstractTreeModelHeaderItem>(this, horizontal_header_item);
}

AbstractTreeModel::~AbstractTreeModel()
{
	m_root_item.reset();
}

bool AbstractTreeModel::setColumnSpecs(std::initializer_list<QString> column_specs)
{
	return m_root_item->setColumnSpecs(column_specs);
}

int AbstractTreeModel::columnCount(const QModelIndex & /* parent */) const
{
	Q_ASSERT(m_root_item != nullptr);
	return m_root_item->columnCount();
}

QVariant AbstractTreeModel::data(const QModelIndex &index, int role) const
{
	Q_ASSERT(checkIndex(index, CheckIndexOption::IndexIsValid));

	if (!index.isValid())
	{
		// Should never get here, checkIndex() should have asserted above.
        return QVariant();
	}

	// Color invalid model indexes.
	if(index.column() > columnCount())
	{
		switch(role)
		{
			case Qt::ItemDataRole::BackgroundRole:
				return QVariant::fromValue(QBrush(Qt::lightGray));
				break;
			default:
				break;
		}
	}

    if (role != Qt::DisplayRole && role != Qt::EditRole)
	{
        return QVariant();
	}

    // Get a pointer to the indexed item.
    AbstractTreeModelItem *item = getItem(index);

    return item->data(index.column());
}

Qt::ItemFlags AbstractTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
	{
		return Qt::NoItemFlags;
	}

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

//AbstractTreeModelItem* AbstractTreeModel::getItem(const QModelIndex &index) const
//{
//	if (index.isValid())
//	{
//        AbstractTreeModelItem *item = static_cast<AbstractTreeModelItem*>(index.internalPointer());
//		if (item != nullptr)
//		{
//            return item;
//		}
//    }
//	/// @todo This might want to be an assert() due to invalid index.
//	return m_root_item;
//}

std::shared_ptr<AbstractTreeModelItem> AbstractTreeModel::getItemById(const UUIncD& id) const
{
	if(id == m_root_item->getId())
	{
		return m_root_item;
	}
	return m_model_item_map.at(id).lock();
}

std::shared_ptr<AbstractTreeModelItem> AbstractTreeModel::getRootItem() const
{
	return m_root_item;
}

QVariant AbstractTreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		return m_root_item->data(section);
	}

    return QVariant();
}

QModelIndex AbstractTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
	{
        return QModelIndex();
	}

    AbstractTreeModelItem *parentItem = getItem(parent);

    AbstractTreeModelItem *childItem = parentItem->child(row);
	if(childItem != nullptr)
	{
        return createIndex(row, column, childItem);
	}
    else
	{
        return QModelIndex();
	}
}

bool AbstractTreeModel::insertColumns(int insert_before_column, int num_columns, const QModelIndex& parent_model_index)
{
	Q_CHECK_PTR(m_root_item);

	bool success;

    beginInsertColumns(parent_model_index, insert_before_column, insert_before_column + num_columns - 1);
	success = m_root_item->insertColumns(insert_before_column, num_columns);
    endInsertColumns();

    return success;
}

bool AbstractTreeModel::insertRows(int insert_before_row, int num_rows, const QModelIndex& parent_model_index)
{
	Q_CHECK_PTR(m_root_item);

    AbstractTreeModelItem *parentItem = getItem(parent_model_index);
    bool success;

    beginInsertRows(parent_model_index, insert_before_row, insert_before_row + num_rows - 1);
	success = parentItem->insertChildren(insert_before_row, num_rows, m_root_item->columnCount());
    endInsertRows();

    return success;
}

QModelIndex AbstractTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
	{
        return QModelIndex();
	}

	std::shared_ptr<AbstractTreeModelItem> childItem = getItemById(static_cast<UUIncD>(index.internalId()));
	std::shared_ptr<AbstractTreeModelItem> parentItem = childItem->parent().lock();

	Q_ASSERT(parentItem);

	if (parentItem == m_root_item)
	{
		return QModelIndex();
	}

	return createIndex(parentItem->childNumber(), 0, quintptr(parentItem->getId()));
}

bool AbstractTreeModel::removeColumns(int position, int columns, const QModelIndex& parent_model_index)
{
    bool success;

    beginRemoveColumns(parent_model_index, position, position + columns - 1);
	success = m_root_item->removeColumns(position, columns);
    endRemoveColumns();

	if (m_root_item->columnCount() == 0)
	{
        removeRows(0, rowCount());
	}

    return success;
}

bool AbstractTreeModel::removeRows(int remove_start_row, int num_rows, const QModelIndex& parent_item_index)
{
	if(num_rows == 0)
	{
		qWr() << "Attempt to remove zero children";
		return false;
	}

    AbstractTreeModelItem *parentItem = getItem(parent_item_index);
    bool success = true;

    beginRemoveRows(parent_item_index, remove_start_row, remove_start_row + num_rows - 1);
    success = parentItem->removeChildren(remove_start_row, num_rows);
    endRemoveRows();

	return success;
}

bool AbstractTreeModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild)
{
	// Defer to base class.
	return this->BASE_CLASS::moveRows(sourceParent, sourceRow, count, destinationParent, destinationChild);
}

bool AbstractTreeModel::moveColumns(const QModelIndex& sourceParent, int sourceColumn, int count, const QModelIndex& destinationParent, int destinationChild)
{
	// Defer to base class.
	return this->BASE_CLASS::moveRows(sourceParent, sourceColumn, count, destinationParent, destinationChild);
}


bool AbstractTreeModel::appendItems(std::vector<std::unique_ptr<AbstractTreeModelItem>> new_items, const QModelIndex &parent)
{
    auto parent_item = getItem(parent);
    Q_CHECK_PTR(parent_item);

    if(new_items.empty())
    {
    	qWr() << "Attempt to append zero items.";
    	return false;
    }

    auto first_new_row_num_after_insertion = parent_item->childCount();

    /// @todo What do we need to do to support/handle different num of columns?
	/// @todo These items have data already and aren't default-constructed, do we need to do anything different
	///       than begin/endInsert rows?
	// parent, first_row_num_after_insertion, last_row_num_after_insertion.
	this->beginInsertRows(parent, first_new_row_num_after_insertion, first_new_row_num_after_insertion + new_items.size() - 1);

    parent_item->appendChildren(std::move(new_items));

    this->endInsertRows();

    return true;
}

bool AbstractTreeModel::appendItem(std::unique_ptr<AbstractTreeModelItem> new_item, const QModelIndex& parent)
{
	std::vector<std::unique_ptr<AbstractTreeModelItem>> new_items;

	new_items.emplace_back(std::move(new_item));
	return appendItems(std::move(new_items), parent);
}

int AbstractTreeModel::rowCount(const QModelIndex &parent) const
{
    AbstractTreeModelItem *parentItem = getItem(parent);

    return parentItem->childCount();
}

bool AbstractTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::EditRole)
	{
        return false;
	}

    AbstractTreeModelItem *item = getItem(index);
    bool result = item->setData(index.column(), value);

	if(result)
	{
		Q_EMIT dataChanged(index, index);
	}

    return result;
}

bool AbstractTreeModel::setHeaderData(int section, Qt::Orientation orientation,
                              const QVariant &value, int role)
{
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
	{
        return false;
	}

	bool result = m_root_item->setData(section, value);

    if (result)
	{
    	// Docs: "If you are changing the number of columns or rows you do not need to emit this signal,
    	// but use the begin/end functions."
		Q_EMIT headerDataChanged(orientation, section, section);
	}

	return result;
}

bool AbstractTreeModel::setHeaderData(int section, const AbstractHeaderSection& header_section)
{
	Q_ASSERT(0);
//	this->setHeaderData(header_section.section(),
//			header_section.orientation(), header_section[role], role);
//	for()
	return true;
}




