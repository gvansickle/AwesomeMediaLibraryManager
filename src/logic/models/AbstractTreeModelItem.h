﻿/*
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
 * @file AbstractTreeModelItem.cpp
 * Implementation of AbstractTreeModelItem.
 */

#ifndef ABSTRACTTREEMODELITEM_H
#define ABSTRACTTREEMODELITEM_H

// Std C++
#include <memory>
#include <deque>
#include <vector>
#include <mutex>
#include <iterator>

// Qt5
#include <QList>
#include <QVariant>
#include <QVector>

// Ours
#include <utils/RegisterQtMetatypes.h> ///< For smart_ptrs etc.
#include <utils/QtHelpers.h>
#include <utils/StaticAnalysis.h>
#include <future/guideline_helpers.h>
#include <future/enable_shared_from_this_virtual.h>
#include <logic/UUIncD.h>
#include <logic/serialization/ISerializable.h>
#include "UndoRedoHelper.h"
class AbstractTreeModel;

/**
 * Base class for AbstractItemTreeModel items.
 * This class is heavily influenced and adapted from at least the following:
 * - The "Editable Tree Model Example" shipped with Qt5.
 * - KDenLive's TreeItem and AbstractProjectItem classes.
 * - My own original work.
 * - Hundreds of nuggets of information from all over the Internet.
 * @note Not derived from QObject.
 */
class AbstractTreeModelItem : public virtual ISerializable, public enable_shared_from_this_virtual<AbstractTreeModelItem>
{

	friend class AbstractTreeModel;

public:
	M_GH_RULE_OF_FIVE_DEFAULT_C21(AbstractTreeModelItem);

public:
	AbstractTreeModelItem(const std::initializer_list<QVariant>& data,
				const std::shared_ptr<AbstractTreeModelItem>& parent_item = nullptr, UUIncD id = UUIncD::null());
	explicit AbstractTreeModelItem(const std::vector<QVariant>& data,
			const std::shared_ptr<AbstractTreeModelItem>& parent_item = nullptr, UUIncD id = UUIncD::null());
	~AbstractTreeModelItem() override;

	/**
	 * KDEN has a clean() in ProjectItemModel, nothing in item.
	 * ETM has nothing like this.
	 * AQP has one in the model which essentially just deletes the root item, and a takeChild()
	 * in the item which is what the removeRows() override uses.
	 */
	virtual void clear();

	/**
	 * From KDenLive:
	 * "This function executes what should be done when the item is deleted but without deleting effectively.
	 * For example, the item will deregister itself from the model and delete the clips from the timeline.
	 * However, the object is NOT actually deleted, and the tree structure is preserved.
	 * @param Undo,Redo are the lambdas accumulating the update.
	 */
	virtual bool selfSoftDelete(Fun &undo, Fun &redo);

    /// Return a pointer to the number'th child of this item.
    /// @returns If @a number is not valid, a pointer to a default constructed AbstractTreeModelItem,
    /// 			which is not added to the QVector.
    // ETM+KDEN
	std::shared_ptr<AbstractTreeModelItem> child(int number);

    /// ETM+KDEN
    /// @returns The number of children this item has.
    virtual int childCount() const;

    /// ETM+KDEN
    /// @returns The number of columns of data this item has.
	virtual int columnCount() const;

    /**
     * Read access to the data of this item.
     * @param column  The column of data to return.
     * @return A QVariant containing all the data in @a column.
     */
     // Both ETM&KDEN have this, but they don't take a role param.
	virtual QVariant data(int column, int role = Qt::DisplayRole) const;

	/// @todo NEW: Return the QVariant in @a column.  Not sure if this is needed.
	// KDEN, see data().
	QVariant dataColumn(int column) const;

	// ETM+KDEN
	bool setData(int column, const QVariant &value);

	/**
	 * Insert new default-constructed columns into this item/row.
	 * @note These are for this item, not model-level.
	 */
//M_WARNING("NEED TO BE OVERRIDDEN IN HeaderItem");
	 // ETM, no KDEN
	virtual bool insertColumns(int insert_before_column, int num_columns);
	// ETM, no KDEN
	virtual bool removeColumns(int position, int columns);

	/// Returns a weak_ptr to this item's parent.
	/// ETM+KDEN
	std::weak_ptr<AbstractTreeModelItem> parent_item() const;
	/// Returns a shared_ptr to the parent item.
	std::shared_ptr<AbstractTreeModelItem> parent() const;


	/// KDEN, used in checkConsistency().
	int depth() const;

	/**
	 * Return the UUIncD of this item.
	 * @note Asserts if the UUIncD is ::null().
	 */
	// GRVS
	UUIncD getId() const;
	void setId(UUIncD id);

	// KDEN
	bool isInModel() const;

