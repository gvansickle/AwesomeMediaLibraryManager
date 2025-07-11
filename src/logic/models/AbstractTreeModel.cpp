/*
 * Copyright 2018, 2019, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
 * @file AbstractTreeModel.cpp
 * Implementation of AbstractTreeModel.
 *
 * This class is heavily adapted from at least the following:
 * - The "Editable Tree Model Example" shipped with Qt5.
 * - KDenLive's AbstractItemModel class.
 * - My own original work.
 * - Hundreds of nuggets of information from all over the Internet.
 */

// This class's header.
#include "AbstractTreeModel.h"

// Std C++
#include <functional>
#include <unordered_set>
#include <queue>

// Qt
#include <QModelIndex>
#include <QAbstractItemModelTester>

// Ours
#include "AbstractTreeModelItem.h"
#include "AbstractTreeModelHeaderItem.h"
#include "ColumnSpec.h"
#include <utils/DebugHelpers.h>
#include <logic/serialization/XmlSerializer.h>
// #include <third_party/sqlite_orm/include/sqlite_orm/sqlite_orm.h>

// static
std::shared_ptr<AbstractTreeModel>
AbstractTreeModel::create(std::initializer_list<ColumnSpec> column_specs, QObject* parent)
{
    auto retval_shptr = std::shared_ptr<AbstractTreeModel>(new AbstractTreeModel(column_specs, parent));

    retval_shptr->m_root_item = AbstractTreeModelHeaderItem::create(column_specs, retval_shptr);

    retval_shptr->checkConsistency();

	return retval_shptr;
}

AbstractTreeModel::AbstractTreeModel(QObject* parent) : QAbstractItemModel(parent)
{
	// Delegate to this constructor, then you'll have a vtable and should be able to call virtual functions in the other constructor.
}

AbstractTreeModel::AbstractTreeModel(std::initializer_list<ColumnSpec> column_specs, QObject* parent)
	: AbstractTreeModel(parent)
{
	/// @todo Anything here?  Odd to just throw the column specs out....
}

AbstractTreeModel::~AbstractTreeModel()
{
	// KDEN does exactly this in its ~AbstractTreeModel().
	m_model_item_map.clear();
	m_root_item.reset();
}

void AbstractTreeModel::SLOT_appendChildToRoot(std::shared_ptr<AbstractTreeModelItem> child)
{
	AMLM_ASSERT_IN_GUITHREAD();
	getRootItem()->appendChild(child);
}

void AbstractTreeModel::SLOT_appendChild(std::shared_ptr<AbstractTreeModelItem> child, UUIncD parent_id)
{
	AMLM_ASSERT_IN_GUITHREAD();
	getItemById(parent_id)->appendChild(child);
}

void AbstractTreeModel::clear(bool quit)
{
	// std::unique_lock write_lock(m_rw_mutex);

#if 0///KDEN @info KDenLive has this in ProjectItemModel, as "clean()", and it's a bit different.
	std::vector<std::shared_ptr<AbstractTreeModelItem>> items_to_delete;

	items_to_delete.reserve(m_root_item->childCount());

	for (int i = 0; i < m_root_item->childCount(); ++i)
	{
		items_to_delete.push_back(std::static_pointer_cast<AbstractTreeModelItem>(m_root_item->child(i)));
	}
	Fun undo = []() { return true; };
	Fun redo = []() { return true; };
	for (const auto &child : items_to_delete)
	{
		qDb() << "clearing" << items_to_delete.size() << "items, current:" << child->m_uuid;
		requestDeleteItem(child, undo, redo);
	}
	Q_ASSERT(m_root_item->childCount() == 0);

	// One last thing, our hidden root node / header node still has ColumnSpecs.
	m_root_item->clear();
#elif 0
	beginResetModel();

	// Delete the root item, which will get us 99% of the way to cleared.
//	m_root_item.reset();

	// One last thing, our hidden root node / header node still has ColumnSpecs.
	m_root_item->clear();

	endResetModel();
#endif
}

bool AbstractTreeModel::setColumnSpecs(std::vector<ColumnSpec> column_specs)
{
	// std::unique_lock write_lock(m_rw_mutex);

	Q_ASSERT(m_root_item);

	return m_root_item->setColumnSpecs(column_specs);
}

