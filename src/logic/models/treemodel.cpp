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

#include <QtWidgets>

#include "treeitem.h"
#include "treemodel.h"

// Ours
#include <UUIncD.h>
#include <utils/DebugHelpers.h>

TreeModel::TreeModel(const QStringList &headers, /*const QString &data,*/ QObject *parent)
    : QAbstractItemModel(parent)
{
    QVector<QVariant> rootData;
    foreach (QString header, headers)
        rootData << header;

    /// GRVS
    m_root_item = std::make_shared<TreeItem>(rootData);
    /// This seems sort of maybe right/maybe wrong.
    register_item(m_root_item);
//	m_model_item_map.insert({m_root_item->getId(), m_root_item});

//    setupModelData(data.split(QString("\n")), rootItem);
}

TreeModel::~TreeModel()
{
//    delete rootItem;
}

int TreeModel::columnCount(const QModelIndex & /* parent */) const
{
    return m_root_item->columnCount();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
	// data() expects a valid index, except it won't get one for data() calls for the root item info.
	Q_ASSERT(checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::DoNotUseParent));

	if (!index.isValid())
	{
		// Should never get here, checkIndex() should have asserted above.
        return QVariant();
	}

    if (role != Qt::DisplayRole && role != Qt::EditRole)
	{
        return QVariant();
	}

	std::shared_ptr<TreeItem> item = getItem(index);

    return item->data(index.column());
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
	{
		return Qt::NoItemFlags;
	}

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

std::shared_ptr<TreeItem> TreeModel::getItem(const QModelIndex &index) const
{
	/**
	 * There's a fail here.  Trying to do a mapping from QModelIndex->TreeItem*, but we're going through
	 * getItemById() to do it (index.internalId() -> TreeItem*).  In e.g. insertChild(), this results in the
	 * second lookup being done before the item is in the map, == error.
	 */
    if (index.isValid())
    {
#warning "SEE ABOVE"
//	    std::shared_ptr<TreeItem> item = static_cast<TreeItem*>(index.internalPointer());
	    std::shared_ptr<TreeItem> item = getItemById(UUIncD(index.internalId()));
        if (item)
        {
	        return item;
        }
    }
    /// ETM & AQP: return root here if invalid index for unclear reasons.
    return m_root_item;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
	    return m_root_item->data(section);
    }

    return QVariant();
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
#if 0///ETM
    if (parent.isValid() && parent.column() != 0)
	{
        return QModelIndex();
	}

	std::shared_ptr<TreeItem> parentItem = getItem(parent);

	std::shared_ptr<TreeItem> childItem = parentItem->child(row);
    if (childItem)
	{
        return createIndex(row, column, quintptr(childItem->getId()));
	}
    else
	{
        return QModelIndex();
	}
#else ///AQP
    if(!m_root_item || row < 0 || column < 0 || column > this->columnCount()
    || (parent.isValid() && parent.column() != 0))
    {
    	return QModelIndex();
    }

	std::shared_ptr<TreeItem> parentItem = getItem(parent);

    Q_ASSERT(parentItem);

    if(std::shared_ptr<TreeItem> item = parentItem->child(row))
    {
    	return createIndex(row, column, quintptr(item->getId()));
    }

    return QModelIndex();
#endif
}

bool TreeModel::insertColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginInsertColumns(parent, position, position + columns - 1);
    success = m_root_item->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool TreeModel::insertRows(int position, int rows, const QModelIndex &parent)
{
#if 1 //ETM
	qDb() << "Trying to insert:" << position << rows << parent;

	std::shared_ptr<TreeItem> parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);

    // Create @a rows default-constructed children of parent.
    auto new_children = parentItem->insertChildren(position, rows, m_root_item->columnCount());

    // Add the new children to the UUID lookup map.
    for(const auto& item : new_children)
    {
    	qDb() << "Adding UUIncD:" << item->getId() << item->columnCount();
    	m_model_item_map.insert({item->getId(), item});
    }

    success = !new_children.empty();

	endInsertRows();

    return success;
