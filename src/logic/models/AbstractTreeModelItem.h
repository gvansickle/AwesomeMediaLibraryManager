/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
 *
 * This class is heavily adapted from at least the following:
 * - The "Editable Tree Model Example" shipped with Qt5.
 * - KDenLive's TreeItem class.
 * - My own original work.
 * - Hundreds of nuggets of information from all over the Internet.
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
#include <utils/RegisterQtMetatypes.h>
#include <utils/QtHelpers.h>
#include <utils/StaticAnalysis.h>
#include <future/guideline_helpers.h>
#include <future/enable_shared_from_this_virtual.h>
#include <logic/UUIncD.h>
#include <logic/serialization/ISerializable.h>
class AbstractTreeModel;

/**
 * Base class for AbstractItemTreeModel items.
 * @note Not derived from QObject.
 */
class AbstractTreeModelItem : public virtual ISerializable, public enable_shared_from_this_virtual<AbstractTreeModelItem>
{
public:

	friend class AbstractTreeModel;

	// Ok, now we're gonna burn all kinds of blood, toil, tears, and sweat trying to make both a BFS
	// and DFS iterator for AbstractTreeModelItem trees.
//	template<class ItemType = AbstractTreeModelItem>
	class bfs_iterator;

protected:

	/// Sets the model and UUIncD.
	/// ETM+KDen
	AbstractTreeModelItem(const std::vector<QVariant>& data, const std::shared_ptr<AbstractTreeModel>& model, bool is_root, UUIncD id = UUIncD::null());
	AbstractTreeModelItem(const std::shared_ptr<AbstractTreeModel>& model, bool is_root, UUIncD id = UUIncD::null());

public:
	/**
	 * Named constructor.
	 */
	/// KDEN
	static std::shared_ptr<AbstractTreeModelItem> construct(const std::vector<QVariant>& data,
			std::shared_ptr<AbstractTreeModel> model, bool isRoot, UUIncD id = UUIncD::null());
	static std::shared_ptr<AbstractTreeModelItem> construct(const QVariant& variant,
			std::shared_ptr<AbstractTreeModel> model, bool isRoot, UUIncD id = UUIncD::null());
	AbstractTreeModelItem() {};
	~AbstractTreeModelItem() override;

    /// Return a pointer to the number'th child of this item.
    /// @returns If @a number is not valid, a pointer to a default constructed AbstractTreeModelItem,
    /// 			which is not added to the QVector.
    // ETM+KDEN
	std::shared_ptr<AbstractTreeModelItem> child(int number) const;

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
	 * Insert default-constructed columns into this item/row.
	 */
	 // ETM, no KDEN
	virtual bool insertColumns(int insert_before_column, int num_columns);
	// ETM, no KDEN
	virtual bool removeColumns(int position, int columns);

	/// Returns a pointer to this item's parent.
	/// ETM+KDEN
	std::weak_ptr<AbstractTreeModelItem> parent() const;
	std::weak_ptr<AbstractTreeModelItem> parent_item() const { return parent(); };

	// KDEN, seems unused.
	int depth() const;

	/**
	 * Return the UUIncD of this item.
	 * @note Asserts if the UUIncD is ::null().
	 */
	// GRVS
	UUIncD getId() const;

	// KDEN
	bool isInModel() const;

	/// @name Operators

	/// Returns true if this and other are the same node, i.e. have the same UUIncD.
	// GRVS
	bool operator==(const AbstractTreeModelItem& other) const;

	/**
	 * Insert @a count default-constructed (i.e. empty) child items (rows), starting after child index @a position.
	 * @return true if successful.
	 */
	// ETM, KDEN only has appendChild().
	virtual bool insertChildren(int position, int count, int columns);

    /// The row number of this item in its parent's list of children.
    // ETM+KDEN (row())
    int childNumber() const;

	/**
	 * Append the given @a new_children to this item.
	 */
	// GRVS+KDEN
	bool appendChildren(std::vector<std::shared_ptr<AbstractTreeModelItem>> new_children);
	/**
	 * @note this must already have a model or this call will assert.
	 */
	// GRVS+KDEN
	bool appendChild(const std::shared_ptr<AbstractTreeModelItem>& new_child);
	/**
	 * Construct and Append a new child item to this item, initializing it from @a data.
	 */
	// KDEN
	std::shared_ptr<AbstractTreeModelItem> appendChild(const std::vector<QVariant>& data = {});

	// KDEN
	void moveChild(int ix, const std::shared_ptr<AbstractTreeModelItem> &child);

