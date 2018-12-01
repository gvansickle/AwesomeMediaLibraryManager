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


#include "AbstractTreeModelItem.h"

// Qt5
#include <QStringList>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

// Ours
#include <utils/DebugHelpers.h>
#include <utils/VectorHelpers.h>

//AbstractTreeModelItem::AbstractTreeModelItem(AbstractTreeModelItem *parent_item)
//{
//	m_parent_item = parent_item;
//}

AbstractTreeModelItem::AbstractTreeModelItem(AbstractTreeModelItem* parent_item)
	: m_parent_item(parent_item)
{
}

AbstractTreeModelItem::~AbstractTreeModelItem()
{
	// Doesn't remove items, just deletes them.
	qDeleteAll(m_child_items);
}

/// Debug streaming implementation.
#define DATASTREAM_FIELDS(X) \
    X(m_parent_item)\
    /*X(m_item_data)*/\
    X(m_child_items)

#define X(field) << obj.field
QTH_DEFINE_QDEBUG_OP(AbstractTreeModelItem,
                             DATASTREAM_FIELDS(X)
                     );
#undef X

AbstractTreeModelItem* AbstractTreeModelItem::child(int number)
{
	if(number >= childCount())
	{
		qWr() << "### CHILD INDEX OUT OF RANGE:" << number;
	}

	/// @note .value() here returns a default constructed AbstractTreeModelItem which is not added to the QVector.
	/// @todo This seems all kinds of wrong, should probably return a nullptr or assert or something.
	return stdex::value(m_child_items, number);
}

const AbstractTreeModelItem* AbstractTreeModelItem::child(int number) const
{
	if(number >= childCount())
	{
		qWr() << "### CHILD INDEX OUT OF RANGE:" << number;
	}
	return stdex::value(m_child_items, number);
}


int AbstractTreeModelItem::childCount() const
{
	return m_child_items.size();
}

/**
 * Find our index in the parent's child list.
 */
int AbstractTreeModelItem::childNumber() const
{
	if (m_parent_item != nullptr)
	{
		return stdex::indexOf(m_parent_item->m_child_items, this);
	}

    return 0;
}

//int AbstractTreeModelItem::columnCount() const
//{
//	return m_item_data.count();
//}
//
//QVariant AbstractTreeModelItem::data(int column) const
//{
//	return m_item_data.value(column);
//}

bool AbstractTreeModelItem::insertChildren(int position, int count, int columns)
{
	if (position < 0 || position > m_child_items.size())
	{
		// Insertion point out of range of existing children.
        return false;
	}

	decltype(m_child_items)::iterator pos_iterator = m_child_items.begin() + position;

	for (int row = 0; row < count; ++row)
	{
//        QVector<QVariant> data(columns);
//		AbstractTreeModelItem *item = new AbstractTreeModelItem(data, this);
		// Create a new default-constructed item.
		AbstractTreeModelItem *item = create_default_constructed_child_item(this);
		m_child_items.insert(pos_iterator, item);
    }

    return true;
}

bool AbstractTreeModelItem::insertColumns(int insert_before_column, int num_columns)
{
	auto current_num_columns = columnCount();

	if (insert_before_column < 0 || insert_before_column > current_num_columns)
	{
		// Check if we're out of bounds.
		/// @todo Probably assert here?
        return false;
	}

	// Insert new columns in this.
	// Since we're ~abstract, we don't have our own data structures to resize here.
	// So I think the best thing to do is punt the adding of columns to this to the
	// derived class via this call, but insert the new columns into all of our children here in the loop below.
	bool success = derivedClassInsertColumns(insert_before_column, num_columns);
//	for (int column = 0; column < num_columns; ++column)
//	{
//		m_item_data.insert(insert_before_column, QVariant());
//	}

	// Insert new columns in children.
	for(AbstractTreeModelItem *child : m_child_items)
	{
        child->insertColumns(insert_before_column, num_columns);
	}

    return true;
}

AbstractTreeModelItem* AbstractTreeModelItem::parent()
{
	return m_parent_item;
}

const AbstractTreeModelItem* AbstractTreeModelItem::parent() const
{
	return m_parent_item;
}

bool AbstractTreeModelItem::removeChildren(int position, int count)
{
	if (position < 0 || position + count > m_child_items.size())
	{
        return false;
	}

	for (int row = 0; row < count; ++row)
	{
		delete stdex::takeAt(m_child_items, position);
	}

    return true;
}


bool AbstractTreeModelItem::removeColumns(int position, int columns)
{
	auto current_num_columns = columnCount();

	// Check that the range is legitimate.
	if (position < 0 || position + columns > current_num_columns)
	{
		return false;
	}

	// Remove our columns in derived classes.
	bool success = derivedClassRemoveColumns(position, columns);
	if(!success)
	{
		return false;
	}

	// Remove columns from all children.
	for(AbstractTreeModelItem *child : m_child_items)
	{
        child->removeColumns(position, columns);
	}

    return true;
}


bool AbstractTreeModelItem::setData(int column, const QVariant &value)
{
	auto current_num_columns = columnCount();

	if (column < 0 || column >= current_num_columns)
	{
        return false;
	}

	return derivedClassSetData(column, value);
}

bool AbstractTreeModelItem::appendChildren(std::vector<AbstractTreeModelItem*> new_children)
{
    /// @todo Support adding new columns if children have them?
    for(auto* child : new_children)
    {
        child->setParentItem(this);
        m_child_items.push_back(child);
    }

	return true;
}

void AbstractTreeModelItem::setParentItem(AbstractTreeModelItem *parent_item)
{
//	Q_ASSERT(parent_item != nullptr);
    AMLM_WARNIF(m_parent_item != nullptr);
//	AMLM_WARNIF(m_parent_item->columnCount() != this->columnCount());

	m_parent_item = parent_item;
}