int AbstractTreeModel::columnCount(const QModelIndex& parent) const
{
	/// @note ETM does this differently, simply always returning m_root_item->columnCount().
	// std::unique_lock read_lock(m_rw_mutex);

	if(!parent.isValid())
	{
		// Invalid index, return root column count.
		return m_root_item->columnCount();
	}
	// Else look up the item and return its column count.
	const auto id = UUIncD(parent.internalId());
	auto item = getItemById(id);
	return item->columnCount();
}

QVariant AbstractTreeModel::data(const QModelIndex &index, int role) const
{
	QModelRoleData roleData(role);
	multiData(index, roleData);
	return roleData.data();
}

void AbstractTreeModel::multiData(const QModelIndex& index, QModelRoleDataSpan roleDataSpan) const
{
	// Locked in ThreadsafeTreeModel.
// std::shared_lock read_lock(m_rw_mutex);

	// Get data for each role at the given index.
	for (QModelRoleData& roleData : roleDataSpan)
	{
		// data() expects a valid index, except it won't get one for data() calls for the root item info.
		//	Q_ASSERT(checkIndex(index, CheckIndexOption::IndexIsValid));

		// Clear any stale data that is in this QModelRoleData.
		roleData.clearData();

		if (!index.isValid())
		{
			// Should never get here, checkIndex() should have asserted above.
			/// @todo ^^^ Assert no longer exists, so... ?
			continue;
		}

		int role = roleData.role();

		// Color invalid model indexes.
		if(index.column() >= columnCount())
		{
			switch(role)
			{
			case Qt::ItemDataRole::BackgroundRole:
				roleData.setData(QBrush(Qt::lightGray));
				break;
			default:
				// return QVariant();
				// break;
				continue;
			}
		}

		if (role != Qt::DisplayRole && role != Qt::EditRole) /// @todo Not in KDen AbstTreeModel: && role != Qt::EditRole)
		{
			// return QVariant();
			roleData.setData(QVariant());
			continue;
		}

		// Get a pointer to the indexed item.
		std::shared_ptr<AbstractTreeModelItem> item = getItem(index);

		// Return the requested [column,role] data from the item.
		// return item->data(index.column(), role);
		roleData.setData(item->data(index.column(), role));
	}
}

