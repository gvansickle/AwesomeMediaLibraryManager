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
 * @file ThreadsafeTreeModel.h
 */
#ifndef SRC_LOGIC_MODELS_THREADSAFETREEMODEL_H_
#define SRC_LOGIC_MODELS_THREADSAFETREEMODEL_H_

#include <memory>
#include <vector>
#include <map>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <logic/serialization/ISerializable.h>
#include <logic/UUIncD.h>
#include <future/enable_shared_from_this_virtual.h>
#include <logic/UndoRedoHelper.h>
#include "AbstractTreeModel.h"

/**
 * Don't know how threadsafe this really is, all indications are that QT5's model/view cannot be made threadsafe.
 * Borrowing requestXxx() concept from KDenLive's ProjectItemModel.
 */
class ThreadsafeTreeModel : public AbstractTreeModel
{
	Q_OBJECT

public:
	static std::shared_ptr<ThreadsafeTreeModel> construct(QObject* parent = nullptr);

protected:
	explicit ThreadsafeTreeModel(QObject* parent);

public:
	~ThreadsafeTreeModel() override;

	bool requestAddItem(std::vector<QVariant> values, QUuid parent_id, Fun undo, Fun redo);

protected:
	/// KDEN/ProjItemModel.
	bool addItem(const std::shared_ptr<AbstractTreeModelItem>);
};

#endif /* SRC_LOGIC_MODELS_THREADSAFETREEMODEL_H_ */
