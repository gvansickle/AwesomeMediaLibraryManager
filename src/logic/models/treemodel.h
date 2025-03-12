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

#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <memory>
#include <map>

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

// Ours
class UUIncD;
#include <future/enable_shared_from_this_virtual.h>

class TreeItem;

class TreeModel : public QAbstractItemModel, public enable_shared_from_this_virtual<TreeModel>
{
    Q_OBJECT

public:
    TreeModel(const QStringList &headers, /*const QString &data,*/
              QObject *parent = 0);
    ~TreeModel();

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

	Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    bool setHeaderData(int section, Qt::Orientation orientation,
                       const QVariant &value, int role = Qt::EditRole) override;

    bool insertColumns(int position, int columns,
                       const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int position, int columns,
                       const QModelIndex &parent = QModelIndex()) override;
    bool insertRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;

	/// GRVS
	/// ETM: Slots From MainWindow, where parent is always from a selection.
	std::shared_ptr<TreeItem> insertChild(const QModelIndex &parent = QModelIndex());
//	void ExternalInsertRow(const QModelIndex &parent = QModelIndex());
	/// GRVS
	/// ETM-inspired append function.  Based on setupModelData().
	std::shared_ptr<TreeItem> append_child(const QVector<QVariant> &data, std::shared_ptr<TreeItem> parent);

//private:
    void setupModelData(const QStringList &lines, TreeItem *parent);

	QModelIndex getIndexFromItem(const std::shared_ptr<TreeItem>& item) const;
	std::shared_ptr<TreeItem> getItemById(const UUIncD &id) const;
	std::shared_ptr<TreeItem> getItem(const QModelIndex &index) const;
	std::weak_ptr<TreeItem> getRootItem() const;

protected:
	/**
	 * Must be called so this model knows about @a child.
	 * @param child
	 */
	virtual void register_item(const std::shared_ptr<TreeItem>& child);
	virtual void deregister_item(UUIncD id, TreeItem* item);

private:
    std::shared_ptr<TreeItem> m_root_item;

	/**
	 * Map of UUIncD's to AbstractTreeModelItems.
	 */
	using item_map_type = std::map<UUIncD, std::weak_ptr<TreeItem>>;
//	std::map<UUIncD, std::weak_ptr<AbstractTreeModelItem>> m_model_item_map;
	item_map_type m_model_item_map;
};

#endif // TREEMODEL_H