	/**
     * Remove and delete the @a count children starting at @a position.
     */
	// ETM, KDEN no plural, see removeChild().
	bool removeChildren(int position, int count);

	/**
	 * Remove given child from children list. The parent of the child is updated accordingly.
	 */
	// KDEN, ETM no singular.
	void removeChild(const std::shared_ptr<AbstractTreeModelItem>& child);

	/// DO NOT USE
	bfs_iterator begin_bfs();
	bfs_iterator end_bfs();

	/**
	 * Change the parent of the current item. Structures are modified accordingly
	 */
	// KDEN
	virtual bool changeParent(std::shared_ptr<AbstractTreeModelItem> newParent);

	/// @name Serialization
	/// These are from the ISerializable interface.
	/// Be sure to override these in derived classes.
	/// @{

	 QVariant toVariant() const override;
	 void fromVariant(const QVariant& variant) override;

	 /**
	  * Call from derived classes from within toVariant()/fromVariant() if necessary.
	  */
	 virtual QVariant toVariantGuts() const;
	 virtual void fromVariantGuts(const QVariant& variant) const;

    /// @}

    /**
     * Returns true if @a this has @a id as an ancestor.
     */
    bool has_ancestor(UUIncD id);

    // Debug stream op free func friender.
    QTH_DECLARE_FRIEND_QDEBUG_OP(AbstractTreeModelItem);

protected:
	/**
	 * Finish construction of object given its pointer.
	 * If @a self is root, calls registerSelf() to register it with the model.
	 * This is a separated function so that it can be called from derived classes
	 */
	static void baseFinishConstruct(const std::shared_ptr<AbstractTreeModelItem>& self);

	/**
	 * Helper functions to handle registration / deregistration to the model.
	 */
	static void registerSelf(const std::shared_ptr<AbstractTreeModelItem>& self);
	void deregisterSelf();

	/**
	 * Reflect update of the parent ptr (for example set this's correct depth).
     * This is called on the child when (re)parented, and meant to be overridden in derived classes.
     * @param ptr is the pointer to the new parent
	 */
	virtual void updateParent(std::shared_ptr<AbstractTreeModelItem> parent);

	/// Our guaranteed-to-be unique-to-this-run-of-the-program numeric ID.
	UUIncD m_uuincid;

	/// The actual number of columns this item (row) has.
	int m_num_columns {0};
	/// The number of columns this item's parent has, and hence the maximum (column-1) we should
	/// ever see in a model index.  -1 if unknown.
	int m_num_parent_columns {-1};

	/// The data for each column of this row.
	std::vector<QVariant> m_item_data;

	std::weak_ptr<AbstractTreeModel> m_model;

	bool m_is_root;

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

	/// Deque of shared_ptr's to child items.
	std::deque<std::shared_ptr<AbstractTreeModelItem>> m_child_items;

	int m_depth;

	bool m_is_in_model;
};

Q_DECLARE_METATYPE(AbstractTreeModelItem);
Q_DECLARE_METATYPE(std::vector<QVariant>);
Q_DECLARE_METATYPE(std::shared_ptr<AbstractTreeModelItem>);

// Debug stream op free func declaration.
QTH_DECLARE_QDEBUG_OP(AbstractTreeModelItem);

//////////////////
/// DO NOT USE, THIS DOES NOT WORK YET.
//////////////////////////////////////
class AbstractTreeModelItem::bfs_iterator : public std::iterator<
														// Category: bfs will be a LegacyInputIterator (can only be incremented, may invalidate all copies of prev value).
														/// @link https://en.cppreference.com/w/cpp/named_req/InputIterator
														std::input_iterator_tag,
														//ItemType,
														AbstractTreeModelItem,
														// Distance is meaningless
														void,
														// Pointer and Reference need to be smart.
														std::shared_ptr<AbstractTreeModelItem>,
														AbstractTreeModelItem&
														>
{
public:
	using iterator_concept = std::input_iterator_tag;

	bfs_iterator();
	explicit bfs_iterator(std::shared_ptr<AbstractTreeModelItem> root_node);

	bfs_iterator& operator++();

	bfs_iterator operator++(int);

	bool operator==(const bfs_iterator& other) const;

	bool operator!=(const bfs_iterator& other) const;

	reference operator*() const;

private:
	std::shared_ptr<AbstractTreeModelItem> m_root_node;
	std::shared_ptr<AbstractTreeModelItem> m_current_node;
	std::shared_ptr<bfs_iterator> m_child_bfs_it;
	CICTIteratorType m_child_list_it;
	bool m_is_at_end {false};
};

#endif // ABSTRACTTREEMODELITEM_H