Qt::ItemFlags AbstractTreeModel::flags(const QModelIndex &index) const
{
	// Locking in ThreadsafeTreeModel, then calls this.
	// std::unique_lock read_lock(m_rw_mutex);

	if (!index.isValid())
	{
		return Qt::NoItemFlags;
	}

#if 0 /// KDen does this here, non-selectable root items, not sure we want it.
	if (index.isValid()) {
        auto item = getItemById((int)index.internalId());
        if (item->depth() == 1) {
            return flags & ~Qt::ItemIsSelectable;
        }
    }
#endif
	// ETM:
    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

std::shared_ptr<AbstractTreeModelItem> AbstractTreeModel::getItem(const QModelIndex &index) const
{
	// std::unique_lock read_lock(m_rw_mutex);
	/**
	 * @warning There's a fail here.  Trying to do a mapping from QModelIndex->TreeItem*, but we're going through
	 * getItemById() to do it (index.internalId() -> TreeItem*).  In e.g. insertChild(), this results in the
	 * second lookup being done before the item is in the map, == error.
	 */
	if (index.isValid())
	{
		std::shared_ptr<AbstractTreeModelItem> item = getItemById(UUIncD(index.internalId()));
		if (item != nullptr)
		{
            return item;
		}
		else
		{
			/// GRVS: AQP doesn't do this, simply falls through and returns root item.
			Q_ASSERT(0);
		}

	}
	// Invalid index, return the root item.
	return m_root_item;
}

//void AbstractTreeModel::setBaseDirectory(const QUrl& base_directory)
//{
//	std::unique_lock write_lock(m_rw_mutex);
//
//	m_base_directory = base_directory;
//}

// KDEN, GRVS locking in ThreadSafeTreeModel.
std::shared_ptr<AbstractTreeModelItem> AbstractTreeModel::getItemById(const UUIncD& id) const
{
	Q_ASSERT(m_root_item);
	Q_ASSERT(id != UUIncD::null());

	if(id == m_root_item->getId())
	{
		return m_root_item;
	}

	Q_ASSERT(m_model_item_map.count(id) > 0);
	return m_model_item_map.at(id).lock();
}

// BOTH
std::shared_ptr<AbstractTreeModelItem> AbstractTreeModel::getRootItem() const
{
	Q_ASSERT(m_root_item);
	return m_root_item;
}

#if 0/// ETM
/**
 * Insert an empty new child under @a parent and returns a shared_ptr to it.
 * ETM: From MainWindow, where parent is always currentIndex() from a selection model.
 * @param parent
 * @return
 */
std::shared_ptr<AbstractTreeModelItem> AbstractTreeModel::insertChild(const QModelIndex& parent)
{
	const QModelIndex index = parent;
	std::shared_ptr<AbstractTreeModel> model = this->shared_from_this();

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
#endif

/**
 * @todo THIS APPEARS TO BE UNUSED.  REMOVE?
 * @param data
 * @param parent
 * @return
 */
std::shared_ptr<AbstractTreeModelItem> AbstractTreeModel::append_child(const QVector<QVariant>& data, std::shared_ptr<AbstractTreeModelItem> parent)
{
	// Append a new item to the given parent's list of children.
	parent->insertChildren(parent->childCount(), 1, m_root_item->columnCount());
	QVector<QVariant> columnData;
	for(int column = 0; column < data.size(); ++column)
	{
		columnData << data[column];
	}
	std::shared_ptr<AbstractTreeModelItem> new_child = parent->child(parent->childCount()-1);

	// Register the new child with this model.
    /// @todo We shouldn't have to do this?
	register_item(new_child);

	for(int column = 0; column < columnData.size(); ++column)
	{
		new_child->setData(column, columnData[column]);
	}

	return new_child;
}

Fun AbstractTreeModel::addItem_lambda(const std::shared_ptr<AbstractTreeModelItem>& new_item, UUIncD parentId)
{
	return [this, new_item, parentId]() {
		// Insertion is simply setting the parent of the item...
		std::shared_ptr<AbstractTreeModelItem> parent;
		if (parentId != UUIncD::null())
		{
			parent = getItemById(parentId);
			if (!parent)
			{
				Q_ASSERT(parent);
				return false;
			}
		}
		// ...and fixing up the parent.
		return new_item->changeParent(parent);
	};
}

Fun AbstractTreeModel::removeItem_lambda(UUIncD id)
{
	return [this, id]() {
		/* Deletion simply deregister the item and remove it from parent.
		   The item object is not actually deleted, because a shared_pointer to it
		   is captured by the reverse operation.
		   Actual deletions occurs when the undo object is destroyed.
		*/
		auto item = m_model_item_map[id].lock();
		Q_ASSERT(item);
		if (!item)
		{
			return false;
		}
		auto parent = item->parent_item().lock();
		parent->removeChild(item);
		return true;
	};
}

Fun AbstractTreeModel::moveItem_lambda(UUIncD id, int destRow, bool force)
{
	Fun lambda = []() { return true; };

	std::vector<std::shared_ptr<AbstractTreeModelItem>> oldStack;
	auto item = getItemById(id);
	if (!force && item->childNumber() == destRow)
	{
		// nothing to do
		return lambda;
	}
	if (auto parent = item->parent_item().lock())
	{
		if (destRow > parent->childCount() || destRow < 0)
		{
			return []() { return false; };
		}
		UUIncD parentId = parent->getId();
		// remove the element to move
		oldStack.push_back(item);
		Fun oper = removeItem_lambda(id);
		PUSH_LAMBDA(oper, lambda);
		// remove the tail of the stack
		for (int i = destRow; i < parent->childCount(); ++i) {
			auto current = parent->child(i);
			if (current->getId() != id) {
				oldStack.push_back(current);
				oper = removeItem_lambda(current->getId());
				PUSH_LAMBDA(oper, lambda);
			}
		}
		// insert back in order
		for (const auto &elem : oldStack)
		{
			oper = addItem_lambda(elem, parentId);
			PUSH_LAMBDA(oper, lambda);
		}
		return lambda;
	}
	return []() { return false; };
}

bool AbstractTreeModel::LoadDatabase(const QString& database_filename)
{
	qIn() << "###### READING AbstractTreeModel from:" << database_filename;

	/// @note Not sure if this is the best place for this, but we need to clear everything before reloading it.

	/// @todo Locking?
	// std::unique_lock write_lock(m_rw_mutex);

	XmlSerializer xmlser;
	/// @todo Better name
	DERIVED_set_default_namespace();
	if(!m_default_namespace_decl.empty())
	{
		xmlser.set_default_namespace(m_default_namespace_decl, m_default_namespace_version);
	}
	xmlser.HACK_skip_extra(false);

	beginResetModel();
	clear();

	bool success = xmlser.load(*this, QUrl::fromLocalFile(database_filename));
	endResetModel();

	qIn() << "###### TREEMODELPTR HAS NUM ROWS:" << rowCount();
	qIn() << "###### FINISHED READING AbstractTreeModel from:" << database_filename << "STATUS:" << success;

	return success;
}

void AbstractTreeModel::SaveDatabase(const QString& database_filename)
{
	// std::unique_lock read_lock(m_rw_mutex);

	qIn() << "###### WRITING AbstractTreeModel to:" << database_filename;
	qIn() << "###### TREEMODELPTR HAS NUM ROWS:" << rowCount();

	XmlSerializer xmlser;

	DERIVED_set_default_namespace();
	if(!m_default_namespace_decl.empty())
	{
		xmlser.set_default_namespace(m_default_namespace_decl, m_default_namespace_version);
	}
	xmlser.save(*this, QUrl::fromLocalFile(database_filename), "playlist");

	qIn() << "###### FINISHED WRITING AbstractTreeModel to:" << database_filename;
}

void AbstractTreeModel::dump_model_info() const
{
	qDb() << "------------------------ MODEL INFO -----------------------------------------";
	qDb() << "Total items in m_model_item_map:" << m_model_item_map.size();
}

#if 0 /// ORM
void AbstractTreeModel::toOrm(std::string filename) const
{
//	using namespace sqlite_orm;
//	auto storage = make_storage(filename + "_db.sqlite",
//	                            make_table("root_item",
//	                                       make_column("id", &ISerializable::m_uuid, autoincrement(), primary_key()),
//	                                       make_column("first_name", &User::firstName),
//	                                       make_column("last_name", &User::lastName),
//	                                       make_column("birth_date", &User::birthDate),
//	                                       make_column("image_url", &User::imageUrl),
//	                                       make_column("type_id", &User::typeId)),
//	                            make_table("user_types",
//	                                       make_column("id", &UserType::id, autoincrement(), primary_key()),
//	                                       make_column("name", &UserType::name),
//	                                       make_column("comment", &UserType::comment, default_value("user"))));
	Q_ASSERT(0);
}

void AbstractTreeModel::fromOrm(std::string filename)
{
	Q_ASSERT(0);
}
#endif

AbstractTreeModel::iterator AbstractTreeModel::begin()
{
	return m_model_item_map.begin();
}

AbstractTreeModel::iterator AbstractTreeModel::end()
{
	return m_model_item_map.end();
}

void AbstractTreeModel::register_item(const std::shared_ptr<AbstractTreeModelItem>& item)
{
	// std::unique_lock write_lock(m_rw_mutex);

	// qDb() << "Registering:" << M_ID_VAL(item->getId()) << M_ID_VAL(m_model_item_map.size());

	UUIncD id = item->getId();
	Q_ASSERT(id.isValid());
	// qDb() << "MIM ADD:" << id;
	AMLM_ASSERT_X(m_model_item_map.count(id) == 0, "Item was already in model.");
	m_model_item_map[id] = item;

    // qDb() << "Registered," << M_ID_VAL(m_model_item_map.size());
}

void AbstractTreeModel::deregister_item(UUIncD id, AbstractTreeModelItem* item)
{
	// std::unique_lock write_lock(m_rw_mutex);

	Q_UNUSED(item);

	if(m_model_item_map.count(id) == 0)
	{
		// Nothing to erase, we weren't in the model for some reason.
		qWr() << "Attempt to deregister unregistered item:" << M_ID_VAL(id) << M_ID_VAL(item);
		Q_ASSERT(0);
	}
	else
	{
		m_model_item_map.erase(id);
	}
}

void AbstractTreeModel::notifyColumnsAboutToInserted(const std::shared_ptr<AbstractTreeModelItem>& parent, int first_column, int last_column)
{
	auto parent_index = getIndexFromItem(parent);
	beginInsertColumns(parent_index, first_column, last_column);
}

void AbstractTreeModel::notifyColumnsInserted()
{
	endInsertColumns();
}

#if EVER_NEEDED
void AbstractTreeModel::notifyRowsAboutToInsert(const std::shared_ptr<AbstractTreeModelItem>& row, int first, int last)
{
	/// @todo handle root item
	auto parent_index = getIndexFromItem(item);
}

void AbstractTreeModel::notifyRowsInserted(const std::shared_ptr<AbstractTreeModelItem>& row)
{

}
#endif

void AbstractTreeModel::notifyRowAboutToAppend(const std::shared_ptr<AbstractTreeModelItem>& item)
{
	auto index = getIndexFromItem(item);
	beginInsertRows(index, item->childCount(), item->childCount());
}

void AbstractTreeModel::notifyRowAppended(const std::shared_ptr<AbstractTreeModelItem>& row)
{
	Q_UNUSED(row);
	endInsertRows();
}

void AbstractTreeModel::notifyRowAboutToDelete(std::shared_ptr<AbstractTreeModelItem> item, int row)
{
	auto index = getIndexFromItem(item);
	beginRemoveRows(index, row, row);
}

void AbstractTreeModel::notifyRowDeleted()
{
	endRemoveRows();
}


// KDEN:
bool AbstractTreeModel::checkConsistency()
{
	// first check that the root is all good
	if (!m_root_item || !m_root_item->m_is_root || !m_root_item->isInModel() || m_model_item_map.count(m_root_item->getId()) == 0)
	{
		qDebug() << !m_root_item->m_is_root << !m_root_item->isInModel() << (m_model_item_map.count(m_root_item->getId()) == 0);
		qDebug() << "ERROR: Model is not valid because root is not properly constructed";
        Q_ASSERT(0);
		return false;
	}

	// Then we traverse the tree from the root, checking the infos on the way
	std::unordered_set<UUIncD> seenIDs;
	std::queue<std::pair<UUIncD, std::pair<int, UUIncD>>> queue; // store (id, (depth, parentId))
	queue.push({m_root_item->getId(), {0, m_root_item->getId()}});
	while (!queue.empty())
	{
		auto current = queue.front();
		UUIncD currentId = current.first;
		int currentDepth = current.second.first;
		UUIncD parentId = current.second.second;
		queue.pop();
		if (seenIDs.count(currentId) != 0)
		{
			qDebug() << "ERROR: Invalid tree: Id found twice."
			         << "It either a cycle or a clash in id attribution";
			return false;
		}
		if (m_model_item_map.count(currentId) == 0)
		{
			qDebug() << "ERROR: Invalid tree: Id not found. Item is not registered";
			return false;
		}
		auto currentItem = m_model_item_map[currentId].lock();
		if (currentItem->depth() != currentDepth)
		{
			qDebug() << "ERROR: Invalid tree: invalid depth info found";
			return false;
		}
		if (!currentItem->isInModel())
		{
			qDebug() << "ERROR: Invalid tree: item thinks it is not in a model";
			return false;
		}
		if (currentId != m_root_item->getId())
		{
			if ((currentDepth == 0 || currentItem->m_is_root))
			{
				qDebug() << "ERROR: Invalid tree: duplicate root";
				return false;
			}
			if (auto ptr = currentItem->parent_item().lock())
			{
				if (ptr->getId() != parentId || ptr->child(currentItem->childNumber())->getId() != currentItem->getId())
				{
					qDebug() << "ERROR: Invalid tree: invalid parent link";
					return false;
				}
			}
			else
			{
				qDebug() << "ERROR: Invalid tree: invalid parent";
				return false;
			}
		}
		// propagate to children
		int i = 0;
		for (const auto &child : currentItem->m_child_items)
		{
			if (currentItem->child(i) != child) {
				qDebug() << "ERROR: Invalid tree: invalid child ordering";
				return false;
			}
			queue.push({child->getId(), {currentDepth + 1, currentId}});
			i++;
		}
	}

	return true;
}

QVariant AbstractTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	// std::unique_lock read_lock(m_rw_mutex);

	// Both ETM and KDEN are the same here.
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		return m_root_item->dataColumn(section);
	}

    return QVariant();
}

