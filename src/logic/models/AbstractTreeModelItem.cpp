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
#include <utility>

// Qt5
#include <QBrush>
#include <QStringList>

// Ours
#include <utils/DebugHelpers.h>
#include <utils/VectorHelpers.h>
#include <logic/UUIncD.h>
#include "AbstractTreeModel.h"
#include <utils/ext_iterators.h>
#include <logic/serialization/SerializationHelpers.h>



AbstractTreeModelItem::AbstractTreeModelItem()
{
	// Just to get a vptr.
}

AbstractTreeModelItem::AbstractTreeModelItem(const std::vector<QVariant>& data, const std::shared_ptr<AbstractTreeModelItem>& parent_item, UUIncD id)
	: AbstractTreeModelItem()
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
	if(parent_item)
	{
		parent_item->appendChild(this->shared_from_this());
	}

	m_parent_item = parent_item;
    m_item_data = data;
}

AbstractTreeModelItem::~AbstractTreeModelItem()
{
	deregister_self();
}

void AbstractTreeModelItem::clear()
{
	// Reset this item to completely empty, except for its place in the model.
//	m_child_items.clear();
//	m_item_data.clear();
//	m_num_columns = 0;
//	m_num_parent_columns = -1;
}

bool AbstractTreeModelItem::selfSoftDelete(Fun& undo, Fun& redo)
{
	Fun local_undo = []() { return true; };
	Fun local_redo = []() { return true; };

	// Recursively "soft delete" child objects.
	for(const auto& child : m_child_items)
	{
		bool status = std::static_pointer_cast<AbstractTreeModelItem>(child)->selfSoftDelete(local_undo, local_redo);
		if(!status)
		{
			// something went wrong with the child soft delete, recusively back out.
			bool undone = local_undo();
			Q_ASSERT(undone);
			return false;
		}
	}
	qWr() << "###################### SELFSOFTDELETE()";
M_WARNING("TODO, NEEDS MUTEX MEMBER");
//	UPDATE_UNDO_REDO(m_rw_mutex, local_redo, local_undo, undo, redo);
	return true;
}

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

	return *it;
}

int AbstractTreeModelItem::childCount() const
{
	return m_child_items.size();
}

int AbstractTreeModelItem::columnCount() const
{
	return m_item_data.size();
}

/**
 * Find our index in the parent's child list.
 * KDEN calls this ::row().
 */
int AbstractTreeModelItem::childNumber() const
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
	if(auto par = m_parent_item.lock())
	{
		std::shared_ptr<AbstractTreeModelItem> this_cast = std::const_pointer_cast<AbstractTreeModelItem>(this->shared_from_this());
		auto cit = std::find(par->m_child_items.cbegin(), par->m_child_items.cend(), this_cast);
		if(cit != par->m_child_items.cend())
		{
			return std::distance(par->m_child_items.cbegin(), cit);
		}
		else
		{
			qCr() << "Can't find ourselves in parent's list";
			return -1;
		}
	}

	// No parent, ETM returns 0 here.
    return 0;
}


bool AbstractTreeModelItem::insertColumns(int insert_before_column, int num_columns)
{
	// Check if caller is trying to inser a column out of bounds.
	if (insert_before_column < 0 || insert_before_column > m_item_data.size())
	{
		qWr() << "Ignoring insertColumns() with bad insert_before_column:" << insert_before_column;
        return false;
	}

	// Insert new, empty columns in this.
	for (int column = 0; column < num_columns; ++column)
	{
		m_item_data.insert(m_item_data.begin() + insert_before_column, QVariant("???"));
	}

	// Propagate the column insertion to our children.
	for(auto& child : m_child_items)
	{
        child->insertColumns(insert_before_column, num_columns);
	}

    return true;
}

bool AbstractTreeModelItem::removeColumns(int position, int columns)
{
	// Check that the range is within our number of real columns.
	if (position < 0 || position + columns > m_item_data.size())
	{
		return false;
	}

	// Remove the columns from the m_item_data.
	// Erase works ~differently for vector<> and deque<>, we don't need a remove() here, items will be removed and deleted.
	m_item_data.erase(m_item_data.begin()+position, m_item_data.begin()+position+columns);

	// Recursively remove columns from all children.
	for(auto& child : m_child_items)
	{
		child->removeColumns(position, columns);
	}

	return true;
}

std::weak_ptr<AbstractTreeModelItem> AbstractTreeModelItem::parent_item() const
{
	return m_parent_item;
}

std::shared_ptr<AbstractTreeModelItem> AbstractTreeModelItem::parent() const
{
	return std::static_pointer_cast<AbstractTreeModelItem>(m_parent_item.lock());
}

//int AbstractTreeModelItem::depth() const
//{
//	return m_depth;
//}

