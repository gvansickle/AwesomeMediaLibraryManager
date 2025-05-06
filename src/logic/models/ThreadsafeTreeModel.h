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
 * @file ThreadsafeTreeModel.h
 */
#ifndef SRC_LOGIC_MODELS_THREADSAFETREEMODEL_H_
#define SRC_LOGIC_MODELS_THREADSAFETREEMODEL_H_

// Std C++
#include <memory>
#include <vector>
#include <map>
#include <shared_mutex>

// Qt
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QReadWriteLock>

// Ours
#include <logic/serialization/ISerializable.h>
#include <logic/UUIncD.h>
#include <future/enable_shared_from_this_virtual.h>
#include "UndoRedoHelper.h"
#include "AbstractTreeModel.h"
#include "ColumnSpec.h"

/**
 * Don't know how threadsafe this really is, all indications are that QT's model/view cannot be made threadsafe.
 * Borrowing requestXxx() concept from KDenLive's ProjectItemModel.
 * This class lives at about the same inheritance level as KDenLive's ProjectItemModel (->AbstractTreeModel->0),
 * but some work is also split into ScanResultsTreeModel.
 */
class ThreadsafeTreeModel : public AbstractTreeModel, public virtual ISerializable, public enable_shared_from_this_virtual<ThreadsafeTreeModel>
{
	Q_OBJECT
	Q_DISABLE_COPY(ThreadsafeTreeModel);
	Q_INTERFACES(ISerializable);

	using BASE_CLASS = AbstractTreeModel;

protected:
	explicit ThreadsafeTreeModel(std::initializer_list<ColumnSpec> column_specs, QObject* parent);

public:
//	/**
//	 * Named constructor.
//	 */
//	static std::shared_ptr<ThreadsafeTreeModel> construct(std::initializer_list<ColumnSpec> column_specs, QObject* parent = nullptr);
//	explicit ThreadsafeTreeModel(std::initializer_list<ColumnSpec> column_specs, QObject* parent);
	~ThreadsafeTreeModel() override;

	/**
	 * Clear out the contents of this model, including all header info etc.
	 */
    void clear(bool quit) override;


	/// @name The requestXxxx() interface.
	///       Borrowed from KDenLive.  Admittedly not 100% clear on why KDenLive makes model operations even more
	///       circuitous than stock Qt does, I think it's an attempt at threadsafety, but also undo/redo are involved.
	///       KDen doesn't have any of these in this base AbstractTreeModel class.
	/// @{

    /**
     * Add a new AbstractTreeModelItem to the tree.
     */
	bool requestAddItem(std::shared_ptr<AbstractTreeModelItem> new_item, UUIncD parent_id,
						Fun undo = noop_undo_redo_lambda, Fun redo = noop_undo_redo_lambda);

	/**
	 * Request the removal and deletion of @a item from the model.
	 */
	bool requestDeleteItem(const std::shared_ptr<AbstractTreeModelItem>& item, Fun &undo, Fun &redo);

	/// @}

	QVariant data(const QModelIndex &index, int role) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	int columnCount(const QModelIndex& parent) const override;
	int rowCount(const QModelIndex& parent) const override;

	QModelIndex getIndexFromId(UUIncD id) const override;
	std::shared_ptr<AbstractTreeModelItem> getItemById(const UUIncD& id) const override;
	std::shared_ptr<AbstractTreeModelItem> getRootItem() const override;
	std::shared_ptr<AbstractTreeModelItem> getItem(const QModelIndex& index) const override;

protected:

    // KDEN/ProjItemModel.
    bool m_closing;

	void register_item(const std::shared_ptr<AbstractTreeModelItem>& item) override;
	void deregister_item(UUIncD id, AbstractTreeModelItem* item) override;

	/**
	 * Adds @a item to this tree model as a child of @a parent_id.
	 * This is the workhorse threadsafe function which adds all new items to the model.  It should be not be called by clients,
	 * but rather called by one of the requestAddXxxx() members.
	 */
	bool addItem(const std::shared_ptr<AbstractTreeModelItem> &item, UUIncD parent_id, Fun &undo, Fun &redo);


	/**
	 * Single writer/multi-reader mutex.
	 * @todo The KDenLive code has/needs this to be recursive, but we should try to un-recurse it.
	 */
    // mutable std::shared_mutex m_rw_mutex;
	// mutable std::recursive_mutex m_rw_mutex;
    mutable QReadWriteLock m_rw_mutex {QReadWriteLock::RecursionMode::Recursive};
};




#endif /* SRC_LOGIC_MODELS_THREADSAFETREEMODEL_H_ */
