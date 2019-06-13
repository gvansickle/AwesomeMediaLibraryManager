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
#include <unordered_set>
#include <queue>
#include "ScanResultsTreeModel.h"


std::shared_ptr<AbstractTreeModel> AbstractTreeModel::construct(QObject* parent)
{
	std::shared_ptr<AbstractTreeModel> self(new AbstractTreeModel(parent));
	self->m_root_item = AbstractTreeModelHeaderItem::construct(self, true);
	return self;
}

AbstractTreeModel::AbstractTreeModel(QObject* parent) : QAbstractItemModel(parent)
{
}

AbstractTreeModel::~AbstractTreeModel()
{
	m_model_item_map.clear();
	m_root_item.reset();
}

bool AbstractTreeModel::setColumnSpecs(std::initializer_list<QString> column_specs)
{
	return m_root_item->setColumnSpecs(column_specs);
}

int AbstractTreeModel::columnCount(const QModelIndex& parent) const
{
#if 0
	Q_ASSERT(m_root_item != nullptr);
	return m_root_item->columnCount();
#else /// kden
	if(!parent.isValid())
	{
		/// Does this make sense?
		return m_root_item->columnCount();
	}
	const auto id = UUIncD(parent.internalId());
	auto item = getItemById(id);
	return item->columnCount();
#endif
}

QVariant AbstractTreeModel::data(const QModelIndex &index, int role) const
{
	Q_ASSERT(checkIndex(index, CheckIndexOption::IndexIsValid));

	if (!index.isValid())
	{
		// Should never get here, checkIndex() should have asserted above.
        return QVariant();
	}

	/// @todo ###
#if 0 // TEMP?
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
#endif
    if (role != Qt::DisplayRole) /// @todo KDen: && role != Qt::EditRole)
	{
        return QVariant();
	}

    // Get a pointer to the indexed item.
	auto item = getItemById(UUIncD(index.internalId()));

    return item->data(index.column(), role);
}

Qt::ItemFlags AbstractTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
	{
		return Qt::NoItemFlags;
	}

#if 0 /// KDen does this here, not sure we need it.
	if (index.isValid()) {
        auto item = getItemById((int)index.internalId());
        if (item->depth() == 1) {
            return flags & ~Qt::ItemIsSelectable;
        }
    }
#endif

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

/// NEW: KDEN:
std::shared_ptr<AbstractTreeModelItem> AbstractTreeModel::getItemById(const UUIncD& id) const
{
	if(id == m_root_item->getId())
	{
		return m_root_item;
	}
	return m_model_item_map.at(id).lock();
}

/// BOTH
std::shared_ptr<AbstractTreeModelItem> AbstractTreeModel::getRootItem() const
{
	return m_root_item;
}

void AbstractTreeModel::register_item(const std::shared_ptr<AbstractTreeModelItem>& item)
{
	UUIncD id = item->getId();
	Q_ASSERT(m_model_item_map.count(id) == 0);
	m_model_item_map[id] = item;
}

void AbstractTreeModel::deregister_item(UUIncD id, AbstractTreeModelItem* item)
{
	Q_UNUSED(item);
	Q_ASSERT(m_model_item_map.count(id) > 0);
	m_model_item_map.erase(id);
}

void AbstractTreeModel::notifyRowAboutToAppend(const std::shared_ptr<AbstractTreeModelItem>& item)
{
	auto index = getIndexFromItem(item);
	beginInsertRows(index, item->childCount(), item->childCount());
}

void AbstractTreeModel::notifyRowAppended(const std::shared_ptr<AbstractTreeModelItem>& row)
{
	Q_UNUSED(row);
	endInsertRows();
}

void AbstractTreeModel::notifyRowAboutToDelete(std::shared_ptr<AbstractTreeModelItem> item, int row)
{
	auto index = getIndexFromItem(item);
	beginRemoveRows(index, row, row);
}

void AbstractTreeModel::notifyRowDeleted()
{
	endRemoveRows();
}

/// NEW: KDEN:
bool AbstractTreeModel::checkConsistency()
{
// first check that the root is all good
	if (!m_root_item || !m_root_item->m_is_root || !m_root_item->isInModel() || m_model_item_map.count(m_root_item->getId()) == 0) {
		qDebug() << !m_root_item->m_is_root << !m_root_item->isInModel() << (m_model_item_map.count(m_root_item->getId()) == 0);
		qDebug() << "ERROR: Model is not valid because root is not properly constructed";
		return false;
	}
	// Then we traverse the tree from the root, checking the infos on the way
	std::unordered_set<UUIncD> seenIDs;
	std::queue<std::pair<UUIncD, std::pair<int, UUIncD>>> queue; // store (id, (depth, parentId))
	queue.push({m_root_item->getId(), {0, m_root_item->getId()}});
	while (!queue.empty())
	{
		auto current = queue.front();
		UUIncD currentId = current.first;
		int currentDepth = current.second.first;
		UUIncD parentId = current.second.second;
		queue.pop();
		if (seenIDs.count(currentId) != 0) {
			qDebug() << "ERROR: Invalid tree: Id found twice."
			         << "It either a cycle or a clash in id attribution";
			return false;
		}
		if (m_model_item_map.count(currentId) == 0) {
			qDebug() << "ERROR: Invalid tree: Id not found. Item is not registered";
			return false;
		}
		auto currentItem = m_model_item_map[currentId].lock();
		if (currentItem->depth() != currentDepth) {
			qDebug() << "ERROR: Invalid tree: invalid depth info found";
			return false;
		}
		if (!currentItem->isInModel()) {
			qDebug() << "ERROR: Invalid tree: item thinks it is not in a model";
			return false;
		}
		if (currentId != m_root_item->getId()) {
			if ((currentDepth == 0 || currentItem->m_is_root)) {
				qDebug() << "ERROR: Invalid tree: duplicate root";
				return false;
			}
			if (auto ptr = currentItem->parent().lock())
			{
				if (ptr->getId() != parentId || ptr->child(currentItem->childNumber())->getId() != currentItem->getId()) {
					qDebug() << "ERROR: Invalid tree: invalid parent link";
					return false;
				}
			} else {
				qDebug() << "ERROR: Invalid tree: invalid parent";
				return false;
			}
		}
		// propagate to children
		int i = 0;
		for (const auto &child : currentItem->m_child_items)
		{
			if (currentItem->child(i) != child) {
				qDebug() << "ERROR: Invalid tree: invalid child ordering";
				return false;
			}
			queue.push({child->getId(), {currentDepth + 1, currentId}});
			i++;
		}
	}
	return true;
}

