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
#include <logic/ISerializable.h>


class AbstractTreeModel : public QAbstractItemModel, public ISerializable
{
    Q_OBJECT

public:
	explicit AbstractTreeModel(QObject *parent = nullptr);
	explicit AbstractTreeModel(/*const QStringList &headers,*/ const QString &data,
			  QObject *parent = nullptr);
	~AbstractTreeModel() override;


	/**
	 * Calls getItem(index), which returns index.internalPointer() which is an AbstractTreeModelItem*.
	 * Item then returns the data for this index and role from its @a data(column) function.
	 *
	 * @todo role gets lost along the way, we need to put that in.
	 */
    QVariant data(const QModelIndex &index, int role) const override;

    /// Header data interface
    /// @{

    /**
     * Get the header data corresponding to the given section number, orientation, and role.
     */
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    /**
     * Set the header data corresponding to the given section number, orientation, and role.
     */
    bool setHeaderData(int section, Qt::Orientation orientation,
                         const QVariant &value, int role = Qt::EditRole) override;

    /// Overload taking an AbstractHeaderSection.
    virtual bool setHeaderData(const AbstractHeaderSection& header_section);
    /// @}

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;


    bool insertColumns(int position, int columns,
                       const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int position, int columns,
                       const QModelIndex &parent = QModelIndex()) override;
    bool insertRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;

	/// @name Extended public model interface.
    /// @{

	/// Set the root item, which doubles as the header item.
	virtual void setRootItem(AbstractTreeModelHeaderItem* root_header_item);


	/// Append a vector of AbstractTreeModelItem's as children of @p parent.
    virtual bool appendItems(QVector<AbstractTreeModelItem*> new_items, const QModelIndex &parent = QModelIndex());

	AbstractTreeModelItem *getItem(const QModelIndex &index) const;

	/**
	 * Write the entire model to the given QXmlStreamWriter.
	 * Override this in derived classes to do the right thing.
	 */
	virtual void writeModel(QXmlStreamWriter* writer) const;

	/**
	 * Read the entire model from the given QXmlStreamWriter.
	 * Override this in derived classes to do the right thing.
	 * @returns false if model could not be read from reader.
	 */
	virtual bool readModel(QXmlStreamReader* reader);

	/// @}

protected:

	/// @name Extended protected model interface.
	/// @{

	virtual AbstractTreeModelHeaderItem * make_root_node(QVector<QVariant> rootData) = 0;
//	virtual AbstractTreeModelItem* make_default_node(QVector<QVariant> rootData, AbstractTreeModelItem* parent) = 0;

	/**
	 * Write the given item to the given QXmlStreamWriter.
	 * Override this in derived classes to do the right thing.
	 */
	virtual void writeItemAndChildren(QXmlStreamWriter* writer, AbstractTreeModelItem* item) const;

	virtual void readItemAndChildren(QXmlStreamWriter* writer, AbstractTreeModelItem* item);

	virtual QString getXmlStreamName() const = 0;
	virtual QString getXmlStreamVersion() const = 0;

	friend class AbstractTreeModelWriter;
	friend class AbstractTreeModelReader;

	/// Recursive descent parser factory functions.
	/// At least for the first level of descent.
	/// Functions take XML stream reader and parent model node, return new node if parsing was successful.
	std::vector<std::function<AbstractTreeModelItem*(QXmlStreamReader*, AbstractTreeModelItem*)>> m_parse_factory_functions;

    /// @}

	AbstractTreeModelHeaderItem* m_root_item;


private:
	void setupModelData(const QStringList &lines, AbstractTreeModelItem *parent);

};


#endif // ABSTRACTTREEMODEL_H
