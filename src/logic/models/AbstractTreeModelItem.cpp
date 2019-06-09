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

// Std C++
#include <memory>

// Qt5
#include <QBrush>
#include <QStringList>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

// Ours
#include <utils/DebugHelpers.h>
#include <utils/VectorHelpers.h>
#include <logic/UUIncD.h>
#include "AbstractTreeModel.h"

std::shared_ptr<AbstractTreeModelItem> AbstractTreeModelItem::construct(std::shared_ptr<AbstractTreeModel> model, bool isRoot, UUIncD id)
{
	std::shared_ptr<AbstractTreeModelItem> self(new AbstractTreeModelItem(model, isRoot, id));
	baseFinishConstruct(self);
	return self;
}

AbstractTreeModelItem::AbstractTreeModelItem(const std::shared_ptr<AbstractTreeModel>& model, bool is_root, UUIncD id)
	: m_model(model), m_depth(0), m_uuincid(id == UUIncD::null() ? UUIncD::create() : id),
	  m_is_in_model(false), m_is_root(is_root)
{
}

AbstractTreeModelItem::~AbstractTreeModelItem()
{
	deregisterSelf();
#if 0 /// OBSOLETE
	// Doesn't remove child items, just deletes them.
//	qDeleteAll(m_child_items);
M_WARNING("FIXME: Both these are virtual calls");
	if(childCount() > 0)
	{
		// Remove and delete all children.
		removeChildren(0, childCount());
	}
#endif
}

/// Debug streaming implementation.
#define M_DATASTREAM_FIELDS(X) \
	X(m_parent_item.lock())\
    /*X(m_item_data)*/\
    X(m_child_items.size())

//#define X(field) << obj.field
//QTH_DEFINE_QDEBUG_OP(AbstractTreeModelItem,
//							 M_DATASTREAM_FIELDS(X)
//                     );
//#undef X
QDebug operator<<(QDebug dbg, const AbstractTreeModelItem& obj)
{
	QDebugStateSaver saver(dbg);
	dbg << "AbstractTreeModelItem (" << M_ID_VAL(*obj.m_parent_item.lock().get()) << M_ID_VAL(obj.m_child_items.size()) << ")";
	return dbg;
}

std::shared_ptr<AbstractTreeModelItem> AbstractTreeModelItem::child(int row)
{
	Q_ASSERT_X(row >= 0 && row < m_child_items.size(), __func__, "Child row out of range.");

	auto it = m_child_items.cbegin();
	std::advance(it, row);

	/// @note .value() here returns a default constructed AbstractTreeModelItem which is not added to the QVector.
	/// @todo This seems all kinds of wrong, should probably return a nullptr or assert or something.
	return *it;
}

const std::shared_ptr<AbstractTreeModelItem> AbstractTreeModelItem::child(int row) const
{
	return child(row);
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
	if (auto shpt = m_parent_item.lock())
	{
//		return stdex::indexOf(m_parent_item->m_child_items, this);
		auto iter = std::find_if(shpt->m_child_items.cbegin(), shpt->m_child_items.cend(),
				[=](const auto& unptr){ return unptr.get() == this; });
		return iter - shpt->m_child_items.cbegin();
	}
	else
	{
		/// @note Expired parent item.
		Q_ASSERT(0);
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

std::weak_ptr<AbstractTreeModelItem> AbstractTreeModelItem::parent()
{
	return m_parent_item;
}

const std::weak_ptr<AbstractTreeModelItem> AbstractTreeModelItem::parent() const
{
	return m_parent_item;
}

UUIncD AbstractTreeModelItem::getId() const
{
	return m_uuincid;
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
    Q_ASSERT(0);
//    if(auto ptr = m_model.lock())
//    {
//    	// We still have a model.
//    	auto child
//    }
    for(auto& child : new_children)
    {
		child->setParentItem(this->shared_from_this());
        m_child_items.emplace_back(std::move(child));
    }

	return true;
}

bool AbstractTreeModelItem::appendChild(std::shared_ptr<AbstractTreeModelItem> new_child)
{
#if 0
	std::vector<std::shared_ptr<AbstractTreeModelItem>> new_children;

	new_children.emplace_back(std::move(new_child));

	return appendChildren(std::move(new_children));
#endif

	if(hasAncestor(new_child->getId())
	{
		// Somehow trying to create a cycle in the tree.
		return false;
	}
	if (auto oldParent = child->parentItem().lock())
	{
		if (oldParent->getId() == m_id)
		{
			// no change needed
			return true;
		}
		else
		{
			// in that case a call to removeChild should have been carried out
			qDebug() << "ERROR: trying to append a child that already has a parent";
			return false;
		}
	}
	if (auto ptr = m_model.lock())
	{
		ptr->notifyRowAboutToAppend(shared_from_this());
		child->updateParent(shared_from_this());
		int id = child->getId();
		auto it = m_childItems.insert(m_childItems.end(), child);
		m_iteratorTable[id] = it;
		registerSelf(child);
		ptr->notifyRowAppended(child);
		return true;
	}
	qDebug() << "ERROR: Something went wrong when appending child in TreeItem. Model is not available anymore";
	Q_ASSERT(false);
	return false;
}

/// Append a child item from data.
/// @todo
std::shared_ptr<AbstractTreeModelItem> AbstractTreeModelItem::appendChild(const QVector<QVariant>& data)
{
	if (auto ptr = m_model.lock())
	{
		auto child = construct(ptr, false);
		appendChild(child);
		return child;
	}
	qDebug() << "ERROR: Something went wrong when appending child in TreeItem. Model is not available anymore";
	Q_ASSERT(false);
	return std::shared_ptr<AbstractTreeModelItem>();
}

bool AbstractTreeModelItem::has_ancestor(UUIncD id)
{
	if(m_uuincid == id)
	{
		// We're our own ancestor.
		return true;
	}

}


void AbstractTreeModelItem::baseFinishConstruct(const std::shared_ptr<AbstractTreeModelItem>& self)
{
	if(self->m_is_root)
	{
		registerSelf(self);
	}
}

void AbstractTreeModelItem::registerSelf(const std::shared_ptr<AbstractTreeModelItem>& self)
{
	// Register children.
	for (const auto& child : self->m_child_items)
	{
		registerSelf(child);
	}
	// If we still have a model, register with it.
	if (auto ptr = self->m_model.lock())
	{
		ptr->register_item(self);
		self->m_is_in_model = true;
	}
	else
	{
		qDebug() << "Error : construction of treeItem failed because parent model is not available anymore";
		Q_ASSERT(false);
	}
}

void AbstractTreeModelItem::deregisterSelf()
{
	for (const auto &child : m_child_items)
	{
		child->deregisterSelf();
	}
	if (m_is_in_model)
	{
		if (auto ptr = m_model.lock())
		{
			ptr->deregister_item(m_uuincid, this);
			m_is_in_model = false;
		}
	}
}

void AbstractTreeModelItem::setParentItem(std::shared_ptr<AbstractTreeModelItem> parent_item)
{
//	Q_ASSERT(parent_item != nullptr);
	AMLM_WARNIF(m_parent_item.expired());
//	AMLM_WARNIF(m_parent_item->columnCount() != this->columnCount());

	m_parent_item = parent_item;
}

std::unique_ptr<AbstractTreeModelItem>
AbstractTreeModelItem::create_default_constructed_child_item(AbstractTreeModelItem* parent, int num_columns)
{
	return std::unique_ptr<AbstractTreeModelItem>(this->do_create_default_constructed_child_item(parent, num_columns));
}

