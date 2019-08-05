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

/*
    treeitem.cpp

    A container for items of data supplied by the simple tree model.
*/

#include "treeitem.h"

#include <QStringList>


TreeItem::TreeItem(const QVector<QVariant> &data, const std::shared_ptr<TreeItem>& parent, UUIncD id)
{
	// Generate or set the UUIncD.
	if(!m_uuincid.isValid())
	{
		if(id.isValid())
		{
			m_uuincid = id;
		}
		else
		{
			m_uuincid = UUIncD::create();
		}
	}

	// AQT
//	if(auto strptr = parentItem.lock())
//	{
//		strptr->addChild()
//	}

    parentItem = parent;
    m_item_data = data;
}

TreeItem::~TreeItem()
{
	// Nothing to do, smart_ptr's will delete our child items.
//    qDeleteAll(childItems);
}

std::shared_ptr<TreeItem> TreeItem::child(int number)
{
    return m_child_items.value(number);
}

int TreeItem::childCount() const
{
    return m_child_items.count();
}

int TreeItem::childNumber() const
{
#if 0
	if (auto shpt = m_parent_item.lock())
	{
		// We compute the distance in the parent's children list
		auto it = shpt->m_child_items.begin();
		return (int)std::distance(it, (decltype(it))shpt->get_m_child_items_iterator(m_uuincid));
	}
	else
	{
		/// @note Expired parent item. KDenLive doesn't do this.
		qWr() << "EXPIRED PARENT ITEM";
//		Q_ASSERT(0);
	}

    return -1;
#endif
	if(auto par = parentItem.lock())
	{
		return par->m_child_items.indexOf(std::const_pointer_cast<TreeItem>(this->shared_from_this()));
	}

    return 0;
}
//! [4]

//! [5]
int TreeItem::columnCount() const
{
    return m_item_data.count();
}


QVariant TreeItem::data(int column) const
{
    return m_item_data.value(column);
}

std::vector<std::shared_ptr<TreeItem>> TreeItem::insertChildren(int position, int count, int columns)
{
	std::vector<std::shared_ptr<TreeItem>> retval;

    if (position < 0 || position > m_child_items.size())
	{
    	// Trying to insert children at an invalid position.
    	// Empty vector == fail.
        return retval;
	}

	for (int row = 0; row < count; ++row)
	{
		// Create the new TreeItem.
		/// @note The new item needs to know its parent, which we give it here, and then it needs to be
		/// added to a model such that it can be looked up via its UUIncD.
        QVector<QVariant> data(columns);
		std::shared_ptr<TreeItem> item = std::make_shared<TreeItem>(data, this->shared_from_this());
        m_child_items.insert(position, item);
        retval.push_back(item);
    }

	return retval;
}

std::shared_ptr<TreeItem> TreeItem::insertChild(int row, std::shared_ptr<TreeItem> item)
{
#if 0 /// AQP
//	return insertChildren(row, 1, )
	item->parentItem = this->shared_from_this();
	m_child_items.insert(row, item);
#else // ETM
	auto retval = insertChildren(row, 1, this->columnCount());
	return retval[0];
#endif
}


bool TreeItem::insertColumns(int position, int columns)
{
    if (position < 0 || position > m_item_data.size())
        return false;

    for (int column = 0; column < columns; ++column)
        m_item_data.insert(position, QVariant());

	for(std::shared_ptr<TreeItem> child : m_child_items)
	{
        child->insertColumns(position, columns);
	}

    return true;
}

std::shared_ptr<TreeItem> TreeItem::parent()
{
	return parentItem.lock();
}

std::weak_ptr<TreeItem> TreeItem::parent_item() const
{
	return parentItem;
}


bool TreeItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > m_child_items.size())
        return false;

    for (int row = 0; row < count; ++row)
	{
		/*delete*/ m_child_items.takeAt(position);
	}

    return true;
}
//! [10]

bool TreeItem::removeColumns(int position, int columns)
{
    if (position < 0 || position + columns > m_item_data.size())
	{
        return false;
	}

    for (int column = 0; column < columns; ++column)
	{
        m_item_data.remove(position);
	}

	for(std::shared_ptr<TreeItem> child : m_child_items)
	{
        child->removeColumns(position, columns);
	}

    return true;
}

bool TreeItem::setData(int column, const QVariant &value)
{
    if (column < 0 || column >= m_item_data.size())
        return false;

    m_item_data[column] = value;
	return true;
}

UUIncD TreeItem::getId() const
{
	AMLM_ASSERT_X(m_uuincid != UUIncD::null(), "This should never trip under any circumstances.");
	return m_uuincid;
}

//void TreeItem::append_child(TreeItem* new_item)
//{
//	// Append a new item to this item's list of children.
//	insertChildren(childCount(), 1, rootItem->columnCount());
//	for(int column = 0; column < )
//	{
//		child(childCount()-1)->setData(column, columnData[column]);
//	}
//}
