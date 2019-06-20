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
#include <mutex>

// Qt5
#include <QList>
#include <QVariant>
#include <QVector>

// Ours
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

	/**
	 * Named constructor.
	 * Taking a QVariant list mostly to keep as close as I can to the two designs I'm mostly leaning on here.
	 */
	static std::shared_ptr<AbstractTreeModelItem> construct(const QVector<QVariant>& data,
			std::shared_ptr<AbstractTreeModel> model, bool isRoot, UUIncD id = UUIncD::null());

protected:
	/// Sets the model and UUIncD.
	AbstractTreeModelItem(const QVector<QVariant>& data, const std::shared_ptr<AbstractTreeModel>& model,
			bool is_root, UUIncD id = UUIncD::null());

public:
	~AbstractTreeModelItem() override;

    /// Return a pointer to the number'th child of this item.
    /// @returns If @a number is not valid, a pointer to a default constructed AbstractTreeModelItem,
    /// 			which is not added to the QVector.
	std::shared_ptr<AbstractTreeModelItem> child(int number) const;

    /// @returns The number of children this item has.
    virtual int childCount() const;

    /// @returns The number of columns of data this item has.
    /// This is the max of the column count of all child items.
	virtual int columnCount() const;

    /**
     * Read access to the data of this item.
     * @param column  The column of data to return.
     * @return A QVariant containing all the data in @a column.
     */
	virtual QVariant data(int column, int role = Qt::DisplayRole) const;

	/// @todo NEW: Return the QVariant in @a column.  Not sure if this is needed.
	QVariant dataColumn(int column) const;

	bool setData(int column, const QVariant &value);

	/**
	 * Insert @a count default-constructed (i.e. empty) child items (rows), starting after child index @a position.
	 * Default construction is via the create_default_constructed_child_item() function (pure virtual here).
	 * @return true if successful.
	 */
//    virtual bool insertChildren(int position, int count, int columns);

    virtual bool insertColumns(int insert_before_column, int num_columns);

	/// Returns a const pointer to this item's parent.
	std::weak_ptr<AbstractTreeModelItem> parent_item() const;

	int depth() const;

	/**
	 * Return the UUIncD of this item.
	 * @note Asserts if the UUIncD is ::null().
	 */
	UUIncD getId() const;

	bool isInModel() const;

	/**
	 * Remove and delete the @a count children starting at @a position.
	 */
    bool removeChildren(int position, int count);


    bool removeColumns(int position, int columns);

    /// The row number of this item in its parent's list of children.
    int childNumber() const;

	/**
	 * Append the given @a new_children to this item.
	 */
	bool appendChildren(std::vector<std::shared_ptr<AbstractTreeModelItem>> new_children);
	/**
	 * @note this must already have a model or this call will assert.
	 */
	bool appendChild(const std::shared_ptr<AbstractTreeModelItem>& new_child);
	/**
	 * Construct and Append a new child item to this item, initializing it from @a data.
	 */
	std::shared_ptr<AbstractTreeModelItem> appendChild(const QVector<QVariant>& data = {});

	void moveChild(int ix, const std::shared_ptr<AbstractTreeModelItem> &child);

	/**
	 * Remove given child from children list. The parent of the child is updated accordingly.
	 */
	void removeChild(const std::shared_ptr<AbstractTreeModelItem>& child);

	/* @brief Change the parent of the current item. Structures are modified accordingly
	 */
	virtual bool changeParent(std::shared_ptr<AbstractTreeModelItem> newParent);

	/// @name Serialization
	/// These are from the ISerializable interface.
	/// Be sure to override these in derived classes.
	/// @{

	 QVariant toVariant() const override { return QVariant(); };
	 void fromVariant(const QVariant& variant) override {};

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
	 * If @a self is root, calls registerSelf() to register it with the model, otherwise does nothing.
	 * This is a separated function so that it can be called from derived classes
	 */
	static void baseFinishConstruct(const std::shared_ptr<AbstractTreeModelItem>& self);

	/**
	 * Helper functions to handle registration / deregistration to the model.
	 */
	static void registerSelf(const std::shared_ptr<AbstractTreeModelItem>& self);
	void deregisterSelf();

	/**
	 * Reflect update of the parent ptr (for example set the correct depth).
     * This is meant to be overridden in derived classes
     * @param ptr is the pointer to the new parent
	 */
	virtual void updateParent(std::shared_ptr<AbstractTreeModelItem> parent);

	/// @name Virtual functions called by the base class to complete certain operations.
	///       The base class will have error-checked function parameters.
	/// @todo I think these are obsolete now, remove.
	/// @{
	virtual bool derivedClassSetData(int column, const QVariant &value) { return 0; };
	virtual bool derivedClassInsertColumns(int insert_before_column, int num_columns) { return 0; };
	virtual bool derivedClassRemoveColumns(int first_column_to_remove, int num_columns) { return 0; };
	/// @}



	/// Our guaranteed-to-be unique-to-this-run-of-the-program numeric ID.
	UUIncD m_uuincid;

	/// Pointer to our parent AbstractTreeModelItem.
	/// For items in a tree model (i.e. not being copy/pasted or mid-construction), this will always
	/// be non-null as long as this item is not the invisible root item.
	std::weak_ptr<AbstractTreeModelItem> m_parent_item;

	/// Mutex for derived classes' use.
	/// @note Should try to make this non-recursive.
	mutable std::recursive_mutex m_mutex;

private:

	using ChildItemContainerType = std::deque<std::shared_ptr<AbstractTreeModelItem>>;
	using CICTIteratorType = ChildItemContainerType::iterator;

	/**
	 * Returns an iterator pointing to the item with id @a id in @var m_child_items, or m_child_items.end() if not found.
	 * @param id
	 * @return
	 */
	CICTIteratorType get_m_child_items_iterator(UUIncD id);

	/// Deque of shared_ptr's to child items.
	std::deque<std::shared_ptr<AbstractTreeModelItem>> m_child_items;

	std::weak_ptr<AbstractTreeModel> m_model;

	int m_depth;

	bool m_is_in_model;
	bool m_is_root;

	/// @note AbstractTreeModelItem contains no data members for actual item data.
	/// Any actual item data beyond the child items is the responsibility of derived classes.
};

// Debug stream op free func declaration.
QTH_DECLARE_QDEBUG_OP(AbstractTreeModelItem);

#endif // ABSTRACTTREEMODELITEM_H
