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
#include <memory>
#include <vector>
#include <map>

// Qt5
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

// Ours
class AbstractTreeModelItem;
class AbstractHeaderSection;
class AbstractTreeModelHeaderItem;
#include <logic/serialization/ISerializable.h>
#include <logic/UUIncD.h>
#include <future/enable_shared_from_this_virtual.h>


/**
 * Abstract tree model base class.  Inherits from QAbstractItemModel and ISerializable.
 */
class AbstractTreeModel : public QAbstractItemModel,
		public virtual ISerializable,
		public enable_shared_from_this_virtual<AbstractTreeModel>
{
    Q_OBJECT

	using BASE_CLASS = QAbstractItemModel;

public:
	static std::shared_ptr<AbstractTreeModel> construct(QObject* parent = nullptr);

protected:
	/**
	 * Creates a new AbstractTreeModel object.
	 * This model will have a default constructed AbstractTreeModelHeader with no columns and no children.
	 */
	explicit AbstractTreeModel(QObject *parent = nullptr);

public:
	~AbstractTreeModel() override;

	/**
	 * Set the ColumnSpecs in the model's root item, which holds the info for the horizontal header.
	 */
	 /// OLD
	virtual bool setColumnSpecs(std::initializer_list<QString> column_specs);

	// bool hasIndex() is not virtual.

	/**
	 * Calls getItem(index), which returns index.internalPointer() which is an AbstractTreeModelItem*.
	 * Item then returns the data for this index and role from its @a data(column) function.
	 *
	 * @todo role gets lost along the way, we need to put that in.
	 */
	 /// BOTH
    QVariant data(const QModelIndex &index, int role) const override;

    /// @todo Maybe override these.
//	QMap<int, QVariant> itemData(const QModelIndex &index) const override;
//	bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles) override;

    /// Header data interface
    /// @{

    /// Get the header data corresponding to the given section number, orientation, and role.
    /// BOTH
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /// Set the header data corresponding to the given section number, orientation, and role.
    bool setHeaderData(int section, Qt::Orientation orientation,
                         const QVariant &value, int role = Qt::EditRole) override;

    /// Overload taking an AbstractHeaderSection.
    virtual bool setHeaderData(int section, const AbstractHeaderSection& header_section);

    /// @}

    /// BOTH
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
	/// BOTH
	Qt::ItemFlags flags(const QModelIndex &index) const override;

    /// BOTH
    QModelIndex parent(const QModelIndex &index) const override;
	/// BOTH
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    /// BOTH
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;


    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    /// @name Row and column insert, remove, and move operations.
    /// @note Singular insert/remove/move row and column functions are not virtual
    ///       but are implemented in QAbstractItemModel.
    /// @{

    /**
     * Inserts @a num_columns new columns into the model before column @a insert_before_column.  If
     * @a insert_before_column is 0, columns are still prepended, and if it's columnCount(), they're still prepended
     * to the non-existent one-past-the-end column;
     * i.e. they're appended to the list.
     *
     * @return true on success.
     */
//    bool insertColumns(int insert_before_column, int num_columns,
//                       const QModelIndex& parent_model_index = QModelIndex()) override;
	/**
	 * Remove columns.
	 */
    bool removeColumns(int remove_start_column, int num_columns,
                       const QModelIndex& parent_model_index = QModelIndex()) override;

    /**
     * Inserts @a count new default-constructed rows under model item @a parent_model_index model before the given row @a insert_before_row.
     * If @a insert_before_row is 0, rows are still prepended.  If it's > rowCount(), operation is meaningless and
     * the call returns false, not having done anything.
     *
     * @return true on success.
     */
//    bool insertRows(int insert_before_row, int num_rows,
//                    const QModelIndex& parent_model_index = QModelIndex()) override;

    /**
     * Remove rows [@a remove_start_row, @a remove_start_row + @a num_rows - 1 ].
     */
    bool removeRows(int remove_start_row, int num_rows,
                    const QModelIndex& parent_item_index = QModelIndex()) override;

    /// @todo These currently just call the base class functions.
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
	                      const QModelIndex &destinationParent, int destinationChild) override;
	bool moveColumns(const QModelIndex &sourceParent, int sourceColumn, int count,
	                         const QModelIndex &destinationParent, int destinationChild) override;

	/// @} // END row/col insert/remove/move.


	/// @name Extended public model interface.
    /// @{

	/**
	 * Append a std::vector of AbstractTreeModelItem's as children of @a parent.
	 * This is effectively the same as insertRows() followed by numerous setData() calls, but the default construction
	 * of the item objects is skipped since we're passing in the @a new_items.
	 */
	virtual bool appendItems(std::vector<std::shared_ptr<AbstractTreeModelItem> > new_items, const QModelIndex &parent = QModelIndex());
	virtual bool appendItem(std::shared_ptr<AbstractTreeModelItem> new_items, const QModelIndex &parent = QModelIndex());

	QModelIndex getIndexFromItem(const std::shared_ptr<AbstractTreeModelItem>& item) const;
	QModelIndex getIndexFromId(UUIncD id) const;
	std::shared_ptr<AbstractTreeModelItem> getItemById(const UUIncD &id) const;
	std::shared_ptr<AbstractTreeModelItem> getRootItem() const;



	/// @name Serialization, from ISerializable.
	/// Remember to override these in derived classes.
	/// @{

	/// Serialize the entire model to a QVariant.
	QVariant toVariant() const override { return QVariant(); };

	/// Serialize the entire model from a QVariant.
	void fromVariant(const QVariant& variant) override {};

	/// @} // END Serialization

	/// @} // END Extended public model interface.

	friend class AbstractTreeModelItem;


protected:

	virtual void register_item(const std::shared_ptr<AbstractTreeModelItem>& item);
	virtual void deregister_item(UUIncD id, AbstractTreeModelItem* item);

	/**
	 * Send the appropriate pre-notification related to a row that we are appending.
	 * @param item is the parent item to which row is about to be appended.
	 */
	void notifyRowAboutToAppend(const std::shared_ptr<AbstractTreeModelItem> &item);

	/* @brief Send the appropriate notification related to a row that we have appended
       @param row is the new element
    */
	void notifyRowAppended(const std::shared_ptr<AbstractTreeModelItem> &row);

	/* @brief Send the appropriate notification related to a row that we are deleting
	   @param item is the parent of the row being deleted
	   @param row is the index of the row being deleted
	*/
	void notifyRowAboutToDelete(std::shared_ptr<AbstractTreeModelItem> item, int row);

	/* @brief Send the appropriate notification related to a row that we have appended
	   @param row is the old element
	*/
	void notifyRowDeleted();

public:
	/** This is a convenience function that helps check if the tree is in a valid state */
	virtual bool checkConsistency();
protected:
	/// @name Extended protected model interface.
	/// @{

	virtual QString getXmlStreamName() const { return ""; };
	virtual QString getXmlStreamVersion() const { return ""; };

    /// @}

    /// Hidden root node of the tree model.
    /// Pulls double duty as the horizontal header item.
	std::shared_ptr<AbstractTreeModelHeaderItem> m_root_item;

	/**
	 * Map of UUIncD's to AbstractTreeModelItems.
	 */
	std::map<UUIncD, std::weak_ptr<AbstractTreeModelItem>> m_model_item_map;
};


#endif // ABSTRACTTREEMODEL_H
