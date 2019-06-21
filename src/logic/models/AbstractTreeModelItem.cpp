/*
 * Copyright 2018, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
 *
 * This file is part of AwesomeMediaLibraryManager.
 *
 * AwesomeMediaLibraryManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AwesomeMediaLibraryManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AwesomeMediaLibraryManager.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file AbstractTreeModelItem.cpp
 * Implementation of AbstractTreeModelItem.
 *
 * This class is heavily adapted from at least the following:
 * - The "Editable Tree Model Example" shipped with Qt5.
 * - KDenLive's TreeItem class.
 * - My own original work.
 * - Hundreds of nuggets of information from all over the Internet.
 */

#include "AbstractTreeModelItem.h"

// Std C++
#include <memory>
//#include <execution>

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

std::shared_ptr<AbstractTreeModelItem> AbstractTreeModelItem::construct(const QVector<QVariant>& data,
		std::shared_ptr<AbstractTreeModel> model, bool isRoot, UUIncD id)
{
	/// @note make_shared doesn't have access to the constructor if it's protected, so we have to do this.
	std::shared_ptr<AbstractTreeModelItem> self(new AbstractTreeModelItem(data, model, isRoot, id));
	baseFinishConstruct(self);
	return self;
}

AbstractTreeModelItem::AbstractTreeModelItem(const QVector<QVariant>& data, const std::shared_ptr<AbstractTreeModel>& model, bool is_root, UUIncD id)
	: m_model(model), m_depth(0), m_uuincid(id == UUIncD::null() ? UUIncD::create() : id),
	  m_is_in_model(false), m_is_root(is_root)
{
}

