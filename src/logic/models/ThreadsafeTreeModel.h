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

// Std C++
#include <memory>
#include <vector>
#include <map>

// Qt5
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

// Ours
#include <logic/serialization/ISerializable.h>
#include <logic/UUIncD.h>
#include <future/enable_shared_from_this_virtual.h>
#include <logic/UndoRedoHelper.h>
#include "AbstractTreeModel.h"

/**
 * Don't know how threadsafe this really is, all indications are that QT5's model/view cannot be made threadsafe.
 * Borrowing requestXxx() concept from KDenLive's ProjectItemModel.
 * This class lives at about the same inheritance level as KDenLive's ProjectItemModel (->AbstractTreeModel->0),
 * but some work is also split into ScanResultsTreeModel.
 */
class ThreadsafeTreeModel : public AbstractTreeModel//, public virtual enable_shared_from_this_virtual<ThreadsafeTreeModel>
{
	Q_OBJECT
	Q_DISABLE_COPY(ThreadsafeTreeModel);
	Q_INTERFACES(ISerializable);

	using BASE_CLASS = AbstractTreeModel;

protected:
	explicit ThreadsafeTreeModel(std::initializer_list<ColumnSpec> column_specs, QObject* parent);

public:
	/**
	 * Named constructor.
	 */
	static std::shared_ptr<ThreadsafeTreeModel> construct(std::initializer_list<ColumnSpec> column_specs, QObject* parent = nullptr);
	~ThreadsafeTreeModel() override;

	/**
	 * Clear out the contents of this model, including all header info etc.
	 */
	void clear() override;

	/// @name The requestXxxx() interface.
	///       Borrowed from KDenLive.  Admittedly not 100% clear on why KDenLive makes model operations even more
	///       circuitous than stock Qt5 does, I think it's an attempt at threadsafety, but also undo/redo are involved.
	///       KDen doesn't have any of these in this base AbstractTreeModel class.
	/// @{

//	UUIncD requestAddItem(std::vector<QVariant> values, UUIncD parent_id,
//	                      Fun undo = noop_undo_redo_lambda, Fun redo = noop_undo_redo_lambda);

	/**
	 * Request the removal and deletion of @a item from the model.
	 */
	bool requestDeleteItem(const std::shared_ptr<AbstractTreeModelItem>& item, Fun &undo, Fun &redo);

	/// @}

protected:

	/// KDEN/ProjItemModel.

	void register_item(const std::shared_ptr<AbstractTreeModelItem>& item) override;
	void deregister_item(UUIncD id, AbstractTreeModelItem* item) override;

	/**
	 * Adds @a item to this tree model as a child of @a parent_id.
	 * This is the workhorse threadsafe function which adds all new items to the model.  It should be not be called by clients,
	 * but rather called by one of the requestAddXxxx() members.
	 */
	bool addItem(const std::shared_ptr<AbstractTreeModelItem> &item, UUIncD parent_id, Fun &undo, Fun &redo);

};

#endif /* SRC_LOGIC_MODELS_THREADSAFETREEMODEL_H_ */
