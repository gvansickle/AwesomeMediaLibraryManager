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

// This class's header.
#include "AbstractTreeModel.h"

// Std C++
#include <functional>

// Qt5
#include <QtWidgets>

// Ours
#include "AbstractTreeModelItem.h"
#include <utils/DebugHelpers.h>



AbstractTreeModel::AbstractTreeModel(const QStringList &headers, const QString &data, QObject *parent)
	: QAbstractItemModel(parent)
{
	/// @todo Move all this out of the constructor?
    QVector<QVariant> rootData;
	for(const QString& header : headers)
	{
        rootData << header;
	}

//	m_root_item = new AbstractTreeModelItem(rootData);
	/// @todo virtual function in constructor.
	m_root_item = AbstractTreeModel()->make_root_node(rootData);
}

AbstractTreeModel::~AbstractTreeModel()
{
	delete m_root_item;
}

int AbstractTreeModel::columnCount(const QModelIndex & /* parent */) const
{
	return m_root_item->columnCount();
}

QVariant AbstractTreeModel::data(const QModelIndex &index, int role) const
{
	Q_ASSERT(checkIndex(index, CheckIndexOption::IndexIsValid));

	if (!index.isValid())
	{
        return QVariant();
	}

    if (role != Qt::DisplayRole && role != Qt::EditRole)
	{
        return QVariant();
	}

    AbstractTreeModelItem *item = getItem(index);

    return item->data(index.column());
}

Qt::ItemFlags AbstractTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
	{
		return Qt::NoItemFlags;
	}

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

AbstractTreeModelItem *AbstractTreeModel::getItem(const QModelIndex &index) const
{
	if (index.isValid())
	{
        AbstractTreeModelItem *item = static_cast<AbstractTreeModelItem*>(index.internalPointer());
        if (item)
		{
            return item;
		}
    }
	return m_root_item;
}

void AbstractTreeModel::writeModel(QXmlStreamWriter* writer) const
{
#warning "TODO"
}

bool AbstractTreeModel::readModel(QXmlStreamReader* reader)
{
	auto& xml = *reader;

	// Check that we're reading an XML file with the right format.
	if(xml.name() == getXmlStreamName()
			&& xml.attributes().value("version") == getXmlStreamVersion())
	{
		// Start the recursive descent.
		// Whatever we find here should be the m_root_node.
		AbstractTreeModelItem* parent_item = nullptr;
		while(xml.readNextStartElement())
		{
			for(const auto& parse_func : m_parse_factory_functions)
			{
				AbstractTreeModelItem* new_item = parse_func(&xml, parent_item);
				if(new_item != nullptr)
				{
					// Parsed it.
				}
				else
				{
					// Not sure what that was.
					qIn() << "Skipping unknown element:" << xml.name();
					xml.skipCurrentElement();
				}
			}
		}

		return true;
	}
	else
	{
		return false;
	}
}

void AbstractTreeModel::writeItemAndChildren(QXmlStreamWriter* writer, AbstractTreeModelItem* item) const
{
	m_root_item->writeItemAndChildren(writer);
}

void AbstractTreeModel::readItemAndChildren(QXmlStreamWriter* writer, AbstractTreeModelItem* item)
{

}

QVariant AbstractTreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		return m_root_item->data(section);
	}

    return QVariant();
}

QModelIndex AbstractTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
	{
        return QModelIndex();
	}

    AbstractTreeModelItem *parentItem = getItem(parent);

    AbstractTreeModelItem *childItem = parentItem->child(row);
    if (childItem)
	{
        return createIndex(row, column, childItem);
	}
    else
	{
        return QModelIndex();
	}
}

bool AbstractTreeModel::insertColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginInsertColumns(parent, position, position + columns - 1);
	success = m_root_item->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool AbstractTreeModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    AbstractTreeModelItem *parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);
	success = parentItem->insertChildren(position, rows, m_root_item->columnCount());
    endInsertRows();

    return success;
}

