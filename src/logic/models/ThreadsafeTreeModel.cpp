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
#include "ThreadsafeTreeModel.h"
#include "ScanResultsTreeModel.h"
#include <logic/serialization/XmlSerializer.h>
#include <utils/DebugHelpers.h>
#include "AbstractTreeModelHeaderItem.h"
#include "AbstractTreeModelItem.h"
#include <KItemViews/KCategorizedSortFilterProxyModel>
#include <QAbstractItemModelTester>
#include <QtWidgets>
#include <queue>
#include <unordered_set>
#include <functional>
#include "AbstractTreeModel.h"
#include <models/ThreadsafeTreeModel.h>

bool ThreadsafeTreeModel::requestAddItem(std::vector<QVariant> values, QUuid parent_id, Fun undo, Fun redo)
{
	std::unique_lock write_lock(m_rw_mutex);

	auto new_item = AbstractTreeModelItem::construct(values, shared_from_this(), /*not root*/false);

	return addItem(new_item, parent_id, undo, redo);
}
