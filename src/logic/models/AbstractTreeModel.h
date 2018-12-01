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


#ifndef ABSTRACTTREEMODEL_H
#define ABSTRACTTREEMODEL_H

// Std C++
#include "TreeModelRootItem.h"

#include <memory>


// Qt5
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

class QXmlStreamWriter;
class QXmlStreamReader;

// Ours
class AbstractTreeModelItem;
class AbstractHeaderSection;
class AbstractTreeModelHeaderItem;
class TreeModelRootItem;
#include <logic/ISerializable.h>


/**
 * Abstract tree model base class.  Inherits from QAbstractItemModel and ISerializable.
 */
class AbstractTreeModel : public QAbstractItemModel, public virtual ISerializable
{
    Q_OBJECT

	using BASE_CLASS = QAbstractItemModel;

public:
	explicit AbstractTreeModel(QObject *parent = nullptr);
	~AbstractTreeModel() override;

	/**
	 * Set the ColumnSpecs in the model's root item, which holds the info for the horizontal header.
	 */
	virtual bool setColumnSpecs(std::initializer_list<QString> column_specs);

	// bool hasIndex() is not virtual.

	/**
	 * Calls getItem(index), which returns index.internalPointer() which is an AbstractTreeModelItem*.
	 * Item then returns the data for this index and role from its @a data(column) function.
	 *
	 * @todo role gets lost along the way, we need to put that in.
	 */
    QVariant data(const QModelIndex &index, int role) const override;

    /// @todo Maybe override these.
//	QMap<int, QVariant> itemData(const QModelIndex &index) const override;
//	bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles) override;

    /// Header data interface
    /// @{

    /// Get the header data corresponding to the given section number, orientation, and role.
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    /// Set the header data corresponding to the given section number, orientation, and role.
    bool setHeaderData(int section, Qt::Orientation orientation,
                         const QVariant &value, int role = Qt::EditRole) override;

    /// Overload taking an AbstractHeaderSection.
    virtual bool setHeaderData(int section, const AbstractHeaderSection& header_section);

    /// @}

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    /// @name Row and column insert, remove, and move operations.
    /// @note Singular insert/remove/move row and column functions are not virtual
    ///       but are implemented in QAbstractItemModel.
    /// @{

    /**
     * Inserts @a count new default-constructed columns into @a parent model before the given
     * column @a insert_before_column.  If @a insert_before_column is 0, columns are still prepended,
     * and if it's columnCount(), they're still prepended to the non-existent one-past-the-end column;
     * i.e. they're appended to the list.
     *
     * @return true on success.
     */
    bool insertColumns(int insert_before_column, int count,
                       const QModelIndex &parent = QModelIndex()) override;

    bool removeColumns(int insert_before_column, int count,
                       const QModelIndex &parent = QModelIndex()) override;

    bool insertRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;

    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
	                      const QModelIndex &destinationParent, int destinationChild) override;
	bool moveColumns(const QModelIndex &sourceParent, int sourceColumn, int count,
	                         const QModelIndex &destinationParent, int destinationChild) override;

	/// @} // END row/col insert/remove/move.

	/// @name Extended public model interface.
    /// @{

	/// Append a vector of AbstractTreeModelItem's as children of @p parent.
    virtual bool appendItems(QVector<AbstractTreeModelItem*> new_items, const QModelIndex &parent = QModelIndex());

	AbstractTreeModelItem* getItem(const QModelIndex &index) const;

	/// @name Serialization, from ISerializable.
	/// Remember to override these in derived classes.
	/// @{

	/// Serialize the entire model to a QVariant.
	///   QVariant toVariant() const override = 0;

	/// Serialize the entire model from a QVariant.
	///   void fromVariant(const QVariant& variant) override = 0;

	/// @} // END Serialization

	/// @} // END Extended public model interface.

protected:

	/// @name Extended protected model interface.
	/// @{

	/**
	 * Override in derived classes to return a newly created root/header item node for the model.
	 */
	virtual AbstractTreeModelHeaderItem * make_root_node(QVector<QVariant> rootData) = 0;

	virtual QString getXmlStreamName() const = 0;
	virtual QString getXmlStreamVersion() const = 0;

    /// @}

    /// Hidden root node of the tree model.
    /// Pulls double duty as the horizontal header item.
	AbstractTreeModelHeaderItem* m_root_item;
};


#endif // ABSTRACTTREEMODEL_H
