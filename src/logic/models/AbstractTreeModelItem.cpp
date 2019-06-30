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

// Qt5
#include <QBrush>
#include <QStringList>
//#include <QXmlStreamWriter>
//#include <QXmlStreamReader>

// Ours
#include <utils/DebugHelpers.h>
#include <utils/VectorHelpers.h>
#include <logic/UUIncD.h>
#include "AbstractTreeModel.h"
//#include "PlaceholderTreeModelItem.h"
#include <utils/ext_iterators.h>

std::shared_ptr<AbstractTreeModelItem> AbstractTreeModelItem::construct(const std::vector<QVariant>& data,
		std::shared_ptr<AbstractTreeModel> model, bool isRoot, UUIncD id)
{
	/// @note make_shared doesn't have access to the constructor if it's protected, so we have to do this.
	std::shared_ptr<AbstractTreeModelItem> self(new AbstractTreeModelItem(data, model, isRoot, id));
	baseFinishConstruct(self);
	return self;
}

AbstractTreeModelItem::AbstractTreeModelItem(const std::vector<QVariant>& data,
		const std::shared_ptr<AbstractTreeModel>& model, bool is_root, UUIncD id)
	: m_item_data(data), m_model(model), m_depth(0), m_uuincid(id == UUIncD::null() ? UUIncD::create() : id),
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
#if 0 /// @todo Move to model.
	// Do this the slow, painful way.
	/// @todo Add some caching.
	int max_columns = 0;
	for(auto it : m_child_items)
	{
		int child_column_max = it->columnCount();
		max_columns = std::max(max_columns, child_column_max);
	}

	return max_columns;
#endif
	return m_item_data.size();
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

bool AbstractTreeModelItem::insertColumns(int insert_before_column, int num_columns)
{
	if (insert_before_column < 0 || insert_before_column > m_item_data.size())
	{
		// Check if we're out of bounds.
		/// @todo Probably assert here?
        return false;
	}

	// Insert new columns in this.
//	bool success = derivedClassInsertColumns(insert_before_column, num_columns);
	for (int column = 0; column < num_columns; ++column)
	{
		m_item_data.insert(m_item_data.begin()+insert_before_column, QVariant());
	}

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

std::weak_ptr<AbstractTreeModelItem> AbstractTreeModelItem::parent() const
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

bool AbstractTreeModelItem::operator==(const AbstractTreeModelItem& other) const
{
	return m_uuincid == other.m_uuincid;
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
		qCr() << "ERROR: Something went wrong when removing child in TreeItem. Model is not available anymore";
		Q_ASSERT(false);
	}
}

AbstractTreeModelItem::bfs_iterator AbstractTreeModelItem::begin_bfs()
{
	Q_ASSERT(0);
	return bfs_iterator(shared_from_this());
}