	/// @name Operators

	/// Returns true if this and other are the same node, i.e. have the same UUIncD.
	// GRVS
	bool operator==(const AbstractTreeModelItem& other) const;

	/// The row number of this item in its parent's list of children.
	// ETM+KDEN (row())
	int childNumber() const;

	/// @name Child append/insert functions.
	/// @{

	/**
	 * @note This is where all(?) children are ultimately created.
	 */
	std::vector<std::shared_ptr<AbstractTreeModelItem>> insertChildren(int position, int count, int columns);

	/**
	 * Insert an already-existing item into this's child list before @a row.
	 * @note This is a little bit of each of AQP, ETM, KDEN, and my own.
	 * - ETM has something like this in MainWindow which just inserts a new default child and returns void.
	 * - AQP has this interface, but all it does is: { item->m_parent = this; m_children.insert(row, item); }
	 * - KDEN's closest equivalent is TreeItem::appendChild(), which does a lot of work, and is closest to what I'm
	 *   doing here.
	 */
	void insertChild(int row, std::shared_ptr<AbstractTreeModelItem> item);

	/**
	 * Append the given @a new_children to this item.
	 */
	// GRVS+KDEN
	bool appendChildren(std::vector<std::shared_ptr<AbstractTreeModelItem>> new_children);
	/**
	 * Append an already-created child item to this item.
	 * @note GRVS+KDEN,AQP has this as addChild().
	 */
	bool appendChild(const std::shared_ptr<AbstractTreeModelItem>& new_child);

	/// @} // END Child append/insert functions.


	// KDEN
	void moveChild(int ix, const std::shared_ptr<AbstractTreeModelItem> &child);

	/**
     * Remove and delete the @a count children starting at @a position.
     * @note ETM has this using takeAt(). KDEN has no such plural in the item, see removeChild(), and doesn't overload
     *       removeChildren() in the model either, which seems odd.  AQP has only takeChild() here and does overload
     *       removeRows() in the model, which uses that to remove children.
     */
	bool removeChildren(int position, int count);

	/**
	 * Remove given child from children list. The parent of the child is updated accordingly.
	 */
	// KDEN, ETM no singular.
	void removeChild(const std::shared_ptr<AbstractTreeModelItem>& child);

	/**
	 * Change the parent of the current item. Structures are modified accordingly
	 */
	// KDEN/GRVS
	virtual bool changeParent(std::shared_ptr<AbstractTreeModelItem> newParent);

	/// @name Serialization
	/// These are from the ISerializable interface.
	/// Be sure to override these in derived classes.
	/// @{

	 QVariant toVariant() const override;
	 void fromVariant(const QVariant& variant) override;

	/// KDEN
	/**
	 *
	 * @tparam T
	 * @tparam BinOp
	 * @param init
	 * @param op      Operation applied to each subtree item.  Signature: T BinOp(T, std::shared_ptr<AbstractTreeModelItem>)
	 * @return
	 */
	template <class T, class BinOp>
	T accumulate(T init, BinOp op);
	template <class T, class BinOp>
	T accumulate_const(T init, BinOp op) const;

    /// @}

    /**
     * Returns true if @a this has @a id as an ancestor.
     */
    bool has_ancestor(UUIncD id);

	bool has_children() const;

	/**
	 * @brief Return true if the item thinks it is a root.
	 * Note that it should be consistent with what the model thinks, but it may have been
	 * messed up at some point if someone wrongly constructed the object with isRoot = true
	 */
	bool isRoot() const;

    // Debug stream op free func friender.
    QTH_DECLARE_FRIEND_QDEBUG_OP(AbstractTreeModelItem);

protected:

	/**
	 * Helper functions to handle registration / deregistration to the model.
	 */
	static void register_self(const std::shared_ptr<AbstractTreeModelItem>& self);
	/**
	 * Remove the subtree rooted at this item from the model @ m_model by recursively calling @a m_model->derigister_item().
	 * Does effectively nothing if item is not in a model.
	 */
	void deregister_self();

	/**
	 * KDEN
	 * GRVS: Sets this item's parent item and updates its depth.
	 * Reflect update of the parent ptr (for example set this's correct depth).
     * This is called on the child when (re)parented, and meant to be overridden in derived classes.
     * @param ptr is the pointer to the new parent
	 */
	virtual void updateParent(std::shared_ptr<AbstractTreeModelItem> parent);

	/// @name Protected Serialization Helpers
	/// @{

	virtual int item_data_to_variant(InsertionOrderedStrVarMap* add_to_map) const;
	virtual int item_data_from_variant(const InsertionOrderedStrVarMap& read_from_map);

	/**
	 * Serialization helper which recursively serializes this item's children to a map.
	 */
	QVariant children_to_variant() const;

