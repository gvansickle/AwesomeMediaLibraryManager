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

// Qt
#include <QBrush>
#include <QStringList>

// Ours
#include <utils/DebugHelpers.h>
#include <utils/VectorHelpers.h>
#include <logic/UUIncD.h>
#include "AbstractTreeModel.h"
#include <utils/ext_iterators.h>
#include <logic/serialization/SerializationHelpers.h>
#include "ItemFactory.h"
#include "ThreadsafeTreeModel.h"


AMLM_QREG_CALLBACK([](){
	qIn() << "Registering std::shared_ptr<AbstractTreeModelItem>";
	qRegisterMetaType<AbstractTreeModelItem>();
	qRegisterMetaType<std::shared_ptr<AbstractTreeModelItem>>();
    qRegisterMetaType<std::unique_ptr<AbstractTreeModelItem>>();
    qRegisterMetaType<std::weak_ptr<AbstractTreeModelItem>>();
});


std::shared_ptr<AbstractTreeModelItem> AbstractTreeModelItem::create(const std::vector<QVariant>& data,
																	const std::shared_ptr<AbstractTreeModel>& model,
																	bool is_root)
{
    std::shared_ptr<AbstractTreeModelItem> new_item = std::shared_ptr<AbstractTreeModelItem>(new AbstractTreeModelItem(data, model));
    baseFinishCreate(new_item);

	return new_item;
}

AbstractTreeModelItem::AbstractTreeModelItem(const std::vector<QVariant>& data, const std::shared_ptr<AbstractTreeModel>& model)
{
	m_item_data = data;
	m_model = model;
	m_depth = 0;
	m_uuincid = UUIncD::create();
	m_is_in_model = false;
	m_is_root = false;
}

AbstractTreeModelItem::~AbstractTreeModelItem()
{
	deregister_self();
}

void AbstractTreeModelItem::clear()
{
	// Reset this item to completely empty, except for its place in the model.
	m_child_items.clear();
	m_item_data.clear();
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
	// qWr() << "###################### SELFSOFTDELETE()";
// M_WARNING("TODO, NEEDS MUTEX MEMBER");
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
            qCr() << "Can't find ourselves in parent's list:" << M_ID_VAL(this_cast->getId());
            Q_ASSERT(0);
			return -1;
		}
	}

	// No parent, ETM returns 0 here, KDen returns -1.
    return -1;
}


bool AbstractTreeModelItem::insertColumns(int insert_before_column, int num_columns)
{
	// Check if caller is trying to insert a column out of bounds.
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
	return m_is_in_model;
}

bool AbstractTreeModelItem::operator==(const AbstractTreeModelItem& other) const
{
	Q_ASSERT(0);
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
		// Get iterator corresponding to child
		auto it = get_m_child_items_iterator(child->getId());
		Q_ASSERT(it != m_child_items.end());
//		Q_ASSERT(m_iteratorTable.count(child->getId()) > 0);
//		auto it = m_iteratorTable[child->getId()];
		// Delete the child.
		m_child_items.erase(it);
		// clean iterator table
//		m_iteratorTable.erase(child->getId());
		child->m_depth = 0;
		child->m_parent_item.reset();
		child->deregister_self();
		ptr->notifyRowDeleted();
	}
	else
	{
		qCr() << "ERROR: Couldn't lock model.";
		Q_ASSERT(false);
	}
}