AbstractTreeModelItem::bfs_iterator AbstractTreeModelItem::end_bfs()
{
	Q_ASSERT(0);
	return bfs_iterator();
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

//#define M_DATASTREAM_FIELDS(X) \
//	X(XMLTAG_NUM_COLUMNS, dummy)
//using strviw_type = QLatin1Literal;

///// Strings to use for the tags.
//#define X(field_tag, member_field) static const strviw_type field_tag ( # member_field );
//	M_DATASTREAM_FIELDS(X);
//#undef X

static const QLatin1Literal XMLTAG_NUM_COLUMNS("num_columns");
static const QLatin1Literal XMLTAG_ITEM_DATA_SIZE("item_data_size");
static const QLatin1Literal XMLTAG_NUM_CHILDREN("num_children");

QVariant AbstractTreeModelItem::toVariant() const
{
	QVariantInsertionOrderedMap map;

	// Number of elements in the std::vector<QVariant>.
	map_insert_or_die(map, XMLTAG_ITEM_DATA_SIZE, QVariant::fromValue<qulonglong>(m_item_data.size()));
	// Number of immediate children.
	map_insert_or_die(map, XMLTAG_NUM_CHILDREN, QVariant::fromValue<qulonglong>(m_child_items.size()));

	/// @todo The "m_item_data" string is not getting written out, not sure if we care.
	QVariantHomogenousList list("m_item_data", "item");
	// The item data itself.
	for(const QVariant& itemdata : m_item_data)
	{
		list_push_back_or_die(list, itemdata);
	}
	// Add them to the output map.
	map_insert_or_die(map, "item_data", list);

	// Child nodes.
	// Create a list of them.
	QVariantHomogenousList vl("children", "child");
	for(int i=0; i<childCount(); i++)
	{
		auto child_ptr = child(i);
		list_push_back_or_die(vl, child_ptr->toVariant());
	}
	// Insert the list into the map.
	map.insert("children", vl);

	return map;

#if 0
	QVariantInsertionOrderedMap map;

#define X(field_tag, member_field) map_insert_or_die(map, field_tag, member_field);
	M_DATASTREAM_FIELDS(X);
#undef X

	// Children to variant list.
	QVariantHomogenousList vl("children", "child");
	for(int i=0; i<childCount(); i++)
	{
		auto child_ptr = child(i);
//		list_push_back_or_die(vl, child_ptr);
		vl.push_back(child_ptr->toVariant());
	}
	map.insert("children", vl);

	return map;
#endif
}

void AbstractTreeModelItem::fromVariant(const QVariant& variant)
{
	QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();

	// Get the number of item_data entries.
	std::vector<QVariant>::size_type item_data_size = 0;
	map_read_field_or_warn(map, XMLTAG_ITEM_DATA_SIZE, &item_data_size);

	// Children to variant list.
	QVariantHomogenousList vl("itemdata_list", "m_item_data");
	map_read_field_or_warn(map, "item_data", &vl);
	for(const auto& it : vl)
	{
		QString itstr = it.toString();
		m_item_data.push_back(itstr);
	}
}


bool AbstractTreeModelItem::removeColumns(int position, int columns)
{
	// Check that the range is within our number of real columns.
	if (position < 0 || position + columns > m_item_data.size())
	{
		return false;
	}

	/// @note Don't need a remove() here, items will be deleted.
	m_item_data.erase(m_item_data.begin()+position, m_item_data.begin()+position+columns);

	// Remove columns from all children.
	for(auto& child : m_child_items)
	{
        child->removeColumns(position, columns);
	}

    return true;
}

QVariant AbstractTreeModelItem::data(int column, int role) const
{
	// Color model indexes with a column beyond what we have data for.
	if(column < 0 || column >= m_item_data.size())
	{
		switch(role)
		{
			case Qt::ItemDataRole::BackgroundRole:
				return QVariant::fromValue(QBrush(Qt::red /*lightGray*/));
				break;
			default:
				break;
		}
	}
	else if(role == Qt::ItemDataRole::DisplayRole || role == Qt::ItemDataRole::EditRole)
	{
		return m_item_data.at(column);
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
#if 0 // NOT NEW
	auto current_num_columns = columnCount();

	if (column < 0 || column >= current_num_columns)
	{
        return false;
	}
	return derivedClassSetData(column, value);
#else // New
	/// Not new.
	if (column < 0 || column >= m_item_data.size())
	{
		qWr() << "Column Out Of Range:" << column;
		return false;
	}
	/// @note KDenLive just does this here.
	m_item_data[column] = value;
	return true;
#endif
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

bool AbstractTreeModelItem::insertChildren(int position, int count, int columns)
{
	if (position < 0 || position > m_child_items.size())
	{
		// Insertion point out of range of existing children.
		qWr() << "INVALID INSERT POSITION:" << position << ", balking.";
		return false;
	}

	// No ancestor cycle or existing parent check needed, rows will be new.

	if(auto model_ptr = m_model.lock())
	{
		// Currently model handles the notifications in insertRows().
//		ptr->notifyRowsAboutToInsert()

		decltype(m_child_items)::iterator pos_iterator = m_child_items.begin() + position;

		for (int row = 0; row < count; ++row)
		{
			std::vector<QVariant> data(columns);

			// Create a new default-constructed item.
//			std::shared_ptr<PlaceholderTreeModelItem> item = PlaceholderTreeModelItem::construct(data, model_ptr);
			std::shared_ptr<AbstractTreeModelItem> item = AbstractTreeModelItem::construct(data, model_ptr, false);
			// Set us as the new item's parent.
			item->updateParent(shared_from_this());
			UUIncD id = item->getId();
			Q_ASSERT(id != UUIncD::null());
			pos_iterator = m_child_items.insert(pos_iterator, item);
			Q_ASSERT(pos_iterator != m_child_items.end());
			registerSelf(item);
			++pos_iterator;
		}

//		ptr->notifyRowsInserted();
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
	if (auto oldParent = new_child->parent().lock())
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

/// Append a child item created from @a data.
/// @todo
std::shared_ptr<AbstractTreeModelItem> AbstractTreeModelItem::appendChild(const std::vector<QVariant>& data)
{
	if (auto ptr = m_model.lock())
	{
		auto child = AbstractTreeModelItem::construct(data, ptr, false);
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

/**
 * Static function which registers @a self and its children with the model self is already registered with.
 * @warning Will assert if @a self doesn't already know its model.
 * @param self
 */
void AbstractTreeModelItem::registerSelf(const std::shared_ptr<AbstractTreeModelItem>& self)
{
	Q_ASSERT(!self->m_model.expired());

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
		// Keep depth up to date.
		m_depth = parent->m_depth + 1;
		// Keep max column count up to date.
		/// @todo
//		m_num_parent_columns = parent->columnCount();
	}
}

AbstractTreeModelItem::CICTIteratorType AbstractTreeModelItem::get_m_child_items_iterator(UUIncD id)
{
	CICTIteratorType retval;
	retval = std::find_if(m_child_items.begin(), m_child_items.end(), [id](auto& val){ return val->m_uuincid == id; });
	return retval;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///
/// AbstractTreeModelItem::bfs_iterator
///

AbstractTreeModelItem::bfs_iterator::bfs_iterator() { }

AbstractTreeModelItem::bfs_iterator::bfs_iterator(std::shared_ptr<AbstractTreeModelItem> root_node)
	: m_root_node(root_node), m_current_node(root_node),
	  m_child_list_it(root_node->m_child_items.begin())
{
	m_child_bfs_it = std::make_shared<bfs_iterator>(root_node->begin_bfs());
}

AbstractTreeModelItem::bfs_iterator AbstractTreeModelItem::bfs_iterator::operator++(int)
{
	auto retval = *this;
	++(*this);
	return retval;
}

bool AbstractTreeModelItem::bfs_iterator::operator==(const AbstractTreeModelItem::bfs_iterator& other) const
{
	return (m_current_node == other.m_current_node);
}

bool AbstractTreeModelItem::bfs_iterator::operator!=(const AbstractTreeModelItem::bfs_iterator& other) const
{
	return !(*this == other);
}

AbstractTreeModelItem::bfs_iterator::reference AbstractTreeModelItem::bfs_iterator::operator*() const
{
	return *m_current_node;
}

AbstractTreeModelItem::bfs_iterator& AbstractTreeModelItem::bfs_iterator::operator++()
{
	// Steps of a DFS at each node:
	// Perform pre-order operation.
	// For each i from 1 to the number of children do:
	//     Visit i-th, if present.
	//     Perform in-order operation.
	// Perform post-order operation.

	// Are we already at the end?
	if(m_current_node == nullptr || m_is_at_end)
	{
		// end() iterator doesn't increment.
		return *this;
	}

	/// Preorder return here?

	// Lock our weak parent ptr.  We should have a parent unless we're the true root.
//	auto parent = m_current_node->parent_item().lock();

//	if(parent == nullptr || parent == m_root_node) /// Handle no-parent differently?
//	{
//		// We hit the node we started at on the way up, next state is end().

//		/// Post-order return here?

//		m_current_node = nullptr;
//		m_is_at_end = true;
//		return *this;
//	}

	// Else we should have a valid m_current_node and it's parent, which should be us?
	// So we visit all children of this node in-order.
//	m_current_node = *m_child_list_it;
	if(m_child_list_it == m_current_node->m_child_items.end())
	{
		// Reached the end of the current node's child list.
		// Now we go back to the parent of m_current_node.
		auto parent = m_current_node->parent_item().lock();
		if(parent != m_root_node)
		{
			m_current_node = m_current_node->parent_item().lock();
		}
	}
	else
	{
		// Still iterating over the child items.
		++m_child_list_it;
		// Recurse on this node as a new root node.
		(*m_child_bfs_it)++;
	}

	return *this;
}