QModelIndex AbstractTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	std::shared_ptr<AbstractTreeModelItem> parent_item;

	// Get the parent item QMI parent is pointing to.
	if(!parent.isValid())
	{
		parent_item = m_root_item;
	}
	else
	{
		parent_item = getItemById(UUIncD(parent.internalId()));
	}

	if (row >= parent_item->childCount())
	{
		// Request is for a row beyond what the parent actually has.
        return QModelIndex();
	}

	std::shared_ptr<AbstractTreeModelItem> child_item = parent_item->child(row);

	if(child_item)
	{
		return createIndex(row, column, quintptr(child_item->getId()));
	}
	else
	{
        return QModelIndex();
	}
}

bool AbstractTreeModel::insertColumns(int insert_before_column, int num_columns, const QModelIndex& parent_model_index)
{
	Q_CHECK_PTR(m_root_item);

	bool success;

	beginInsertColumns(parent_model_index, insert_before_column, insert_before_column + num_columns - 1);

// M_WARNING("TODO: This is what ETM has.  Should it be the parent_model_index, not the m_root_item?");
	success = m_root_item->insertColumns(insert_before_column, num_columns);

	endInsertColumns();

	return success;
}

bool AbstractTreeModel::insertRows(int insert_before_row, int num_rows, const QModelIndex& parent_model_index)
{
#if 1 //ETM
	qDb() << "Trying to insert:" << insert_before_row << num_rows << parent_model_index;

	std::shared_ptr<AbstractTreeModelItem> parentItem = getItem(parent_model_index);
	bool success;

	beginInsertRows(parent_model_index, insert_before_row, insert_before_row + num_rows - 1);

	// Create @a rows default-constructed children of parent.
	auto new_children = parentItem->insertChildren(insert_before_row, num_rows, m_root_item->columnCount());

	// Add the new children to the UUID lookup map.
	for(const auto& item : new_children)
	{
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

QModelIndex AbstractTreeModel::parent(const QModelIndex &index) const
{
	// std::unique_lock write_lock(m_rw_mutex);

	// Check index but don't touch parent, since per Qt docs that would make this go recursive.
    // Q_ASSERT(checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::DoNotUseParent));
    // Return invalid parent index for invalid index.
	if (!index.isValid())
	{
        return QModelIndex();
	}

	auto child_uuincd_int = index.internalId();
	UUIncD child_uuincd = UUIncD(child_uuincd_int);

	std::shared_ptr<AbstractTreeModelItem> childItem = getItemById(child_uuincd);
	std::shared_ptr<AbstractTreeModelItem> parentItem = childItem->parent_item().lock();

	Q_ASSERT(parentItem);

	if (parentItem == m_root_item)
	{
		return QModelIndex();
	}

	return createIndex(parentItem->childNumber(), 0, quintptr(parentItem->getId()));
}

bool AbstractTreeModel::removeColumns(int position, int columns, const QModelIndex& parent_model_index)
{
    bool success;

    beginRemoveColumns(parent_model_index, position, position + columns - 1);
	success = m_root_item->removeColumns(position, columns);
    endRemoveColumns();

	if (m_root_item->columnCount() == 0)
	{
        removeRows(0, rowCount());
	}

    return success;
}

bool AbstractTreeModel::removeRows(int remove_start_row, int num_rows, const QModelIndex& parent_item_index)
{
	if(num_rows == 0)
	{
		qWr() << "Attempt to remove zero children";
		return false;
	}

	std::shared_ptr<AbstractTreeModelItem> parentItem = getItemById(static_cast<UUIncD>(parent_item_index.internalId()));
    bool success = true;

    beginRemoveRows(parent_item_index, remove_start_row, remove_start_row + num_rows - 1);
    success = parentItem->removeChildren(remove_start_row, num_rows);
    endRemoveRows();

	return success;
}

bool AbstractTreeModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild)
{
	// Defer to base class.
	return this->BASE_CLASS::moveRows(sourceParent, sourceRow, count, destinationParent, destinationChild);
}

bool AbstractTreeModel::moveColumns(const QModelIndex& sourceParent, int sourceColumn, int count, const QModelIndex& destinationParent, int destinationChild)
{
	// Defer to base class.
	return this->BASE_CLASS::moveColumns(sourceParent, sourceColumn, count, destinationParent, destinationChild);
}


QModelIndex AbstractTreeModel::getIndexFromItem(const std::shared_ptr<AbstractTreeModelItem>& item, int column) const
{
	// std::unique_lock read_lock(m_rw_mutex);

	Q_CHECK_PTR(item);
	if(item == m_root_item)
	{
		return QModelIndex();
	}
	if (auto ptr = item->parent_item().lock())
	{
		auto parent_index = getIndexFromItem(ptr);
		return index(item->childNumber(), column, parent_index);
	}
	return QModelIndex();
}

QModelIndex AbstractTreeModel::getIndexFromId(UUIncD id) const
{
	// std::unique_lock read_lock(m_rw_mutex);

	if(id == m_root_item->getId())
	{
		return QModelIndex();
	}
	Q_ASSERT(m_model_item_map.count(id) > 0);
	if(auto ptr = m_model_item_map.at(id).lock())
	{
		return getIndexFromItem(ptr);
	}
	Q_ASSERT(0);
	return QModelIndex();
}

int AbstractTreeModel::rowCount(const QModelIndex &parent) const
{
	// std::unique_lock read_lock(m_rw_mutex);

	// Only column 0 has children, and hence a non-zero rowCount().
    if(parent.isValid() && (parent.column() != 0))
	{
		return 0;
	}
	std::shared_ptr<AbstractTreeModelItem> parent_item;
	if(!parent.isValid())
	{
		Q_CHECK_PTR(m_root_item);
		parent_item = m_root_item;
	}
	else
	{
/// @todo Fixed? M_WARNING("FIXME: We still get here before parent has been added to the model.");
		parent_item = getItemById(UUIncD(parent.internalId()));
//		parent_item = getItem(parent);

	}

	return parent_item->childCount();
}

bool AbstractTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	// std::unique_lock write_lock(m_rw_mutex);

	// setData() expects a valid index.
	// From Qt docs:  https://doc.qt.io/qt-6/qabstractitemmodel.html#checkIndex
	/// "A legal model index is either an invalid model index, or a valid model index for which all the following holds:
	/// the index' model is this;
	/// the index' row is greater or equal than zero;
	/// the index' row is less than the row count for the index' parent;
	/// the index' column is greater or equal than zero;
	/// the index' column is less than the column count for the index' parent.
	/// The options argument may change some of these checks."

	Q_ASSERT(checkIndex(index, CheckIndexOption::IndexIsValid));

	if (role != Qt::EditRole)
	{
        return false;
	}

	std::shared_ptr<AbstractTreeModelItem> item = getItem(index);
    bool result = item->setData(index.column(), value);

	if(result)
	{
		Q_EMIT dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
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
    	// Docs: "If you are changing the number of columns or rows you do not need to emit this signal,
    	// but use the begin/end functions."
		Q_EMIT headerDataChanged(orientation, section, section);
	}

	return result;
}

bool AbstractTreeModel::setHeaderData(int section, const AbstractHeaderSection& header_section)
{
	Q_ASSERT(0);
//	this->setHeaderData(header_section.section(),
//			header_section.orientation(), header_section[role], role);
//	for()
	return true;
}