QVariant AbstractTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
    	/// @kden : dataColumn()
		return m_root_item->data(section);
	}

    return QVariant();
}

QModelIndex AbstractTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	std::shared_ptr<AbstractTreeModelItem> parent_item;

	if(!parent.isValid())
	{
		parent_item = m_root_item;
	}
	else
	{
		parent_item = getItemById(UUIncD(parent.internalId()));
	}

	if (row >= parent_item->childCount())
	{
        return QModelIndex();
	}

	std::shared_ptr<AbstractTreeModelItem> childItem = parent_item->child(row);

	if(childItem)
	{
		return createIndex(row, column, quintptr(childItem->getId()));
	}
    else
	{
        return QModelIndex();
	}
}

//bool AbstractTreeModel::insertColumns(int insert_before_column, int num_columns, const QModelIndex& parent_model_index)
//{
//	Q_CHECK_PTR(m_root_item);
//
//	bool success;
//
//    beginInsertColumns(parent_model_index, insert_before_column, insert_before_column + num_columns - 1);
//	success = m_root_item->insertColumns(insert_before_column, num_columns);
//    endInsertColumns();
//
//    return success;
//}

//bool AbstractTreeModel::insertRows(int insert_before_row, int num_rows, const QModelIndex& parent_model_index)
//{
//	Q_CHECK_PTR(m_root_item);
//
//	std::shared_ptr<AbstractTreeModelItem> parent_item = getItemById(UUIncD(parent_model_index.internalId()));
//    bool success;
//
//    beginInsertRows(parent_model_index, insert_before_row, insert_before_row + num_rows - 1);
//	success = parent_item->insertChildren(insert_before_row, num_rows, m_root_item->columnCount());
//    endInsertRows();
//
//    return success;
//}

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

	std::shared_ptr<AbstractTreeModelItem> parentItem = getItemById(static_cast<UUIncD>(parent_item_index.internalId()));
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
	return this->BASE_CLASS::moveColumns(sourceParent, sourceColumn, count, destinationParent, destinationChild);
}


bool AbstractTreeModel::appendItems(std::vector<std::shared_ptr<AbstractTreeModelItem>> new_items, const QModelIndex &parent)
{
	std::shared_ptr<AbstractTreeModelItem> parent_item = getItemById(static_cast<UUIncD>(parent.internalId()));
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

bool AbstractTreeModel::appendItem(std::shared_ptr<AbstractTreeModelItem> new_item, const QModelIndex& parent)
{
	Q_ASSERT(0/*NOT IMPL*/);
	std::vector<std::shared_ptr<AbstractTreeModelItem>> new_items;

	new_items.emplace_back(std::move(new_item));
	return appendItems(std::move(new_items), parent);
}

QModelIndex AbstractTreeModel::getIndexFromItem(const std::shared_ptr<AbstractTreeModelItem>& item) const
{
	Q_CHECK_PTR(item);
	if(item == m_root_item)
	{
		return QModelIndex();
	}
	auto parent_index = getIndexFromItem(item->parent().lock());
	return index(item->childNumber(), 0, parent_index);
}

QModelIndex AbstractTreeModel::getIndexFromId(UUIncD id) const
{
	if(id == m_root_item->getId())
	{
		return QModelIndex();
	}
	Q_ASSERT(m_model_item_map.count(id) > 0);
	if(auto ptr = m_model_item_map.at(id).lock())
	{
		return getIndexFromItem(ptr);
	}
	Q_ASSERT(0);
	return QModelIndex();
}

int AbstractTreeModel::rowCount(const QModelIndex &parent) const
{
	// Only the hidden parent item has row count info.
	/// @todo Is this right?
	if(parent.column() > 0)
	{
		return 0;
	}
	std::shared_ptr<AbstractTreeModelItem> parent_item;
	if(!parent.isValid())
	{
		parent_item = m_root_item;
	}
	else
	{
		parent_item = getItemById(UUIncD(parent.internalId()));
	}

	return parent_item->childCount();
}

bool AbstractTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::EditRole)
	{
        return false;
	}

	std::shared_ptr<AbstractTreeModelItem> item = getItemById(UUIncD(index.internalId()));
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




