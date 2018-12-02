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
#include <vector>

// Qt5
#include <QList>
#include <QVariant>
#include <QVector>

// Ours
#include <src/utils/QtHelpers.h>
#include <utils/StaticAnalysis.h>
#include <logic/ISerializable.h>

/**
 * Base class for AbstractItemTreeModel items.
 */
class AbstractTreeModelItem : public virtual ISerializable
{

public:
	/**
	 *
	 * @param parent_item  The AbstractTreeModelItem which is both the owner and "tree-wise" parent of this item.
	 *                     @note This is completely unrelated to QObject parentage, this class isn't derived from QObject.
	 *                     However, we still own our children and have to delete them on destruction.
	 */
	explicit AbstractTreeModelItem(AbstractTreeModelItem* parent_item = nullptr);
	~AbstractTreeModelItem() override;

	/**
	 * @todo Virtual constructor returning covariant model item pointer.
	 * Think we'll probably need this.
	 */
//	virtual AbstractTreeModelItem* create() const = 0;
	/// Virtual constructor for copy.
//	virtual AbstractTreeModelItem* clone() const = 0;

    /// Return a pointer to the number'th child of this item.
    /// @returns If @a number is not valid, a pointer to a default constructed AbstractTreeModelItem,
    /// 			which is not added to the QVector.
	/// @todo That seems all kinds of wrong, should probably return a nullptr or assert or something.
	AbstractTreeModelItem* child(int number);

	/// @copydoc AbstractTreeModelItem::child(int)
	/// Const version.
	const AbstractTreeModelItem* child(int number) const;

    /// @returns The number of children this item has.
    virtual int childCount() const;

    /// @returns The number of columns of data this item has.
    /// This is the max of the column count of all child items.
    virtual int columnCount() const = 0;

    /**
     * Read access to the data of this item.
     * @param column  The column of data to return.
     * @return A QVariant containing all the data in @a column.
     */
	virtual QVariant data(int column) const = 0;

	bool setData(int column, const QVariant &value);

	/**
	 * Insert @a count default-constructed (i.e. empty) child items (rows), starting after child index @a position.
	 * Default construction is via the create_default_constructed_child_item() function (pure virtual here).
	 * @return true if successful.
	 */
    virtual bool insertChildren(int position, int count, int columns);

    virtual bool insertColumns(int insert_before_column, int num_columns);

    /// Returns a pointer to this item's parent.
    AbstractTreeModelItem *parent();
	/// Returns a const pointer to this item's parent.
	const AbstractTreeModelItem *parent() const;

    bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);

    /// The row number of this item in its parent's list of children.
    int childNumber() const;


	bool appendChildren(std::vector<AbstractTreeModelItem*> new_children);

	/// @name Serialization
	/// These are from the ISerializable interface.
	/// Be sure to override these in derived classes.
	/// @{

    // virtual QVariant toVariant() const = 0;
    // virtual void fromVariant(const QVariant& variant) = 0;

    /// @}

    // Debug stream op free func friender.
    QTH_FRIEND_QDEBUG_OP(AbstractTreeModelItem);

protected:

    /// Sets this item's parent item to parent_item.
    /// Primarily for use in appendChildren().
    virtual void setParentItem(AbstractTreeModelItem* parent_item);

	/**
	 * Factory function for creating default-constructed child nodes.
	 * Used by insertChildren().  Override in derived classes.
	 * @todo Convert to smart pointer (std::unique_ptr<AbstractTreeModelItem>) return type, retain covariant return.
	 */
	virtual AbstractTreeModelItem*
	create_default_constructed_child_item(AbstractTreeModelItem* parent) = 0;

	/// @name Virtual functions called by the base class to complete certain operations.
	///       The base class will have error-checked function parameters.
	/// @{
	virtual bool derivedClassSetData(int column, const QVariant &value) = 0;
	virtual bool derivedClassInsertColumns(int insert_before_column, int num_columns) = 0;
	virtual bool derivedClassRemoveColumns(int first_column_to_remove, int num_columns) = 0;
	/// @}

private:

	/// Pointer to our parent AbstractTreeModelItem.
	/// For items in a tree model (i.e. not being copy/pasted or mid-construction), this will always
	/// be non-null as long as this item is not the invisible root item.
	AbstractTreeModelItem *m_parent_item { nullptr };

	/// Vector of child items.
	/// This item owns its children for memory-management purposes.
	std::vector<gsl::owner<AbstractTreeModelItem*>> m_child_items;

	/// @note Any actual item data beyond the child items is the responsibility of derived classes.
};

// Debug stream op free func declaration.
QTH_DECLARE_QDEBUG_OP(AbstractTreeModelItem);

#endif // ABSTRACTTREEMODELITEM_H
