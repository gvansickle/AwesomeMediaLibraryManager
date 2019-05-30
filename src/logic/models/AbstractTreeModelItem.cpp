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
#include <QBrush>
#include <QStringList>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

// Ours
#include <utils/DebugHelpers.h>
#include <utils/VectorHelpers.h>
#include "AbstractTreeModel.h"

AbstractTreeModelItem* AbstractTreeModelItem::make_tree_item(const std::vector<QVariant>& columns, AbstractTreeModel* model, bool is_root, int id)
{
	AbstractTreeModelItem* self = new AbstractTreeModelItem(columns, model, is_root, id);
	base_finish_construction(self);
	return self;
}

AbstractTreeModelItem::AbstractTreeModelItem(const std::vector<QVariant>& columns, AbstractTreeModel* model, bool is_root, int id)
	: m_column_data(columns), m_model(model), m_id(id == -1 ? AbstractTreeModel::get_next_child_id() : id), m_is_root(is_root)
{
}

void AbstractTreeModelItem::base_finish_construction(const AbstractTreeModelItem* self)
{
	/// @todo
}

AbstractTreeModelItem::~AbstractTreeModelItem()
{
	// Doesn't remove child items, just deletes them.
//	qDeleteAll(m_child_items);
M_WARNING("FIXME: Both these are virtual calls");
	if(childCount() > 0)
	{
		// Remove and delete all children.
		removeChildren(0, childCount());
	}
}

/// Debug streaming implementation.
#define M_DATASTREAM_FIELDS(X) \
    X(m_parent_item)\
    /*X(m_item_data)*/\
    X(m_child_items.size())

#define X(field) << obj.field
QTH_DEFINE_QDEBUG_OP(AbstractTreeModelItem,
							 M_DATASTREAM_FIELDS(X)
                     );
#undef X

AbstractTreeModelItem* AbstractTreeModelItem::child(int number)
{
	if(number >= childCount())
	{
		qWr() << "### CHILD INDEX OUT OF RANGE:" << number;
		Q_ASSERT(0);
		return nullptr;
	}

	/// @note .value() here returns a default constructed AbstractTreeModelItem which is not added to the QVector.
	/// @todo This seems all kinds of wrong, should probably return a nullptr or assert or something.
	return m_child_items[number].get();
}

const AbstractTreeModelItem* AbstractTreeModelItem::child(int number) const
{
	if(number >= childCount())
	{
		qWr() << "### CHILD INDEX OUT OF RANGE:" << number;
		Q_ASSERT(0);
		return nullptr;
	}
	return m_child_items[number].get();
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
//		return stdex::indexOf(m_parent_item->m_child_items, this);
		auto iter = std::find_if(m_parent_item->m_child_items.cbegin(), m_parent_item->m_child_items.cend(),
				[=](const auto& unptr){ return unptr.get() == this; });
		return iter - m_parent_item->m_child_items.cbegin();
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
		qWr() << "INVALID INSERT POSITION:" << position << ", balking.";
        return false;
	}

	decltype(m_child_items)::iterator pos_iterator = m_child_items.begin() + position;

	for (int row = 0; row < count; ++row)
	{
//        QVector<QVariant> data(columns);
//		AbstractTreeModelItem *item = new AbstractTreeModelItem(data, this);
		// Create a new default-constructed item.
		std::unique_ptr<AbstractTreeModelItem> item = std::move(create_default_constructed_child_item(this, columns));
		m_child_items.insert(pos_iterator, std::move(item));
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
	for(auto& child : m_child_items)
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
		qCr() << "out of bounds:" << position << count;
        return false;
	}

	if(count == 0)
	{
		qWr() << "Attempt to remove zero children";
		return false;
	}

	auto start = m_child_items.begin()+position;
	auto end = m_child_items.begin()+position+count-1;
	m_child_items.erase(start, end);

//	for (int row = 0; row < count; ++row)
//	{
//		// Remove and delete the child.
//		auto child = stdex::takeAt(m_child_items, position);
//		child.release();
//	}

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
	for(auto& child : m_child_items)
	{
        child->removeColumns(position, columns);
	}

    return true;
}

QVariant AbstractTreeModelItem::data(int column, int role) const
{
	// Color invalid model indexes.
	if(column > columnCount())
	{
		switch(role)
		{
			case Qt::ItemDataRole::BackgroundRole:
				return QVariant::fromValue(QBrush(Qt::lightGray));
				break;
			default:
				break;
		}
	}
	return QVariant();
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

bool AbstractTreeModelItem::appendChildren(std::vector<std::shared_ptr<AbstractTreeModelItem>> new_children)
{
    /// @todo Support adding new columns if children have them?
    for(auto& child : new_children)
    {
        child->setParentItem(this);
        m_child_items.emplace_back(std::move(child));
    }

	return true;
}

bool AbstractTreeModelItem::appendChild(std::shared_ptr<AbstractTreeModelItem> new_child)
{
M_WARNING("TODO");
	std::vector<std::shared_ptr<AbstractTreeModelItem>> new_children;

	new_children.emplace_back(std::move(new_child));

	return appendChildren(std::move(new_children));
}

void AbstractTreeModelItem::setParentItem(AbstractTreeModelItem *new_parent_item)
{
	// Can't reparent root item for now.
	Q_ASSERT(!m_is_root);
	Q_ASSERT(m_parent_item != nullptr);
//    AMLM_WARNIF(m_parent_item != nullptr);
//	AMLM_WARNIF(m_parent_item->columnCount() != this->columnCount());

	auto* old_parent_item = m_parent_item;
	if(old_parent_item != nullptr)
	{
		/// @todo Should convert over to id or smart ptr here.
		old_parent_item->removeChildren(childNumber(), 1);
	}

	if(new_parent_item != nullptr)
	{
		new_parent_item->appendChild(shared_from_this());
		m_parent_item = new_parent_item;
	}
}

std::unique_ptr<AbstractTreeModelItem>
AbstractTreeModelItem::create_default_constructed_child_item(AbstractTreeModelItem* parent, int num_columns)
{
	return std::unique_ptr<AbstractTreeModelItem>(this->do_create_default_constructed_child_item(parent, num_columns));
}

