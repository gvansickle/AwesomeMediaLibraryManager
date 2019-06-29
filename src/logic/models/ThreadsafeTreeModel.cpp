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


ThreadsafeTreeModel::ThreadsafeTreeModel(QObject* parent) : AbstractTreeModel(parent)
{

}

std::shared_ptr<ThreadsafeTreeModel> ThreadsafeTreeModel::construct(QObject* parent)
{
	std::shared_ptr<ThreadsafeTreeModel> self(new ThreadsafeTreeModel(parent));
	self->m_root_item = AbstractTreeModelHeaderItem::construct(self);
	return self;
}

ThreadsafeTreeModel::~ThreadsafeTreeModel()
{

}

bool ThreadsafeTreeModel::requestAddItem(std::vector<QVariant> values, UUIncD parent_id, Fun undo, Fun redo)
{
	std::unique_lock write_lock(m_rw_mutex);

	auto new_item = AbstractTreeModelItem::construct(values, shared_from_this(), /*not root*/false);

	return addItem(new_item, parent_id, undo, redo);
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
