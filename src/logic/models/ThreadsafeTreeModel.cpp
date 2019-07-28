/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

// Qt5
#include <QAbstractItemModelTester>

// Ours
#include <logic/serialization/XmlSerializer.h>
#include <utils/DebugHelpers.h>
#include <logic/UndoRedoHelper.h>
#include "AbstractTreeModelHeaderItem.h"
#include "AbstractTreeModelItem.h"
#include "AbstractTreeModel.h"


ThreadsafeTreeModel::ThreadsafeTreeModel(std::initializer_list<ColumnSpec> column_specs, QObject* parent)
	: AbstractTreeModel(column_specs, parent)
{

}

std::shared_ptr<ThreadsafeTreeModel> ThreadsafeTreeModel::construct(std::initializer_list<ColumnSpec> column_specs,
		QObject* parent)
{
	std::shared_ptr<ThreadsafeTreeModel> self(new ThreadsafeTreeModel(column_specs, parent));
	self->m_root_item = AbstractTreeModelHeaderItem::construct(column_specs, self);
	return self;
}

ThreadsafeTreeModel::~ThreadsafeTreeModel()
{
	// Same as KdenLive's ProjectModelItem, it's destructor is defaulted.
}

void ThreadsafeTreeModel::clear()
{
	std::unique_lock write_lock(m_rw_mutex);

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
		qDb() << "clearing" << items_to_delete.size() << "items, current:" << child->m_uuid;
		requestDeleteItem(child, undo, redo);
	}
	Q_ASSERT(m_root_item->childCount() == 0);
	//	m_fileWatcher->clear();
}

bool ThreadsafeTreeModel::requestDeleteItem(const std::shared_ptr<AbstractTreeModelItem>& item, Fun& undo, Fun& redo)
{
	std::unique_lock write_locker(m_rw_mutex);
	Q_ASSERT(item);
	if (!item)
	{
		return false;
	}
	UUIncD parentId = UUIncD::null();
	QString binId;
	if (std::shared_ptr<AbstractTreeModelItem> ptr = item->parent_item().lock())
	{
		parentId = ptr->getId();
//		binId = ptr->clipId();
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
}

//UUIncD ThreadsafeTreeModel::requestAddItem(std::vector<QVariant> values, UUIncD parent_id, Fun undo, Fun redo)
//{
//	std::unique_lock write_lock(m_rw_mutex);

//	auto new_item = AbstractTreeModelItem::construct(values, std::static_pointer_cast<ThreadsafeTreeModel>(shared_from_this()), /*not root*/false);

//	bool status = addItem(new_item, parent_id, undo, redo);

//	if(!status)
//	{
//		// Add failed for some reason, return a null UUIncD.
//		return UUIncD::null();
//	}
//	return new_item->getId();
//}

void ThreadsafeTreeModel::register_item(const std::shared_ptr<AbstractTreeModelItem>& item)
{
	std::unique_lock write_lock(m_rw_mutex);

	BASE_CLASS::register_item(item);
}

void ThreadsafeTreeModel::deregister_item(UUIncD id, AbstractTreeModelItem* item)
{
	std::unique_lock write_lock(m_rw_mutex);

	// Per KdenLive:
	// "here, we should suspend jobs belonging to the item we delete. They can be restarted if the item is reinserted by undo"

	BASE_CLASS::deregister_item(id, item);

}

bool ThreadsafeTreeModel::addItem(const std::shared_ptr<AbstractTreeModelItem>& item, UUIncD parent_id, Fun& undo, Fun& redo)
{
	std::unique_lock write_lock(m_rw_mutex);

	auto parent_item = getItemById(parent_id);
	Q_CHECK_PTR(parent_item);

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