#else
    ///AQP
    if(!m_root_item)
    {
    	// No root item yet, create it.
	    QVector<QVariant> data;
	    data << "[header a]" << "[header b]";
    	m_root_item = std::make_shared<TreeItem>(data, nullptr);
    }

    std::shared_ptr<TreeItem> parent_item = parent.isValid() ? getItem(parent) : m_root_item;

	beginInsertRows(parent, position, position + rows - 1);

	for (int i = 0; i < rows; ++i)
	{
		QVector<QVariant> data;
		for(int col=0; col != columnCount(); ++i)
		{
			data << "[No data]";
		}
		std::shared_ptr<TreeItem> item = std::make_shared<TreeItem>(data, parent_item);
		parent_item->insertChild(position, item);
	}

	endInsertRows();

	return true;
#endif

}

/**
 * Insert a default-constructed row under @a parent.
 * @param parent
 */
//void TreeModel::ExternalInsertRow(const QModelIndex &parent)
//{
//	QModelIndex index = parent;
//	QAbstractItemModel *model = this;
//
//	if (!model->insertRow(index.row()+1, index.parent()))
//	{
//		return;
//	}
//
////	updateActions();
//
//	for (int column = 0; column < model->columnCount(index.parent()); ++column)
//	{
//		QModelIndex child = model->index(index.row()+1, column, index.parent());
//		model->setData(child, QVariant("[No data]"), Qt::EditRole);
//	}
//}


QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
	{
        return QModelIndex();
	}

	std::shared_ptr<TreeItem> childItem = getItem(index);
	std::shared_ptr<TreeItem> parentItem = childItem->parent();

    if (parentItem == m_root_item)
	{
        return QModelIndex();
	}

    return createIndex(parentItem->childNumber(), 0, quintptr(parentItem->getId()));
}

bool TreeModel::removeColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginRemoveColumns(parent, position, position + columns - 1);
    success = m_root_item->removeColumns(position, columns);
    endRemoveColumns();

    if (m_root_item->columnCount() == 0)
        removeRows(0, rowCount());

    return success;
}

bool TreeModel::removeRows(int position, int rows, const QModelIndex &parent)
{
	std::shared_ptr<TreeItem> parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
	if(parent.isValid() && parent.column() != 0)
	{
		// No child items in columns other than 0.
		return 0;
	}

//	// This is the root item.
//	return m_root_item->childCount();

	std::shared_ptr<TreeItem> parentItem = getItem(parent);

	if(parentItem)
	{
		return parentItem->childCount();
	}
    else
	{
    	return 0;
	}
}

bool TreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
	{
        return false;
	}

	std::shared_ptr<TreeItem> item = getItem(index);
    bool result = item->setData(index.column(), value);

    if (result)
	{
		Q_EMIT dataChanged(index, index, {role});
	}

    return result;
}

bool TreeModel::setHeaderData(int section, Qt::Orientation orientation,
                              const QVariant &value, int role)
{
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    bool result = m_root_item->setData(section, value);

    if (result)
		Q_EMIT headerDataChanged(orientation, section, section);

    return result;
}

void TreeModel::setupModelData(const QStringList &lines, TreeItem *parent)
{
    QList<TreeItem*> parents;
    QList<int> indentations;
    parents << parent;
    indentations << 0;

    int number = 0;

    while (number < lines.count()) {
        int position = 0;
        while (position < lines[number].length()) {
            if (lines[number].at(position) != ' ')
                break;
            ++position;
        }

        QString lineData = lines[number].mid(position).trimmed();

        if (!lineData.isEmpty()) {
            // Read the column data from the rest of the line.
            QStringList columnStrings = lineData.split("\t", QString::SkipEmptyParts);
            QVector<QVariant> columnData;
            for (int column = 0; column < columnStrings.count(); ++column)
                columnData << columnStrings[column];

            if (position > indentations.last()) {
                // The last child of the current parent is now the new parent
                // unless the current parent has no children.

                if (parents.last()->childCount() > 0) {
//                    parents << parents.last()->child(parents.last()->childCount()-1);
                    indentations << position;
                }
            } else {
                while (position < indentations.last() && parents.count() > 0) {
                    parents.pop_back();
                    indentations.pop_back();
                }
            }

            // Append a new item to the current parent's list of children.
            TreeItem *parent = parents.last();
            parent->insertChildren(parent->childCount(), 1, m_root_item->columnCount());
            for (int column = 0; column < columnData.size(); ++column)
                parent->child(parent->childCount() - 1)->setData(column, columnData[column]);
        }

        ++number;
	}
}