QModelIndex AbstractTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
	{
        return QModelIndex();
	}

    AbstractTreeModelItem *childItem = getItem(index);
    AbstractTreeModelItem *parentItem = childItem->parent();

	if (parentItem == m_root_item)
	{
		return QModelIndex();
	}

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

bool AbstractTreeModel::removeColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginRemoveColumns(parent, position, position + columns - 1);
	success = m_root_item->removeColumns(position, columns);
    endRemoveColumns();

	if (m_root_item->columnCount() == 0)
	{
        removeRows(0, rowCount());
	}

    return success;
}

bool AbstractTreeModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    AbstractTreeModelItem *parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

bool AbstractTreeModel::appendItems(QVector<AbstractTreeModelItem *> new_items, const QModelIndex &parent)
{
    auto parent_item = getItem(parent);
    Q_CHECK_PTR(parent_item);

    auto first_new_row = parent_item->childCount();

    /// @todo What do we need to do to support/handle different num of columns?
    beginInsertRows(parent, first_new_row, first_new_row + new_items.size());

    parent_item->appendChildren(new_items);

    endInsertRows();

    return true;
}

int AbstractTreeModel::rowCount(const QModelIndex &parent) const
{
    AbstractTreeModelItem *parentItem = getItem(parent);

    return parentItem->childCount();
}

bool AbstractTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::EditRole)
	{
        return false;
	}

    AbstractTreeModelItem *item = getItem(index);
    bool result = item->setData(index.column(), value);

	if(result)
	{
		Q_EMIT dataChanged(index, index);
	}

    return result;
}

bool AbstractTreeModel::setHeaderData(int section, Qt::Orientation orientation,
                              const QVariant &value, int role)
{
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
	{
        return false;
	}

	bool result = m_root_item->setData(section, value);

    if (result)
	{
    	// Docs: "If you are changing the number of columns or rows you do not need to emit this signal, but use the begin/end functions."
		Q_EMIT headerDataChanged(orientation, section, section);
	}

	return result;
}

bool AbstractTreeModel::setHeaderData(const AbstractHeaderSection& header_section)
{
	Q_ASSERT(0);
//	this->setHeaderData(header_section.section(),
//			header_section.orientation(), header_section[role], role);
//	for()
	return true;
}

void AbstractTreeModel::setupModelData(const QStringList &lines, AbstractTreeModelItem *parent)
{
    QList<AbstractTreeModelItem*> parents;
    QList<int> indentations;
    parents << parent;
    indentations << 0;

    int number = 0;

	while (number < lines.count())
	{
        int position = 0;
		while (position < lines[number].length())
		{
            if (lines[number].at(position) != ' ')
			{
                break;
			}
            ++position;
        }

        QString lineData = lines[number].mid(position).trimmed();

		if (!lineData.isEmpty())
		{
            // Read the column data from the rest of the line.
            QStringList columnStrings = lineData.split("\t", QString::SkipEmptyParts);
            QVector<QVariant> columnData;
            for (int column = 0; column < columnStrings.count(); ++column)
			{
                columnData << columnStrings[column];
			}

			if (position > indentations.last())
			{
                // The last child of the current parent is now the new parent
                // unless the current parent has no children.

				if (parents.last()->childCount() > 0)
				{
                    parents << parents.last()->child(parents.last()->childCount()-1);
                    indentations << position;
                }
			}
			else
			{
				while (position < indentations.last() && parents.count() > 0)
				{
                    parents.pop_back();
                    indentations.pop_back();
                }
            }

            // Append a new item to the current parent's list of children.
            AbstractTreeModelItem *parent = parents.last();
			parent->insertChildren(parent->childCount(), 1, m_root_item->columnCount());
            for (int column = 0; column < columnData.size(); ++column)
			{
                parent->child(parent->childCount() - 1)->setData(column, columnData[column]);
			}
        }

        ++number;
    }
}
