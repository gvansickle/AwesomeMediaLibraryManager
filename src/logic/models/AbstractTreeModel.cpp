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
 * @file AbstractTreeModel.cpp
 * Implementation of AbstractTreeModel.
 *
 * This class is heavily adapted from at least the following:
 * - The "Editable Tree Model Example" shipped with Qt5.
 * - KDenLive's AbstractItemModel class.
 * - My own original work.
 * - Hundreds of nuggets of information from all over the Internet.
 */

// This class's header.
#include "AbstractTreeModel.h"

// Std C++
#include <functional>
#include <unordered_set>
#include <queue>

// Qt5
#include <QtWidgets>
#include <QAbstractItemModelTester>

// KF5
#include <KItemViews/KCategorizedSortFilterProxyModel>

// Ours
#include "AbstractTreeModelItem.h"
#include "AbstractTreeModelHeaderItem.h"
#include <utils/DebugHelpers.h>
#include <logic/serialization/XmlSerializer.h>
#include "ScanResultsTreeModel.h"

/**
 * This really should never get called, since AbstractTreeModel is abstract.  Mostly here for an example for derived classes.
 * @param parent
 * @return
 */
std::shared_ptr<AbstractTreeModel> AbstractTreeModel::construct(QObject* parent)
{
	std::shared_ptr<AbstractTreeModel> self(new AbstractTreeModel(parent));
	self->m_root_item = AbstractTreeModelHeaderItem::construct(self, true);
	self->m_model_tester = new QAbstractItemModelTester(self.get(), QAbstractItemModelTester::FailureReportingMode::Fatal, self.get());
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
		// Invalid parent, return root column count.
		return m_root_item->columnCount();
	}
	// Else look up the item and return it's column count.
	const auto id = UUIncD(parent.internalId());
	auto item = getItemById(id);
	return item->columnCount();
#endif
}

QVariant AbstractTreeModel::data(const QModelIndex &index, int role) const
{
	// data() expects a valid index.
//	Q_ASSERT(checkIndex(index, CheckIndexOption::IndexIsValid));

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

	// Return the requested [column,role] data from the item.
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
	Q_ASSERT(id != UUIncD::null());
	if(id == m_root_item->getId())
	{
		return m_root_item;
	}
	Q_ASSERT(m_model_item_map.count(id) > 0);
	return m_model_item_map.at(id).lock();
}

/// BOTH
std::shared_ptr<AbstractTreeModelItem> AbstractTreeModel::getRootItem() const
{
	return m_root_item;
}

Fun AbstractTreeModel::addItem_lambda(const std::shared_ptr<AbstractTreeModelItem>& new_item, UUIncD parentId)
{
	return [this, new_item, parentId]() {
		// Insertion is simply setting the parent of the item...
		std::shared_ptr<AbstractTreeModelItem> parent;
		if (parentId != UUIncD::null())
		{
			parent = getItemById(parentId);
			if (!parent)
			{
				Q_ASSERT(parent);
				return false;
			}
		}
		// ...and fixing up the parent.
		return new_item->changeParent(parent);
	};
}

Fun AbstractTreeModel::removeItem_lambda(UUIncD id)
{
	return [this, id]() {
		/* Deletion simply deregister clip and remove it from parent.
		   The actual object is not actually deleted, because a shared_pointer to it
		   is captured by the reverse operation.
		   Actual deletions occurs when the undo object is destroyed.
		*/
		auto item = m_model_item_map[id].lock();
		Q_ASSERT(item);
		if (!item)
		{
			return false;
		}
		auto parent = item->parent_item().lock();
		parent->removeChild(item);
		return true;
	};
}

Fun AbstractTreeModel::moveItem_lambda(UUIncD id, int destRow, bool force)
{
	Fun lambda = []() { return true; };

	std::vector<std::shared_ptr<AbstractTreeModelItem>> oldStack;
	auto item = getItemById(id);
	if (!force && item->childNumber() == destRow)
	{
		// nothing to do
		return lambda;
	}
	if (auto parent = item->parent_item().lock())
	{
		if (destRow > parent->childCount() || destRow < 0)
		{
			return []() { return false; };
		}
		UUIncD parentId = parent->getId();
		// remove the element to move
		oldStack.push_back(item);
		Fun oper = removeItem_lambda(id);
		PUSH_LAMBDA(oper, lambda);
		// remove the tail of the stack
		for (int i = destRow; i < parent->childCount(); ++i) {
			auto current = parent->child(i);
			if (current->getId() != id) {
				oldStack.push_back(current);
				oper = removeItem_lambda(current->getId());
				PUSH_LAMBDA(oper, lambda);
			}
		}
		// insert back in order
		for (const auto &elem : oldStack) {
			oper = addItem_lambda(elem, parentId);
			PUSH_LAMBDA(oper, lambda);
		}
		return lambda;
	}
	return []() { return false; };
}