QModelIndex TreeModel::getIndexFromItem(const std::shared_ptr<TreeItem>& item) const
{
//	std::unique_lock read_lock(m_rw_mutex);

	Q_CHECK_PTR(item);
	if(item == m_root_item)
	{
		return QModelIndex();
	}
	auto parent_index = getIndexFromItem(item->parent_item().lock());
	return index(item->childNumber(), 0, parent_index);
}

/**
 * @note This function requires that the item being looked up has already been added to the model.
 * @param id
 * @return
 */
std::shared_ptr<TreeItem> TreeModel::getItemById(const UUIncD& id) const
{
	Q_ASSERT(m_root_item);
	Q_ASSERT(id != UUIncD::null());

	if(id == m_root_item->getId())
	{
		return m_root_item;
	}
//	#error "An empty model dies here in the view, via parent()/getId() duing an insert"
	AMLM_ASSERT_GT(m_model_item_map.count(id), 0);
	return m_model_item_map.at(id).lock();
}

/**
 * Insert an empty new child under @a parent and returns a shared_ptr to it.
 * ETM: From MainWindow, where parent is always currentIndex() from a selection model.
 * @param parent
 * @return
 */
std::shared_ptr<TreeItem> TreeModel::insertChild(const QModelIndex &parent)
{
	const QModelIndex index = parent;
	std::shared_ptr<TreeModel> model = this->shared_from_this();

	// Is there no such parent?
	if(model->columnCount(index) == 0)
	{
		// No parent with a column 0, so create it by inserting a column 0.
		/// GRVS Seems a bit odd.
		if(!model->insertColumn(0, index))
		{
			return nullptr;
		}
	}

	if(!model->insertRow(0, index))
	{
		return nullptr;
	}

	// Ok, at this point we should have a new default-constructed child in parent's child list.
	// We need to add it to this model's UUIncD<->TreeItem map.
//	register_item();

	QModelIndex child;
	for (int column = 0; column < model->columnCount(index); ++column)
	{
		child = model->index(0, column, index);
//#error /// This fails because the TreeItem backing child hasn't been added to the model map yet, so circular dependency.
		auto shpt = getItem(child);
		model->register_item(shpt);
		model->setData(child, QVariant("[No data]"), Qt::EditRole);
		if (!model->headerData(column, Qt::Horizontal).isValid())
		{
			model->setHeaderData(column, Qt::Horizontal, QVariant("[No header]"), Qt::EditRole);
		}
	}

	return getItem(child);
}

std::shared_ptr<TreeItem> TreeModel::append_child(const QVector<QVariant> &data, std::shared_ptr<TreeItem> parent)
{
	// Append a new item to the given parent's list of children.
//	TreeItem *parent = parents.last();
	parent->insertChildren(parent->childCount(), 1, m_root_item->columnCount());
	QVector<QVariant> columnData;
	for(int column = 0; column < data.size(); ++column)
	{
		columnData << data[column];
	}
	std::shared_ptr<TreeItem> new_child = parent->child(parent->childCount()-1);
	register_item(new_child);
	for(int column = 0; column < columnData.size(); ++column)
	{
		new_child->setData(column, columnData[column]);
	}

	return new_child;
}

//void TreeModel::insertRow(const QModelIndex& parent)
//{
//	QModelIndex index = parent;
//	auto model = shared_from_this();
//
//	if (!model->insertRow(index.row()+1, index.parent()))
//	{
//		return;
//	}
//
////	updateActions();
//
//	for (int column = 0; column < model->columnCount(index.parent()); ++column)
//	{
//		QModelIndex child = model->index(index.row()+1, column, index.parent());
//		model->setData(child, QVariant("[No data]"), Qt::EditRole);
//	}
//}

///GRVS
void TreeModel::register_item(const std::shared_ptr<TreeItem>& child)
{
	UUIncD id = child->getId();
	Q_ASSERT(id.isValid());
//	AMLM_ASSERT_X(m_model_item_map.count(id) == 0, "Model already has an entry for child");
	m_model_item_map[id] = child;
}
///GRVS