UUIncD AbstractTreeModelItem::getId() const
{
	Q_ASSERT(m_uuincid != UUIncD::null());
	return m_uuincid;
}

void AbstractTreeModelItem::setId(UUIncD id)
{
	Q_ASSERT(m_uuincid != UUIncD::null());
	m_uuincid = id;
}

bool AbstractTreeModelItem::isInModel() const
{
	if(auto temp = m_model.lock())
	{
		return true;
	}
	else
	{
		return false;
	}
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

#if 1///
void AbstractTreeModelItem::removeChild(const std::shared_ptr<AbstractTreeModelItem>& child)
{
	if (auto ptr = m_model.lock())
	{
		ptr->notifyRowAboutToDelete(shared_from_this(), child->childNumber());
		// Get iterator corresponding to child
		auto it = get_m_child_items_iterator(child->getId());
		Q_ASSERT(it != m_child_items.end());
//		Q_ASSERT(m_iteratorTable.count(child->getId()) > 0);
//		auto it = m_iteratorTable[child->getId()];
		// Delete the child.
		m_child_items.erase(it);
		// clean iterator table
//		m_iteratorTable.erase(child->getId());
//		child->m_depth = 0;
		child->m_parent_item.reset();
		child->deregister_self();
		ptr->notifyRowDeleted();
	}
	else
	{
		qCr() << "ERROR: Something went wrong when removing child in TreeItem. Model is not available anymore";
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
#endif///

#define M_DATASTREAM_FIELDS(X) \
	/* TAG_IDENTIFIER, tag_string, member_field, var_name */ \
	X(XMLTAG_CHILD_NODE_LIST, child_node_list, nullptr)

#define M_DATASTREAM_FIELDS_CONTSIZES(X) \
	X(XMLTAG_NUM_COLUMNS, num_columns, m_item_data) \
	X(XMLTAG_ITEM_DATA_SIZE, item_data_size, m_item_data) \
	X(XMLTAG_NUM_CHILDREN, num_children, m_child_items)

using strviw_type = QLatin1String;

///// Strings to use for the tags.
#define X(field_tag, tag_string, var_name) static const strviw_type field_tag ( # tag_string );
	M_DATASTREAM_FIELDS(X);
	M_DATASTREAM_FIELDS_CONTSIZES(X);
#undef X


QVariant AbstractTreeModelItem::toVariant() const
{
	InsertionOrderedMap<QString, QVariant> map;

	// Write class info to the map.
	set_map_class_info(this, &map);

#define X(field_tag, tag_string, var_name) map_insert_or_die(map, field_tag, var_name);
	M_DATASTREAM_FIELDS(X);
#undef X
#define X(field_tag, tag_string, var_name) map_insert_or_die(map, field_tag, (qulonglong)(var_name).size());
	M_DATASTREAM_FIELDS_CONTSIZES(X);
#undef X

	// Number of elements in the std::vector<QVariant>.
//	map_insert_or_die(map, XMLTAG_ITEM_DATA_SIZE, QVariant::fromValue<qulonglong>(m_item_data.size()));
	// Number of immediate children.
//	map_insert_or_die(map, XMLTAG_NUM_CHILDREN, QVariant::fromValue<qulonglong>(m_child_items.size()));

	/// @todo The "m_item_data" string is not getting written out, not sure if we care.
	QVariantHomogenousList list("m_item_data", "item");
	// The item data itself.
	for(const QVariant& itemdata : m_item_data)
	{
		list_push_back_or_die(list, itemdata);
	}
	// Add them to the output map.
	map_insert_or_die(map, "item_data", list);

	// Serialize out Child nodes.
	/// @todo ???
//	auto child_list = childrenToVariant();

	// Insert the list into the map.
//	map_insert_or_die(map, XMLTAG_CHILD_NODE_LIST, child_list);

	return map;
}



void AbstractTreeModelItem::fromVariant(const QVariant& variant)
{
	InsertionOrderedMap<QString, QVariant> map = variant.value<InsertionOrderedMap<QString, QVariant>>();

#define X(field_tag, tag_string, member_field) map_read_field_or_warn(map, field_tag, member_field);
//	M_DATASTREAM_FIELDS(X);
#undef X

	// Get the number of item_data entries.
	std::vector<QVariant>::size_type item_data_size = 0;
	map_read_field_or_warn(map, XMLTAG_ITEM_DATA_SIZE, &item_data_size);

	// This item's data from variant list.
	QVariantHomogenousList vl("itemdata_list", "m_item_data");
	map_read_field_or_warn(map, "item_data", &vl);
	for(const auto& it : vl)
	{
		QString itstr = it.toString();
		m_item_data.push_back(itstr);
	}

	// Get this item's children.
	qulonglong num_children = 0;
	map_read_field_or_warn(map, XMLTAG_NUM_CHILDREN, &num_children);

	qDb() << XMLTAG_NUM_CHILDREN << num_children;


	QVariantHomogenousList child_list(XMLTAG_CHILD_NODE_LIST, "child");
	child_list = map.at(XMLTAG_CHILD_NODE_LIST).value<QVariantHomogenousList>();
	qDb() << M_ID_VAL(child_list.size());

	AMLM_ASSERT_EQ(num_children, child_list.size());

	// Read in our children.
	/// @todo ???
//	childrenFromVariant(child_list);

	////////////////////////////////////
#if 0
	// Now read in our children.  We need this Item to be in a model for that to work.
	Q_ASSERT(isInModel());

	auto model_ptr = m_model.lock();
	Q_ASSERT(model_ptr);

	// What was the derived class that was actually written?
	std::string metatype_class_str = map.get_attr("class");
	if(metatype_class_str.empty())
	{
		// Get as much info as we can.
		auto vartype = variant.type();
		const char* typename_per_var = variant.typeName();
		auto metatype = QMetaType::typeName(vartype);
		qDb() << "Class attr:" << M_ID_VAL(metatype) << M_ID_VAL(vartype) << M_ID_VAL(typename_per_var);
//		Q_ASSERT(0);
	}

	AMLM_ASSERT_EQ(num_children, m_child_items.size());
#endif///
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

std::vector<std::shared_ptr<AbstractTreeModelItem>> AbstractTreeModelItem::insertChildren(int position, int count, int columns)
{
	std::vector<std::shared_ptr<AbstractTreeModelItem>> retval;

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
		std::vector<QVariant> data(columns);
		std::shared_ptr<AbstractTreeModelItem> item = std::make_shared<AbstractTreeModelItem>(data, this->shared_from_this());
		m_child_items[position] = item;
		retval.push_back(item);
	}

	return retval;
}

void AbstractTreeModelItem::insertChild(int row, std::shared_ptr<AbstractTreeModelItem> item)
{
#if 1 /// AQP

	AMLM_ASSERT_X(!item->isInModel(), "TODO: ITEM ALREADY IN MODEL, MOVE ITEMS BETWEEN MODELS");
	AMLM_ASSERT_X(isInModel(), "TODO: PARENT ITEM NOT IN MODEL");

	item->m_parent_item = this->shared_from_this();

//	item->changeParent(this->shared_from_this());

	// Need an iterator to insert before.
	auto ins_it = m_child_items.begin();
	std::advance(ins_it, row);

	m_child_items.insert(ins_it, item);

	// If the parent is in a model, add the child item to the same model.
	if(auto model = m_model.lock())
	{
		item->m_model = m_model;
		register_self(item);
	}

	verify_post_add_ins_child(item);

#else // ETM
	M_WARNING("Something's wrong here, item is unused.");
	auto retval = insertChildren(row, 1, this->columnCount());
	return retval[0];
#endif
}

bool AbstractTreeModelItem::appendChildren(std::vector<std::shared_ptr<AbstractTreeModelItem>> new_children)
{
	/// @todo Support adding new columns if children have them?
	for(auto& child : new_children)
	{
		bool retval = appendChild(child);

		verify_post_add_ins_child(child);

		if(!retval)
		{
M_TODO("CRASHING HERE");/// @todo Recovery?
Q_ASSERT(0);
			return false;
		}
	}

	return true;
}

bool AbstractTreeModelItem::appendChild(const std::shared_ptr<AbstractTreeModelItem>& new_child)
{
#if 1///
	this->insertChild(childCount(), new_child);

	verify_post_add_ins_child(new_child);

	return true;
#else /// KDEN
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
			return false;
		}
	}
	if (auto ptr = m_model.lock())
	{
		std::shared_ptr<AbstractTreeModelItem> sft = shared_from_this();
		Q_ASSERT(sft);
		ptr->notifyRowAboutToAppend(shared_from_this());
		new_child->updateParent(shared_from_this());
		UUIncD id = new_child->getId();
		auto it = m_child_items.insert(m_child_items.end(), new_child);
//		m_iteratorTable[id] = it;
		register_self(new_child);
		ptr->notifyRowAppended(new_child);

		verify_post_add_ins_child(new_child);


		return true;
	}
	qDebug() << "ERROR: Something went wrong when appending child in TreeItem. Model is not available anymore";
	Q_ASSERT(false);
	return false;
#endif///
}

/// Append a child item created from @a data.
std::shared_ptr<AbstractTreeModelItem> AbstractTreeModelItem::appendChild(const std::vector<QVariant>& data)
{

Q_ASSERT(0);

#if 0///
	if (auto ptr = m_model.lock())
	{
		// Create the new child with this item's model as the model.
		// Not that by definition, this will not be the root item.
//		auto child = AbstractTreeModelItem::construct(data, ptr, false);
		auto child = AbstractTreeModelItem::construct(data, this->shared_from_this());
		appendChild(child);
		return child;
	}
	qDebug() << "ERROR: Something went wrong when appending child to AbstractTreeModelItem. Model is not available anymore";
	Q_ASSERT(false);
#endif///
	return std::shared_ptr<AbstractTreeModelItem>();
}

//void AbstractTreeModelItem::moveChild(int ix, const std::shared_ptr<AbstractTreeModelItem>& child)
//{
//	if (auto ptr = m_model.lock())
//	{
//		auto parentPtr = child->m_parent_item.lock();
//		if (parentPtr && parentPtr->getId() != m_uuincid)
//		{
//			parentPtr->removeChild(child);
//		}
//		else
//		{
//			// Deletion of child.
////			auto it = m_iteratorTable[child->getId()];
//			auto it = get_m_child_items_iterator(child->getId());
//			m_child_items.erase(it);
//		}
//		ptr->notifyRowAboutToAppend(shared_from_this());
//		child->updateParent(shared_from_this());
//		UUIncD id = child->getId();
//		auto pos = m_child_items.begin();
//		std::advance(pos, ix);
//		auto it = m_child_items.insert(pos, child);
////		m_iteratorTable[id] = it;
//		ptr->notifyRowAppended(child);
//		m_is_in_model = true;
//	}
//	else
//	{
//		qDebug() << "ERROR: Something went wrong when moving child in AbstractTreeModelItem. Model is not available anymore";
//		Q_ASSERT(false);
//	}
//}

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

bool AbstractTreeModelItem::isRoot() const
{
	return m_is_root;
}

void AbstractTreeModelItem::verify_post_add_ins_child(const std::shared_ptr<AbstractTreeModelItem>& inserted_child)
{
	AMLM_ASSERT_X(inserted_child->has_ancestor(m_uuincid), "UUID of the parent not an ancestor of child.");
	auto child_locked_par_item = inserted_child->m_parent_item.lock();
	AMLM_ASSERT_X(child_locked_par_item == this->shared_from_this(), "CHILD'S PARENT IS NOT THIS");
	// If parent is in a model, child is in the same model.
	auto child_par_model = child_locked_par_item->m_model.lock();
	if(isInModel())
	{
		AMLM_ASSERT_X(inserted_child->isInModel(), "PARENT IN MODEL, CHILD ISN'T");
		AMLM_ASSERT_X(child_par_model == m_model.lock(), "PARENT AND CHILD ARE IN DIFFERENT MODELS");
	}
	else
	{
		AMLM_ASSERT_X(!child_par_model, "This is not in a model but child is");
	}
}

/**
 * Static function which registers @a self and its children with the model @a self is already registered with.
 * @warning Will assert if @a self doesn't already know its model.
 * @param self
 */
void AbstractTreeModelItem::register_self(const std::shared_ptr<AbstractTreeModelItem>& self)
{
//	Q_ASSERT(self->m_model);
//	Q_ASSERT(!self->m_model.expired());

	// Register children.
	for (const auto& child : self->m_child_items)
	{
		register_self(child);
	}
	// If we still have a model, register with it.
	if (auto ptr = self->m_model.lock())
	{
		ptr->register_item(self);
//		self->isInModel() = true;
		AMLM_ASSERT_EQ(self->isInModel(), true);
	}
	else
	{
		qWr() << "COULDN'T LOCK MODEL:";// << M_ID_VAL(self->m_model);// << M_ID_VAL(self->m_model);
		AMLM_ASSERT_X(false, "Error : construction of AbstractTreeModelItem failed because parent model is not available anymore");
	}
}

void AbstractTreeModelItem::deregister_self()
{
	// Deregister our child items.
	for (const auto &child : m_child_items)
	{
		child->deregister_self();
	}
	// Potentially deregister ourself.
	if (isInModel())
	{
		/// This is from KDenLive's TreeItem.  Looks like they're trying to keep the model
		/// in memory until all children are deleted.

		// We're in a model, deregister ourself from it.
		if (auto ptr = m_model.lock())
		{
			ptr->deregister_item(m_uuincid, this);
			AMLM_ASSERT_X(isInModel() == false, "ITEM STILL IN MODEL");
		}
		else
		{
			Q_ASSERT(0);
		}
	}
}

void AbstractTreeModelItem::updateParent(std::shared_ptr<AbstractTreeModelItem> parent)
{
	// New parent, possibly null.
	m_parent_item = parent;
	if(parent)
	{
		// Keep depth up to date.
//		m_depth = parent->m_depth + 1;
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