	/**
	 * Serialization helper which recursively serializes this item's children from a map.
	 */
	void children_from_variant(const QVariant& variant);

	/// @}

	/// @name Pre/Post-condition checks
	/// @{

	/**
	 * Verify postconditions after a child item is added or inserted to this item.
	 * @param inserted_child
	 */
	void verify_post_add_ins_child(const std::shared_ptr<AbstractTreeModelItem>& inserted_child);

	/// @}

	template <class T, class MapType>
	static void set_map_class_info(const T* self, MapType* map)
	{
		int id = qMetaTypeId<T>();
		qDb() << "QMetaType:" << id << QMetaType::typeName(id);// << QVariant(*this).typeName();
		map->m_metatype_id = id;
		map->m_class = QMetaType::typeName(id);
	}

	template <class MapType>
	static void set_map_class_info(const std::string& classname, MapType* map)
	{
//		int id = qMetaTypeId<T>();
		int id = 0;
//		qDb() << "QMetaType:" << id << QMetaType::typeName(id);// << QVariant(*this).typeName();
		qDb() << "No QMetatype, class:" << classname << "id:" << id;
		map->m_id = id;
//		map->m_class = QMetaType::typeName(id);
		map->m_class = classname;
	}

	template <class T, class MapType>
	static void dump_map_class_info(const T* self, MapType* map)
	{
		int id = qMetaTypeId<T>();
		qDb() << "QMetaType:" << id << QMetaType::typeName(id);
//		map->m_id = id;
//		map->m_class = QMetaType::typeName(id);
	}


	/**
	 * Mutex in support of undo/redo, specifically in selfSoftDelete().
	 * Note that this is separate from the mutex in the model.
	 * @todo The KDenLive code has/needs this to be recursive, but we should try to un-recurse it.
	 */
//	mutable std::shared_mutex m_rw_mutex;
//	mutable std::recursive_mutex m_rw_mutex;

	/// Our guaranteed-to-be unique-to-this-run-of-the-program numeric ID.
	UUIncD m_uuincid;

	/// The actual number of columns this item (row) has.
//	int m_num_columns {0};
	/// The number of columns this item's parent has, and hence the maximum (column-1) we should
	/// ever see in a model index.  -1 if unknown.
//	int m_num_parent_columns {-1};

	/// The data for each column of this row.
	std::vector<QVariant> m_item_data;

	std::weak_ptr<AbstractTreeModel> m_model;

	bool m_is_root {false};

	/// Deque of shared_ptr's to child items.
	std::deque<std::shared_ptr<AbstractTreeModelItem>> m_child_items;

private:

	using ChildItemContainerType = std::deque<std::shared_ptr<AbstractTreeModelItem>>;
	using CICTIteratorType = ChildItemContainerType::iterator;

	/**
	 * Returns an iterator pointing to the item with id @a id in @var m_child_items, or m_child_items.end() if not found.
	 * @param id
	 * @return
	 */
	CICTIteratorType get_m_child_items_iterator(UUIncD id);

	/// Pointer to our parent AbstractTreeModelItem.
	/// For items in a tree model (i.e. not being copy/pasted or mid-construction), this will always
	/// be non-null as long as this item is not the invisible root item.
	std::weak_ptr<AbstractTreeModelItem> m_parent_item;

	/// The depth of this item in its tree.
	///KDEN
	int m_depth {0};
};

// For name access and QVariant use.
Q_DECLARE_METATYPE(AbstractTreeModelItem);
Q_DECLARE_METATYPE(std::vector<QVariant>);
Q_DECLARE_METATYPE(std::weak_ptr<AbstractTreeModelItem>);
Q_DECLARE_METATYPE(std::shared_ptr<AbstractTreeModelItem>);

// Debug stream op free func declaration.
QTH_DECLARE_QDEBUG_OP(AbstractTreeModelItem);


template <class T, class BinOp>
T AbstractTreeModelItem::accumulate(T init, BinOp op)
{
	T res = op(init, shared_from_this());
	for (const auto &c : m_child_items)
	{
		res = c->accumulate(res, op);
	}
	return res;
}
template <class T, class BinOp>
T AbstractTreeModelItem::accumulate_const(T init, BinOp op) const
{
	T res = op(init, shared_from_this());
	for (const auto &c : m_child_items)
	{
		res = c->accumulate_const(res, op);
	}
	return res;
}

/**
 * Attempt to use The Power Of Templates(tm) to make a factory factory.
 */
template <class T, class... Args>
std::shared_ptr<T> TreeItemFactory(Args... args)
{
	std::shared_ptr<T> retval = std::make_shared<T>(std::forward<Args>(args)...);
	retval->postConstructorFinalization();

  return retval;
}




#endif // ABSTRACTTREEMODELITEM_H
