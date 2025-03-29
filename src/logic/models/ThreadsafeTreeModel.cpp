/*
 * Copyright 2019, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
 * @file ThreadsafeTreeModel.cpp
 */

// This
#include "ThreadsafeTreeModel.h"

// Std C++
#include <queue> /// ???
#include <unordered_set>
#include <functional>

// Qt
#include <QAbstractItemModelTester>

// Ours
#include <shared_mutex>
#include <logic/serialization/XmlSerializer.h>
#include <models/UndoRedoHelper.h>
#include <utils/DebugHelpers.h>
#include "AbstractTreeModelHeaderItem.h"
#include "AbstractTreeModelItem.h"
#include "AbstractTreeModel.h"





ThreadsafeTreeModel::ThreadsafeTreeModel(std::initializer_list<ColumnSpec> column_specs, QObject* parent)
	: AbstractTreeModel(column_specs, parent)
{

}

//std::shared_ptr<ThreadsafeTreeModel> ThreadsafeTreeModel::construct(std::initializer_list<ColumnSpec> column_specs,
//		QObject* parent)
//{
//	std::shared_ptr<ThreadsafeTreeModel> self(new ThreadsafeTreeModel(column_specs, parent));
//	self->m_root_item = AbstractTreeModelHeaderItem::construct(column_specs, self);
//	return self;
//}

ThreadsafeTreeModel::~ThreadsafeTreeModel()
{
	// Same as KdenLive's ProjectModelItem, its destructor is defaulted.
}

void ThreadsafeTreeModel::clear(bool quit)
{
    QWriteLocker locker(&m_rw_mutex);

	m_closing = true;

	std::vector<std::shared_ptr<AbstractTreeModelItem>> items_to_delete;

	items_to_delete.reserve(m_root_item->childCount());

	for (int i = 0; i < m_root_item->childCount(); ++i)
	{
		items_to_delete.push_back(std::static_pointer_cast<AbstractTreeModelItem>(m_root_item->child(i)));
	}
	Fun undo = []() { return true; };
	Fun redo = []() { return true; };
	for (const auto &child : items_to_delete)
	{
		qDb() << "Clearing" << items_to_delete.size() << "items, current:" << child->m_uuid;
		requestDeleteItem(child, undo, redo);
	}
	items_to_delete.clear();
	Q_ASSERT(m_root_item->childCount() == 0);
	m_closing = false;
	if (!quit)
	{
		// KDen: m_uuid = QUuid::createUuid();
	}

	// One last thing, our hidden root node / header node still has ColumnSpecs.
	// m_root_item->clear();
}

bool ThreadsafeTreeModel::requestDeleteItem(const std::shared_ptr<AbstractTreeModelItem>& item, Fun& undo, Fun& redo)
{
	#if 0///
	std::unique_lock write_locker(m_rw_mutex);
	Q_ASSERT(item);
	if (!item)
	{
		return false;
	}
	UUIncD parentId = UUIncD::null();

	if (std::shared_ptr<AbstractTreeModelItem> ptr = item->parent_item().lock())
	{
		parentId = ptr->getId();
	}
//	bool isSubClip = item->itemType() == AbstractProjectItem::SubClipItem;
	item->selfSoftDelete(undo, redo);
	UUIncD id = item->getId();
	Fun operation = removeItem_lambda(id);
	Fun reverse = addItem_lambda(item, parentId);
	bool request_was_successful = operation();
	if (request_was_successful)
	{
//		if (isSubClip)
//		{
//			Fun update_doc = [this, binId]() {
//				std::shared_ptr<AbstractProjectItem> parentItem = getItemByBinId(binId);
//				if (parentItem && parentItem->itemType() == AbstractProjectItem::ClipItem) {
//					auto clipItem = std::static_pointer_cast<ProjectClip>(parentItem);
//					clipItem->updateZones();
//				}
//				return true;
//			};
//			update_doc();
//			PUSH_LAMBDA(update_doc, operation);
//			PUSH_LAMBDA(update_doc, reverse);
//		}
		UPDATE_UNDO_REDO(m_rw_mutex, operation, reverse, undo, redo);
	}

	return request_was_successful;
#endif///
	return false;
}

QVariant ThreadsafeTreeModel::data(const QModelIndex& index, int role) const
{
    READ_LOCK()
	return BASE_CLASS::data(index, role);
}

bool ThreadsafeTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    QWriteLocker locker(&m_rw_mutex);
	return BASE_CLASS::setData(index, value, role);
}

QVariant ThreadsafeTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    READ_LOCK()
	return BASE_CLASS::headerData(section, orientation, role);
}

Qt::ItemFlags ThreadsafeTreeModel::flags(const QModelIndex& index) const
{
    READ_LOCK()

	return BASE_CLASS::flags(index);
}

std::shared_ptr<AbstractTreeModelItem> ThreadsafeTreeModel::getItem(const QModelIndex& index) const
{
    READ_LOCK()
	return AbstractTreeModel::getItem(index);
}

int ThreadsafeTreeModel::columnCount(const QModelIndex& parent) const
{
	// KDen: This is mutex + the same as AbstractTreeModel.
    READ_LOCK()

	if (!parent.isValid())
	{
		// Invalid index, return root column count.
		return m_root_item->columnCount();
	}
	// Else look up the item and return its column count.
	const auto id = UUIncD(parent.internalId());
	auto item = getItemById(id);
	return item->columnCount();
}

int ThreadsafeTreeModel::rowCount(const QModelIndex& parent) const
{
    READ_LOCK()
	return BASE_CLASS::rowCount(parent);
}

bool ThreadsafeTreeModel::requestAddItem(std::shared_ptr<AbstractTreeModelItem> new_item, UUIncD parent_id, Fun undo, Fun redo)
{
    QWriteLocker locker(&m_rw_mutex);

    bool status = addItem(new_item, parent_id, undo, redo);

    return status;
}

void ThreadsafeTreeModel::register_item(const std::shared_ptr<AbstractTreeModelItem>& item)
{
    QWriteLocker locker(&m_rw_mutex);

	BASE_CLASS::register_item(item);
}

void ThreadsafeTreeModel::deregister_item(UUIncD id, AbstractTreeModelItem* item)
{
    QWriteLocker locker(&m_rw_mutex);

	// Per KdenLive:
	// "here, we should suspend jobs belonging to the item we delete. They can be restarted if the item is reinserted by undo"

	BASE_CLASS::deregister_item(id, item);

}

bool ThreadsafeTreeModel::addItem(const std::shared_ptr<AbstractTreeModelItem>& item, UUIncD parent_id, Fun& undo, Fun& redo)
{
    QWriteLocker locker(&m_rw_mutex);

	std::shared_ptr<AbstractTreeModelItem> parent_item = getItemById(parent_id);

	if(!parent_item)
	{
		qCr() << "ERROR, BAD PARENT ITEM with ID:" << parent_id; // << "," << item;
		return false;
	}

	Fun operation = addItem_lambda(item, parent_item->getId());

	UUIncD itemId = item->getId();
	Fun reverse = removeItem_lambda(itemId);
	bool res = operation();
	Q_ASSERT(item->isInModel());
	if (res)
	{
        UPDATE_UNDO_REDO(m_rw_mutex, operation, reverse, undo, redo);
	}
	return res;
}