bool AbstractTreeModelItem::changeParent(std::shared_ptr<AbstractTreeModelItem> newParent)
{
	Q_ASSERT(!m_is_root);
	if (m_is_root)
	{
		// Don't allow changing the root item's parent.
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


#define M_DATASTREAM_FIELDS(X) \
	/* TAG_IDENTIFIER, tag_string, member_field, var_name */ \
    X(XMLTAG_CHILD_ITEM_LIST, child_item_list, nullptr)

#define M_DATASTREAM_FIELDS_CONTSIZES(X) \
	X(XMLTAG_NUM_COLUMNS, num_columns, m_item_data) \
	X(XMLTAG_ITEM_DATA_SIZE, item_data_size, m_item_data) \
	X(XMLTAG_NUM_CHILDREN, num_children, m_child_items)

using strviw_type = QLatin1String;

///// Strings to use for the tags.
#define X(field_tag, tag_string, var_name) static constexpr strviw_type field_tag ( # tag_string );
	M_DATASTREAM_FIELDS(X);
	M_DATASTREAM_FIELDS_CONTSIZES(X);
#undef X


QVariant AbstractTreeModelItem::toVariant() const
{
	InsertionOrderedMap<QString, QVariant> map;

	// Write class info to the map.
	set_map_class_info(this, &map);

// #define X(field_tag, tag_string, var_name) map_insert_or_die(map, field_tag, var_name);
// 	M_DATASTREAM_FIELDS(X);
// #undef X
#define X(field_tag, tag_string, var_name) map_insert_or_die(map, field_tag, (qulonglong)(var_name).size());
	M_DATASTREAM_FIELDS_CONTSIZES(X);
#undef X

	/// @todo The "m_item_data" string is not getting written out, not sure if we care.
	QVariantHomogenousList list("m_item_data", "item");
	// The item data itself.
	for(const QVariant& itemdata : m_item_data)
	{
		list_push_back_or_die(list, itemdata);
	}
	// Add them to the output map.
	map_insert_or_die(map, "item_data", list);

    // Serialize out Child items.
    QVariantHomogenousList child_list("m_child_items", "child_item");
	for (auto child : m_child_items)
	{
		// Add the AbstractTreeModelItem to the list.
		// Qt can't serialize smart pointers so we have to send the pointed-to objects.
		QVariant var_child = child->toVariant();
		list_push_back_or_die(child_list, var_child);
	}

	// Insert the list into the map.
    map_insert_or_die(map, XMLTAG_CHILD_ITEM_LIST, child_list);

	return map;
}


void AbstractTreeModelItem::fromVariant(const QVariant& variant)
{
	InsertionOrderedMap<QString, QVariant> map = variant.value<InsertionOrderedMap<QString, QVariant>>();

#define X(field_tag, tag_string, member_field) map_read_field_or_warn(map, field_tag, member_field);
//	M_DATASTREAM_FIELDS(X);
#undef X

    if(m_is_root)
    {
        // Already set up in ...HeaderItem::fromVariant().
    }
    else
    {
        // m_model.lock()->m_model_item_map[m_uuincid] = weak_from_this();
    }
	// m_uuincd is set in the AbstractTreeModelItem constructor.

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

    // Now read in our children.  We need this Item to be in a model and registered for that to work,
	// so first add *this to the model.
	if (m_is_root)
	{
		// We're root, so we have no parent.
		// Here, we're already registered with the model by the AbstractTreeModel::create() call.
		Q_ASSERT(m_is_in_model);
		Q_ASSERT(m_uuincid != UUIncD::null());
	}
	else
	{
        // For all other items, we need to get *this into the model, so it can be found
        // when its children are subsequently added to the model.
        // This should have happened on the previous recursion, in the child item requestAddItem() call below.
        // std::dynamic_pointer_cast<ThreadsafeTreeModel>(m_model.lock())->requestAddItem(shared_from_this(), parentid);
        // std::shared_ptr<AbstractTreeModelItem> parent_item = m_model.lock()->getItemById();
	}
    Q_ASSERT(isInModel());
    QVariantHomogenousList child_list(XMLTAG_CHILD_ITEM_LIST, "child_item");
	child_list = map.at(XMLTAG_CHILD_ITEM_LIST).value<QVariantHomogenousList>();
	qDb() << M_ID_VAL(child_list.size());

	AMLM_ASSERT_EQ(num_children, child_list.size());

#if 1
    // auto model_ptr = parent()->m_model.lock();
    auto model_ptr = m_model.lock();
	Q_ASSERT(model_ptr);
    for(auto& child_item : child_list)
    {
        // What was the derived class type that was actually written?
    	std::string class_attr;
        if (child_item.canConvert<InsertionOrderedMap<QString, QVariant>>())
    	{
            auto child_item_map = child_item.value<InsertionOrderedMap<QString, QVariant>>();
    		class_attr = child_item_map.get_attr("class");
    	}
    	else
    	{
    		// Tree is corrupted.
    		Q_ASSERT(0);
    	}

    	auto derived_child_ptr = ItemFactory::instance().createItem(class_attr);

    	if (derived_child_ptr)
    	{
    		derived_child_ptr->setModel(model_ptr);
            std::dynamic_pointer_cast<ThreadsafeTreeModel>(model_ptr)->requestAddItem(derived_child_ptr, this->getId());

            // derived_child_ptr->updateParent(shared_from_this());
    		derived_child_ptr->fromVariant(child_item);
            // std::shared_ptr<AbstractTreeModelItem> cptr(std::move(derived_child_ptr));
            // qDb() << "APPENDING" << *cptr;

            // appendChild(cptr);
    	}
#endif
#if 0
        auto derived_child_metatype = QMetaType::fromName(class_attr.c_str());
        if(derived_child_metatype.isValid())
        {
            qDb() << "Derived QMetaType:" << derived_child_metatype;

            if (!child_item.canConvert(derived_child_metatype))
        	{
        		Q_ASSERT(0);
        	}

        	bool convert_ok = child_item.convert(derived_child_metatype);
        	Q_ASSERT(convert_ok);

        	void* data = derived_child_metatype.create(variant.constData());
        	std::shared_ptr<AbstractTreeModelItem> child_sptr(reinterpret_cast<AbstractTreeModelItem*>(data));
            // qDb() << "Class attr:" << M_ID_VAL(metatype) << M_ID_VAL(typename_per_var);
            // AbstractTreeModelItem child = child_item.value<AbstractTreeModelItem>();
            // std::shared_ptr<AbstractTreeModelItem> child_sptr = AbstractTreeModelItem::create(child.m_item_data, model_ptr, child.isRoot(), UUIncD::create());
            // std::shared_ptr<AbstractTreeModelItem> child_sptr = std::make_shared<AbstractTreeModelItem>();
            // *child_sptr = child;
            appendChild(child_sptr);
        }
        else
        {
            // Error.
            Q_ASSERT(0);
        }
#endif
    }
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

/// Return the QVariant at @a column.
/// KDen behavior is to return def const QVariant if > num cols.
QVariant AbstractTreeModelItem::dataColumn(int column) const
{
	try
	{
		return m_item_data.at(column);
	}
	catch (...) // std::out_of_range
	{
		return QVariant();
	}
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
        std::shared_ptr<AbstractTreeModelItem> item = AbstractTreeModelItem::create(data,
			m_model.lock()->shared_from_this(), UUIncD::create());
		m_child_items[position] = item;
		retval.push_back(item);
	}

	return retval;
}

void AbstractTreeModelItem::insertChild(int row, std::shared_ptr<AbstractTreeModelItem> item)
{
#if 1 /// AQP

    AMLM_ASSERT_X(!item->isInModel(), "TODO: ITEM ALREADY IN A MODEL, MOVE ITEMS BETWEEN MODELS");
//	AMLM_ASSERT_X(isInModel(), "TODO: PARENT ITEM NOT IN MODEL");

    if(has_ancestor(item->getId()))
    {
        // Trying to create a cycle, abort.
        return;
    }

    if(auto old_parent = item->parent_item().lock())
    {
        if(old_parent->getId() == getId())
        {
            // Child is already parented by us, nothing to do.
            return;
        }
        qCr() << "ERROR: Incoming child already has a parent.";
        return;
    }

    // If the parent is in a model, add the child item to the same model.
    if(auto model = m_model.lock())
    {
        model->notifyRowAboutToAppend(shared_from_this());
        item->updateParent(shared_from_this());

        // Need an iterator to insert before.
        auto ins_it = m_child_items.begin();
        std::advance(ins_it, row);

        m_child_items.insert(ins_it, item);

		register_self(item);

        model->notifyRowAppended(item);
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
// M_TODO("CRASHING HERE");/// @todo Recovery?
Q_ASSERT(0);
			return false;
		}
	}

	return true;
}

bool AbstractTreeModelItem::appendChild(const std::shared_ptr<AbstractTreeModelItem>& new_child)
{
    AMLM_ASSERT_IN_GUITHREAD();

	if(has_ancestor(new_child->getId()))
	{
		// Somehow trying to create a cycle in the tree.
        // Q_ASSERT(0);
		return false;
	}
	if (auto oldParent = new_child->parent_item().lock())
	{
        if (oldParent->getId() == getId())
		{
			// new_child has us as current parent, no change needed.
			return true;
		}
		else
		{
            // Otherwise new_child has a different parent already.
            qCr() << "ERROR: trying to append a child that already has a parent";
            // Q_ASSERT(0);
			return false;
		}
	}
	if (auto ptr = m_model.lock())
	{
        // Make sure *this is managed by a shared_ptr.
        if(auto sharedThis = weak_from_this().lock())
        {
            // qDb() << "Use count:" << sharedThis.use_count();
        }
        else
        {
            qCr() << "Object is not managed by a std::shared_ptr";
        	Q_ASSERT(0);
        }

        std::shared_ptr<AbstractTreeModelItem> sft = shared_from_this();
		Q_ASSERT(sft);
		ptr->notifyRowAboutToAppend(shared_from_this());
		new_child->updateParent(shared_from_this());
		UUIncD id = new_child->getId();
        m_child_items.push_back(new_child);
		register_self(new_child);
		ptr->notifyRowAppended(new_child);

		verify_post_add_ins_child(new_child);


		return true;
	}
    qCr() << "ERROR: Something went wrong when appending child.";
	Q_ASSERT(false);
	return false;
}

/// Append a child item created from @a data.
std::shared_ptr<AbstractTreeModelItem> AbstractTreeModelItem::appendChild(const std::vector<QVariant>& data)
{
	if (auto ptr = m_model.lock())
	{
		// Create the new child with this item's model as the model.
		// Note that by definition, this will not be the root item.
		auto child = AbstractTreeModelItem::create(data, ptr, false);
		appendChild(child);
		return child;
	}
	qCr() << "ERROR: Something went wrong when appending child to AbstractTreeModelItem.";
	Q_ASSERT(false);

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
	AMLM_ASSERT_X(inserted_child->has_ancestor(m_uuincid), "UUIncD of the parent is not an ancestor of child.");
	auto child_locked_par_item = inserted_child->m_parent_item.lock();
	AMLM_ASSERT_X(child_locked_par_item == this->shared_from_this(), "CHILD'S PARENT IS NOT THIS");
	// If parent is in a model, child should be in the same model.
	auto child_par_model = child_locked_par_item->m_model.lock();
	auto child_in_model = inserted_child->isInModel();
	if(isInModel())
	{
		AMLM_ASSERT_X(inserted_child->isInModel(), "PARENT IN MODEL, CHILD ISN'T");
		AMLM_ASSERT_X(child_par_model == m_model.lock(), "PARENT AND CHILD ARE IN DIFFERENT MODELS");
	}
	if (!inserted_child->isInModel() && !isInModel())
	{
		qDb() << "NEITHER PARENT NOR CHILD isInModel()";
	}
	if (!isInModel() && child_locked_par_item->isInModel())
	{
		AMLM_ASSERT_X(0, "*this is not in a model but child is");
	}
}

void AbstractTreeModelItem::baseFinishCreate(const std::shared_ptr<AbstractTreeModelItem>& new_item)
{
	if(new_item->isRoot())
	{
		register_self(new_item);
	}
}

/**
 * Static function which registers @a self and its children with the model @a self is already registered with.
 * @warning Will assert if @a self doesn't already know its model.
 * @param self
 */
void AbstractTreeModelItem::register_self(const std::shared_ptr<AbstractTreeModelItem>& self)
{
    // Register children, who will then register their own children, etc....
	for (const auto& child : self->m_child_items)
	{
		register_self(child);
	}
	// If we still have a model, register with it.
    // qDb() << M_ID_VAL(self->m_model);
    Q_ASSERT(!(self->m_model.expired()));
    Q_ASSERT(self->m_model.use_count() > 0);
	if (auto ptr = self->m_model.lock())
	{
		ptr->register_item(self);
		self->m_is_in_model = true;
		AMLM_ASSERT_EQ(self->isInModel(), true);
	}
	else
	{
        qWr() << "COULDN'T LOCK MODEL:"; //<< M_ID_VAL((self->m_model));// << M_ID_VAL(self->m_model);
        AMLM_ASSERT_X(false,"Error : construction of AbstractTreeModelItem failed");
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
			m_is_in_model = false;
			AMLM_ASSERT_X(isInModel() == false, "ITEM STILL IN MODEL");
		}
        // else
        // {
		//  /// @note This will assert on model destruct.  Kdenlive ignores this else.
        // 	Q_ASSERT(0);
        // }
	}
}

void AbstractTreeModelItem::updateParent(std::shared_ptr<AbstractTreeModelItem> parent)
{
	// New parent, possibly null.
	m_parent_item = parent;
	if(parent)
	{
		// Keep depth up to date.
		m_depth = parent->m_depth + 1;
		// Keep max column count up to date.
		/// @todo Do we need this?
//		m_num_parent_columns = parent->columnCount();
	}
}

AbstractTreeModelItem::CICTIteratorType AbstractTreeModelItem::get_m_child_items_iterator(UUIncD id)
{
	CICTIteratorType retval;
	retval = std::find_if(m_child_items.begin(), m_child_items.end(), [id](auto& val){ return val->m_uuincid == id; });
	return retval;
}