AbstractTreeModel::iterator AbstractTreeModel::begin()
{
	return m_model_item_map.begin();
}

AbstractTreeModel::iterator AbstractTreeModel::end()
{
	return m_model_item_map.end();
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
//	Q_ASSERT(m_model_item_map.count(id) > 0);
	AMLM_ASSERT_GT(m_model_item_map.count(id), 0);
	m_model_item_map.erase(id);
}

#if EVER_NEEDED
void AbstractTreeModel::notifyRowsAboutToInsert(const std::shared_ptr<AbstractTreeModelItem>& row, int first, int last)
{
	/// @todo handle root item
	auto parent_index = getIndexFromItem(item);
}

void AbstractTreeModel::notifyRowsInserted(const std::shared_ptr<AbstractTreeModelItem>& row)
{

}
#endif

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
	if (!m_root_item || !m_root_item->m_is_root || !m_root_item->isInModel() || m_model_item_map.count(m_root_item->getId()) == 0)
	{
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
		if (seenIDs.count(currentId) != 0)
		{
			qDebug() << "ERROR: Invalid tree: Id found twice."
			         << "It either a cycle or a clash in id attribution";
			return false;
		}
		if (m_model_item_map.count(currentId) == 0)
		{
			qDebug() << "ERROR: Invalid tree: Id not found. Item is not registered";
			return false;
		}
		auto currentItem = m_model_item_map[currentId].lock();
		if (currentItem->depth() != currentDepth)
		{
			qDebug() << "ERROR: Invalid tree: invalid depth info found";
			return false;
		}
		if (!currentItem->isInModel())
		{
			qDebug() << "ERROR: Invalid tree: item thinks it is not in a model";
			return false;
		}
		if (currentId != m_root_item->getId())
		{
			if ((currentDepth == 0 || currentItem->m_is_root))
			{
				qDebug() << "ERROR: Invalid tree: duplicate root";
				return false;
			}
			if (auto ptr = currentItem->parent().lock())
			{
				if (ptr->getId() != parentId || ptr->child(currentItem->childNumber())->getId() != currentItem->getId())
				{
					qDebug() << "ERROR: Invalid tree: invalid parent link";
					return false;
				}
			}
			else
			{
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
		return m_root_item->data(section, role);
	}

    return QVariant();
}

QModelIndex AbstractTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	std::shared_ptr<AbstractTreeModelItem> parent_item;

	// Get the parent item.
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
		// Request is for a row beyond what the parent actually has.
        return QModelIndex();
	}

	std::shared_ptr<AbstractTreeModelItem> child_item = parent_item->child(row);

	if(child_item)
	{
		return createIndex(row, column, quintptr(child_item->getId()));
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
M_WARNING("TODO: This should be the parent_model_index, not the root_item.");
	success = m_root_item->insertColumns(insert_before_column, num_columns);
	endInsertColumns();

	return success;
}

bool AbstractTreeModel::insertRows(int insert_before_row, int num_rows, const QModelIndex& parent_model_index)
{
	Q_CHECK_PTR(m_root_item);

	std::shared_ptr<AbstractTreeModelItem> parent_item = getItemById(UUIncD(parent_model_index.internalId()));
	bool success;

	beginInsertRows(parent_model_index, insert_before_row, insert_before_row + num_rows - 1);
	success = parent_item->insertChildren(insert_before_row, num_rows, m_root_item->columnCount());
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
	std::shared_ptr<AbstractTreeModelItem> parent_item;
	if(!parent.isValid())
	{
		parent_item = m_root_item;
	}
	else
	{
		parent_item = getItemById(static_cast<UUIncD>(parent.internalId()));
	}

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
	// setData() expects a valid index.
	Q_ASSERT(checkIndex(index, CheckIndexOption::IndexIsValid));

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




