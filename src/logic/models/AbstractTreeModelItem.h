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
 * Adapted from the "Editable Tree Model Example" shipped with Qt5.
 */

/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef ABSTRACTTREEMODELITEM_H
#define ABSTRACTTREEMODELITEM_H

// Std C++
#include <memory>
#include <deque>

// Qt5
#include <QList>
#include <QVariant>
#include <QVector>

// Ours
#include <utils/QtHelpers.h>
#include <utils/StaticAnalysis.h>
#include <future/guideline_helpers.h>
#include <future/cloneable.h>
#include <logic/UUIncD.h>
#include <logic/serialization/ISerializable.h>
class AbstractTreeModel;

/**
 * Base class for AbstractItemTreeModel items.
 * @note Not derived from QObject.
 */
class AbstractTreeModelItem : public virtual ISerializable, public std::enable_shared_from_this<AbstractTreeModelItem>
{

public:
	static std::shared_ptr<AbstractTreeModelItem> construct(std::shared_ptr<AbstractTreeModel>& model, bool isRoot,
			UUIncD id = UUIncD::null());

protected:
//	M_GH_RULE_OF_FIVE_DEFAULT_C21(AbstractTreeModelItem);
	/// Default constructor.  Sets the UUIncD.
	AbstractTreeModelItem(const std::shared_ptr<AbstractTreeModel>& model, bool is_root, UUIncD id = UUIncD::null());

public:
//	/**
//	 * Default constructor.
//	 * @param parent_item  The AbstractTreeModelItem which is both the owner and "tree-wise" parent of this item.
//	 *                     @note This is completely unrelated to QObject parentage, this class isn't derived from QObject.
//	 *                     However, we still own our children and have to delete them on destruction.
//	 */
//	explicit AbstractTreeModelItem(AbstractTreeModelItem* parent_item);
	~AbstractTreeModelItem() override;

    /// Return a pointer to the number'th child of this item.
    /// @returns If @a number is not valid, a pointer to a default constructed AbstractTreeModelItem,
    /// 			which is not added to the QVector.
	/// @todo That seems all kinds of wrong, should probably return a nullptr or throw or something.
	std::shared_ptr<AbstractTreeModelItem> child(int number);

	/// @copydoc AbstractTreeModelItem::child(int)
	/// Const version.
	const std::shared_ptr<AbstractTreeModelItem> child(int number) const;

    /// @returns The number of children this item has.
    virtual int childCount() const;

    /// @returns The number of columns of data this item has.
    /// This is the max of the column count of all child items.
	virtual int columnCount() const { return 0; };

    /**
     * Read access to the data of this item.
     * @param column  The column of data to return.
     * @return A QVariant containing all the data in @a column.
     */
	virtual QVariant data(int column, int role = Qt::DisplayRole) const;

	bool setData(int column, const QVariant &value);

	/**
	 * Insert @a count default-constructed (i.e. empty) child items (rows), starting after child index @a position.
	 * Default construction is via the create_default_constructed_child_item() function (pure virtual here).
	 * @return true if successful.
	 */
    virtual bool insertChildren(int position, int count, int columns);

    virtual bool insertColumns(int insert_before_column, int num_columns);

    /// Returns a pointer to this item's parent.
	std::weak_ptr<AbstractTreeModelItem> parent();
	/// Returns a const pointer to this item's parent.
	const std::weak_ptr<AbstractTreeModelItem> parent() const;

	/**
	 * Return the UUIncD of this item.
	 */
	UUIncD getId() const;

	/**
	 * Remove and delete the @a count children starting at @a position.
	 */
    bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);

    /// The row number of this item in its parent's list of children.
    int childNumber() const;

	/**
	 * Append the given @a new_children to this item.  This item takes ownership of the children via std::unique_ptr<>,
	 * and is set as the parent of the child items.
	 * @param new_children
	 * @return
	 */
	bool appendChildren(std::vector<std::shared_ptr<AbstractTreeModelItem>> new_children);
	bool appendChild(std::shared_ptr<AbstractTreeModelItem> new_child);

	/// @name Serialization
	/// These are from the ISerializable interface.
	/// Be sure to override these in derived classes.
	/// @{

	 QVariant toVariant() const override { return QVariant(); };
	 void fromVariant(const QVariant& variant) override {};

    /// @}

    // Debug stream op free func friender.
    QTH_DECLARE_FRIEND_QDEBUG_OP(AbstractTreeModelItem);

protected:
	/* @brief Finish construction of object given its pointer
       This is a separated function so that it can be called from derived classes */
	static void baseFinishConstruct(const std::shared_ptr<AbstractTreeModelItem>& self);

	/* @brief Helper functions to handle registration / deregistration to the model */
	static void registerSelf(const std::shared_ptr<AbstractTreeModelItem>& self);
	void deregisterSelf();

    /// Sets this item's parent item to parent_item.
    /// Primarily for use in appendChildren().
	virtual void setParentItem(std::shared_ptr<AbstractTreeModelItem> parent_item);

	/**
	 * Non-virtual Interface factory function for creating default-constructed child nodes.
	 * Used by insertChildren().  Do not attempt to override in derived classes.
	 */
	std::unique_ptr<AbstractTreeModelItem>
	create_default_constructed_child_item(AbstractTreeModelItem* parent, int num_columns);

	/**
	 * The covariant-return-type factory function for child items.  Override in derived classes.
	 */
	virtual AbstractTreeModelItem*
	do_create_default_constructed_child_item(AbstractTreeModelItem* parent, int num_columns) { return 0; };

	/// @name Virtual functions called by the base class to complete certain operations.
	///       The base class will have error-checked function parameters.
	/// @{
	virtual bool derivedClassSetData(int column, const QVariant &value) { return 0; };
	virtual bool derivedClassInsertColumns(int insert_before_column, int num_columns) { return 0; };
	virtual bool derivedClassRemoveColumns(int first_column_to_remove, int num_columns) { return 0; };
	/// @}

	/// Our guaranteed-to-be unique-to-this-run-of-the-program numeric ID.
	UUIncD m_uuincid;

private:

	/// Pointer to our parent AbstractTreeModelItem.
	/// For items in a tree model (i.e. not being copy/pasted or mid-construction), this will always
	/// be non-null as long as this item is not the invisible root item.
	std::weak_ptr<AbstractTreeModelItem> m_parent_item;

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
