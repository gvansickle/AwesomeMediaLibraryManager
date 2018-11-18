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
class QXmlStreamWriter;

// Ours
#include <src/utils/QtHelpers.h>

#include <logic/ISerializable.h>

/**
 * Generic item for use in AbstractItemTreeModel.
 */
class AbstractTreeModelItem : public ISerializable
{
public:
	explicit AbstractTreeModelItem(AbstractTreeModelItem *parent = nullptr);
	explicit AbstractTreeModelItem(const QVector<QVariant> &data, AbstractTreeModelItem *parent = nullptr);
	 ~AbstractTreeModelItem() override;

    /// Return a pointer to the number'th child of this item.
    /// @returns Pointer to a default constructed AbstractTreeModelItem, which is not added to the QVector.
	/// @todo This seems all kinds of wrong, should probably return a nullptr or assert or something.
	AbstractTreeModelItem *child(int number);

	/// Const version.
	const AbstractTreeModelItem *child(int number) const;

    /// The number of children this item has.
    int childCount() const;

    int columnCount() const;
	virtual QVariant data(int column) const;

    bool insertChildren(int position, int count, int columns);
    bool insertColumns(int position, int columns);

    AbstractTreeModelItem *parent();
	const AbstractTreeModelItem *parent() const;

    bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);

    /// The row number of this item in its parent's list of children.
    int childNumber() const;

    bool setData(int column, const QVariant &value);

    bool appendChildren(QVector<AbstractTreeModelItem*> new_children);

	// Serialization
    // Be sure to override these in derived classes.
    //virtual QVariant toVariant() const = 0;
    //virtual void fromVariant(const QVariant& variant) = 0;

	/**
	 * Write this item and any children to the given QXmlStreamWriter.
	 * Override this in derived classes to do the right thing.
	 * @returns true
	 */
	virtual bool writeItemAndChildren(QXmlStreamWriter* writer) const = 0;


    // Debug stream op free func friender.
    QTH_FRIEND_QDEBUG_OP(AbstractTreeModelItem)

protected:

    /// Sets this item's parent item to parent_item.
    /// Primarily for use in appendChildren().
    virtual void setParentItem(AbstractTreeModelItem* parent_item);

	/// Factory function for creating default-constructed nodes.
	/// Used by insertChildren().
	AbstractTreeModelItem* make_default_node(const QVector<QVariant> &data, AbstractTreeModelItem *parent = nullptr);


private:

	// Vector of child items.
	std::vector<AbstractTreeModelItem*> m_child_items;

	// Vector of items for each column.
	QVector<QVariant> m_item_data;

	// Pointer to our parent item.
	AbstractTreeModelItem *m_parent_item;
};

// Debug stream op free func declaration.
QTH_DECLARE_QDEBUG_OP(AbstractTreeModelItem);

#endif // ABSTRACTTREEMODELITEM_H