AbstractTreeModelItem::~AbstractTreeModelItem()
{
	deregisterSelf();
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

std::shared_ptr<AbstractTreeModelItem> AbstractTreeModelItem::child(int row) const
{
	Q_ASSERT_X(row >= 0 && row < m_child_items.size(), __func__, "Child row out of range.");

	auto it = m_child_items.cbegin();
	std::advance(it, row);

	return *it;
}

int AbstractTreeModelItem::childCount() const
{
	return m_child_items.size();
}

int AbstractTreeModelItem::columnCount() const
{
	M_TODO("KDEN returns the length of the QVariant vector here.");
	return 0;
}

/**
 * Find our index in the parent's child list.
 */
int AbstractTreeModelItem::childNumber() const
{
	if (auto shpt = m_parent_item.lock())
	{
//		auto iter = std::find_if(shpt->m_child_items.cbegin(), shpt->m_child_items.cend(),
//				[=](const auto& unptr){ return unptr.get() == this; });
//		return iter - shpt->m_child_items.cbegin();
		// We compute the distance in the parent's children list
		auto it = shpt->m_child_items.begin();
		return (int)std::distance(it, (decltype(it))shpt->get_m_child_items_iterator(m_uuincid));
	}
	else
	{
		/// @note Expired parent item. KDenLive doesn't do this.
		Q_ASSERT(0);
	}

    return -1;
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

#if 0 /// 1
bool AbstractTreeModelItem::insertChildren(int position, int count, int columns)
{
	AMLM_WARNIF(1);
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
#endif

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

//std::weak_ptr<AbstractTreeModelItem> AbstractTreeModelItem::parent()
//{
//	return m_parent_item;
//}

std::weak_ptr<AbstractTreeModelItem> AbstractTreeModelItem::parent_item() const
{
	return m_parent_item;
}

int AbstractTreeModelItem::depth() const
{
	return m_depth;
}

UUIncD AbstractTreeModelItem::getId() const
{
	Q_ASSERT(m_uuincid != UUIncD::null());
	return m_uuincid;
}

bool AbstractTreeModelItem::isInModel() const
{
	return m_is_in_model;
}

bool AbstractTreeModelItem::removeChildren(int position, int count)
{
	/// @todo
	Q_ASSERT(0);
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

void AbstractTreeModelItem::removeChild(const std::shared_ptr<AbstractTreeModelItem>& child)
{
	if (auto ptr = m_model.lock())
	{
		ptr->notifyRowAboutToDelete(shared_from_this(), child->childNumber());
		// get iterator corresponding to child
		auto it = get_m_child_items_iterator(child->getId());
		Q_ASSERT(it != m_child_items.end());
//		Q_ASSERT(m_iteratorTable.count(child->getId()) > 0);
//		auto it = m_iteratorTable[child->getId()];
		// deletion of child
		m_child_items.erase(it);
		// clean iterator table
//		m_iteratorTable.erase(child->getId());
		child->m_depth = 0;
		child->m_parent_item.reset();
		child->deregisterSelf();
		ptr->notifyRowDeleted();
	}
	else
	{
		qDebug() << "ERROR: Something went wrong when removing child in TreeItem. Model is not available anymore";
		Q_ASSERT(false);
	}
}

bool AbstractTreeModelItem::changeParent(std::shared_ptr<AbstractTreeModelItem> newParent)
{
	Q_ASSERT(!m_is_root);
	if (m_is_root)
	{
		return false;
	}
	std::shared_ptr<AbstractTreeModelItem> oldParent;
	if ((oldParent = m_parent_item.lock()))
	{
		oldParent->removeChild(shared_from_this());
	}
	bool res = true;
	if (newParent)
	{
		res = newParent->appendChild(shared_from_this());
		if (res)
		{
			m_parent_item = newParent;
		}
		else if (oldParent)
		{
			// something went wrong, we have to reset the parent.
			bool reverse = oldParent->appendChild(shared_from_this());
			Q_ASSERT(reverse);
		}
	}
	return res;
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

/// NEW: Return the QVariant in @a column.
/// KDen behavior is to return def const QVariant if > num cols.
QVariant AbstractTreeModelItem::dataColumn(int column) const
{
	return data(column);
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
		bool retval = appendChild(child);
		if(!retval)
		{
			/// @todo Recovery?
			Q_ASSERT(0);
			return false;
		}
    }

	return true;
}

bool AbstractTreeModelItem::appendChild(const std::shared_ptr<AbstractTreeModelItem>& new_child)
{
	if(has_ancestor(new_child->getId()))
	{
		// Somehow trying to create a cycle in the tree.
		return false;
	}
	if (auto oldParent = new_child->parent_item().lock())
	{
		if (oldParent->getId() == m_uuincid)
		{
			// new_child has us as current parent, no change needed.
			return true;
		}
		else
		{
			// in that case a call to removeChild should have been carried out
			qDebug() << "ERROR: trying to append a child that already has a parent";
			Q_ASSERT(0);
			return false;
		}
	}
	if (auto ptr = m_model.lock())
	{
		ptr->notifyRowAboutToAppend(shared_from_this());
		new_child->updateParent(shared_from_this());
		int id = new_child->getId();
		auto it = m_child_items.insert(m_child_items.end(), new_child);
//		m_iteratorTable[id] = it;
		registerSelf(new_child);
		ptr->notifyRowAppended(new_child);
		return true;
	}
	qDebug() << "ERROR: Something went wrong when appending child in TreeItem. Model is not available anymore";
	Q_ASSERT(false);
	return false;
}

/**
 * Append a child item from data.
 */
std::shared_ptr<AbstractTreeModelItem> AbstractTreeModelItem::appendChild(const QVector<QVariant>& data)
{
	if (auto ptr = m_model.lock())
	{
		auto child = construct(data, ptr, false);
		appendChild(child);
		return child;
	}
	qDebug() << "ERROR: Something went wrong when appending child to AbstractTreeModelItem. Model is not available anymore";
	Q_ASSERT(false);
	return std::shared_ptr<AbstractTreeModelItem>();
}

void AbstractTreeModelItem::moveChild(int ix, const std::shared_ptr<AbstractTreeModelItem>& child)
{
	if (auto ptr = m_model.lock())
	{
		auto parentPtr = child->m_parent_item.lock();
		if (parentPtr && parentPtr->getId() != m_uuincid)
		{
			parentPtr->removeChild(child);
		}
		else
		{
			// deletion of child
//			auto it = m_iteratorTable[child->getId()];
			auto it = get_m_child_items_iterator(child->getId());
			m_child_items.erase(it);
		}
		ptr->notifyRowAboutToAppend(shared_from_this());
		child->updateParent(shared_from_this());
		int id = child->getId();
		auto pos = m_child_items.begin();
		std::advance(pos, ix);
		auto it = m_child_items.insert(pos, child);
//		m_iteratorTable[id] = it;
		ptr->notifyRowAppended(child);
		m_is_in_model = true;
	}
	else
	{
		qDebug() << "ERROR: Something went wrong when moving child in AbstractTreeModelItem. Model is not available anymore";
		Q_ASSERT(false);
	}
}

bool AbstractTreeModelItem::has_ancestor(UUIncD id)
{
	if(m_uuincid == id)
	{
		// We're our own ancestor.
		return true;
	}
	if(auto ptr = m_parent_item.lock())
	{
		// We have a parent, recurse into it for the answer.
		/// @note It's nice to have a lot of stack sometimes, isn't it?
		return ptr->has_ancestor(id);
	}
	return false;
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
		qDebug() << "Error : construction of AbstractTreeModelItem failed because parent model is not available anymore";
		Q_ASSERT(false);
	}
}

void AbstractTreeModelItem::deregisterSelf()
{
	// Deregister our child items.
	for (const auto &child : m_child_items)
	{
		child->deregisterSelf();
	}
	if (m_is_in_model)
	{
		// We're in a model, deregister ourself from it.
		if (auto ptr = m_model.lock())
		{
			ptr->deregister_item(m_uuincid, this);
			m_is_in_model = false;
		}
	}
}

void AbstractTreeModelItem::updateParent(std::shared_ptr<AbstractTreeModelItem> parent)
{
	m_parent_item = parent;
	if(parent)
	{
		m_depth = parent->m_depth + 1;
	}
}

AbstractTreeModelItem::CICTIteratorType AbstractTreeModelItem::get_m_child_items_iterator(UUIncD id)
{
	CICTIteratorType retval;
	retval = std::find_if(/*std::execution::par,*/ m_child_items.begin(), m_child_items.end(), [id](auto& val){ return val->m_uuincid == id; });
	return retval;
}

