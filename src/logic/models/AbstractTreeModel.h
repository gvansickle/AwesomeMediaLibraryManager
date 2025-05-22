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

#ifndef ABSTRACTTREEMODEL_H
#define ABSTRACTTREEMODEL_H
/**
 * @file AbstractTreeModel.h
 * Interface of AbstractTreeModel.
 */

// Std C++
#include <memory>
#include <vector>
#include <map>

// Qt
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
class QAbstractItemModelTester;

// Ours
class AbstractTreeModelItem;
class AbstractHeaderSection;
class AbstractTreeModelHeaderItem;
class ColumnSpec;
#include <logic/serialization/ISerializable.h>
#include <logic/UUIncD.h>
#include <future/enable_shared_from_this_virtual.h>
#include "UndoRedoHelper.h"


/**
 * Abstract tree model base class.  Inherits from QAbstractItemModel and ISerializable.
 * This class is heavily adapted from at least the following:
 * - The "Editable Tree Model Example" shipped with Qt5.
 * - KDenLive's AbstractItemModel class.
 * - My own original work.
 * - Hundreds of nuggets of information from all over the Internet.
 */
class AbstractTreeModel : public QAbstractItemModel, public virtual ISerializable, public enable_shared_from_this_virtual<AbstractTreeModel>
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractTreeModel)
    Q_INTERFACES(ISerializable)

	using BASE_CLASS = QAbstractItemModel;

protected:
	/**
	 * My bright idea here is to defer to this constructor from other constructors which need to call virtual functions on this.
	 * I have no idea if that really works like I think it does.
	 * Wait, yes I do: @link https://docs.microsoft.com/en-us/cpp/cpp/constructors-cpp?view=vs-2019#delegating_constructors
	 * "The object created by the constructors is fully initialized as soon as any constructor is finished."
	 */
	explicit AbstractTreeModel(QObject* parent = nullptr);
protected:
	/**
	 * Creates a new AbstractTreeModel object.
	 * @warning This model will NOT have a root item because virtual.  See setColumnSpecs(), which you should
	 *          call immediately after creating a new model.
	 * In general, derived constructors don't do much more than pass the @a parent param.
	 */
    AbstractTreeModel(std::initializer_list<ColumnSpec> column_specs, QObject *parent = nullptr);

public:
	/**
	 * Named constructor.
	 */
	static std::shared_ptr<AbstractTreeModel> create(std::initializer_list<ColumnSpec> column_specs, QObject *parent = nullptr);

	/**
	 * Destructor.  Clears all items in the model, including the root item.
	 */
	~AbstractTreeModel() override;

public Q_SLOTS:
	/**
	 * Append a child item to the root item.  FBO non-GUI threads.
	 * @param child  The child to append to the root item.
	 */
	void SLOT_appendChildToRoot(std::shared_ptr<AbstractTreeModelItem> child);

	void SLOT_appendChild(std::shared_ptr<AbstractTreeModelItem> child, UUIncD parent_id);

public:
	/// GRVS/KDEN's is ProjItemModel::clean().
	/**
	 * Clears all data from the model.
	 * May need to be overridded in derived classes.
	 */
    virtual void clear(bool quit = false);

	/**
	 * Sets the ColumnSpecs which hold the info for the horizontal header.
	 */
	virtual bool setColumnSpecs(std::vector<ColumnSpec> column_specs);

	// bool hasIndex() is not virtual.

	/// BOTH
	/**
	 * Calls getItem(index), which returns index.internalPointer() which is an AbstractTreeModelItem*.
	 * Item then returns the data for this index and role from its @a data(column) function.
	 *
	 * @todo role gets lost along the way, we need to put that in.
	 */
    QVariant data(const QModelIndex &index, int role) const override;

    /// @todo Maybe override these.
//	QMap<int, QVariant> itemData(const QModelIndex &index) const override;
//	bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles) override;

    /// Header data interface
    /// @{

    /// Get the header data corresponding to the given section number, orientation, and role.
    /// BOTH
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /// Set the header data corresponding to the given section number, orientation, and role.
    bool setHeaderData(int section, Qt::Orientation orientation,
                         const QVariant &value, int role = Qt::EditRole) override;

    /// Overload taking an AbstractHeaderSection.
    virtual bool setHeaderData(int section, const AbstractHeaderSection& header_section);

    /// @}

    /// BOTH
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
	/// BOTH
	Qt::ItemFlags flags(const QModelIndex &index) const override;

    /// BOTH
    QModelIndex parent(const QModelIndex &index) const override;
	/// BOTH
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * Returns the number of columns for the children of the given parent.
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;


    /// ETM, KDEN AbsTreeModel doesn't override this.
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    /// @name Row and column insert, remove, and move operations.
    /// @note Singular insert/remove/move row and column functions are not virtual
    ///       but are implemented in QAbstractItemModel.
    /// @{

    /**
     * Inserts @a num_columns new columns into the model before column @a insert_before_column.  If
     * @a insert_before_column is 0, columns are still prepended, and if it's columnCount(), they're still prepended
     * to the non-existent one-past-the-end column, i.e. they're appended to the list.
     *
     * @return true on success.
     */
	bool insertColumns(int insert_before_column, int num_columns,
					   const QModelIndex& parent_model_index = QModelIndex()) override;
	/**
	 * Remove columns.
	 */
    bool removeColumns(int remove_start_column, int num_columns,
                       const QModelIndex& parent_model_index = QModelIndex()) override;

    /**
     * Inserts @a count new default-constructed rows under model item @a parent_model_index model before the given row @a insert_before_row.
     * If @a insert_before_row is 0, rows are still prepended.  If it's > rowCount(), operation is meaningless and
     * the call returns false, not having done anything.
     *
     * @return true on success.
     */
	bool insertRows(int insert_before_row, int num_rows,
					const QModelIndex& parent_model_index = QModelIndex()) override;

    /**
     * Remove rows [@a remove_start_row, @a remove_start_row + @a num_rows - 1 ].
     */
    bool removeRows(int remove_start_row, int num_rows,
                    const QModelIndex& parent_item_index = QModelIndex()) override;

    /// @todo These currently just call the base class functions.
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
	                      const QModelIndex &destinationParent, int destinationChild) override;
	bool moveColumns(const QModelIndex &sourceParent, int sourceColumn, int count,
	                         const QModelIndex &destinationParent, int destinationChild) override;

	/// @} // END row/col insert/remove/move.


	/// @name Extended public model interface.
    /// @{

	/// GRVS
	/// Insert an empty new child under @a parent and returns a shared_ptr to it.
	/// ETM: From MainWindow, where parent is always currentIndex() from a selection model.
//	std::shared_ptr<AbstractTreeModelItem> insertChild(const QModelIndex &parent = QModelIndex());
	/// GRVS
	/// ETM-inspired append function.  Based on setupModelData().
	std::shared_ptr<AbstractTreeModelItem> append_child(const QVector<QVariant> &data, std::shared_ptr<AbstractTreeModelItem> parent);

	virtual QModelIndex getIndexFromItem(const std::shared_ptr<AbstractTreeModelItem>& item, int column = 0) const;
	virtual QModelIndex getIndexFromId(UUIncD id) const;
	virtual std::shared_ptr<AbstractTreeModelItem> getItemById(const UUIncD &id) const;
	virtual std::shared_ptr<AbstractTreeModelItem> getRootItem() const;
	// ETM/GRVS/AQP(itemForIndex)
	virtual std::shared_ptr<AbstractTreeModelItem> getItem(const QModelIndex &index) const;


	/// @name Public interface: Lambda generators for tree structure modification.
	/// @{

	/* @brief Helper function to generate a lambda that adds an item to the tree */
	Fun addItem_lambda(const std::shared_ptr<AbstractTreeModelItem>& new_item, UUIncD parentId);

	/* @brief Helper function to generate a lambda that removes an item from the tree */
	Fun removeItem_lambda(UUIncD id);

	/* @brief Helper function to generate a lambda that changes the row of an item */
	Fun moveItem_lambda(UUIncD id, int destRow, bool force = false);

	/// @} // END Lambda generators.


	/// @name Cut/Copy/Paste support.
	/// @{

	/// @}


	/// @name Serialization, from ISerializable.
	/// Remember to override these in derived classes.
	/// @{

	/**
	 * Sets the base directory of the model.
	 */
    virtual void setBaseDirectory(const QUrl& base_directory) {}

	/// Load and save the database to a file.
	/// @note The idea is that these shouldn't need to be overridden in derived classes, but just in case we make
	/// them virtual.
	virtual bool LoadDatabase(const QString& database_filename);
	virtual void SaveDatabase(const QString& database_filename);

	/// Serialize the entire model to a QVariant.
    QVariant toVariant() const override { Q_ASSERT(0); return QVariant(); } // = 0;

	/// Serialize the entire model from a QVariant.
    void fromVariant(const QVariant& variant) override { Q_ASSERT(0); } // = 0;

#if 0
	virtual void toOrm(std::string filename) const;
	virtual void fromOrm(std::string filename);
#endif

	/// @} // END Serialization

	/// @} // END Extended public model interface.

	friend class AbstractTreeModelItem;

	/// @name Debug
	/// @{

	long get_total_model_node_count() const { return m_model_item_map.size(); };

	void dump_model_info() const;

	/// @}

	/// @temp?
	using item_map_type = std::map<UUIncD, std::weak_ptr<AbstractTreeModelItem>>;
	/// Generic node iterator type.  No order guarantees at all.
	using iterator = item_map_type::iterator;
	iterator begin();
	iterator end();

protected:

	/**
	 * Register/deregister an item with the model.  Intended to be called from an AbstractTreeModelItem.
	 */
	virtual void register_item(const std::shared_ptr<AbstractTreeModelItem>& item);
	virtual void deregister_item(UUIncD id, AbstractTreeModelItem* item);

	/// @name Derived-class serialization info.
	/// @{

    virtual void DERIVED_set_default_namespace() {}

//	virtual void DERIVED_clean() {};

	/// @}

	/**
	 * Send the appropriate pre-notification related to a row that we are appending.
	 * @param item is the parent item to which row is about to be appended.
	 */
//	void notifyRowsAboutToInsert(const std::shared_ptr<AbstractTreeModelItem>& row, int first, int last);

	/* @brief Send the appropriate notification related to a row that we have appended
	   @param row is the new element
	*/
//	void notifyRowsInserted(const std::shared_ptr<AbstractTreeModelItem>& row);

	void notifyColumnsAboutToInserted(const std::shared_ptr<AbstractTreeModelItem>& parent, int first_column, int last_column);
	void notifyColumnsInserted();

	/**
	 * Send the appropriate pre-notification related to a row that we are appending.
	 * @param item is the parent item to which row is about to be appended.
	 */
	void notifyRowAboutToAppend(const std::shared_ptr<AbstractTreeModelItem>& item);

	/* @brief Send the appropriate notification related to a row that we have appended
       @param row is the new element
    */
	void notifyRowAppended(const std::shared_ptr<AbstractTreeModelItem>& row);

	/* @brief Send the appropriate notification related to a row that we are deleting
	   @param item is the parent of the row being deleted
	   @param row is the index of the row being deleted
	*/
	void notifyRowAboutToDelete(std::shared_ptr<AbstractTreeModelItem> item, int row);

	/* @brief Send the appropriate notification related to a row that we have appended
	   @param row is the old element
	*/
	void notifyRowDeleted();

public:
	/** This is a convenience function that helps check if the tree is in a valid state */
	virtual bool checkConsistency();

protected:

	void setItemMap(UUIncD id);

    virtual QString getXmlStreamName() const { return ""; }
    virtual QString getXmlStreamVersion() const { return ""; }

	std::string m_default_namespace_decl {"XXXBROKENXXX"};
	std::string m_default_namespace_version {"XXXBROKENXXX"};

    /// @}

    /// Associated model tester.
    /// Parented to the model itself.
    QAbstractItemModelTester* m_model_tester {nullptr};

    /// Hidden root node of the tree model.
    /// Pulls double duty as the horizontal header item.
	std::shared_ptr<AbstractTreeModelHeaderItem> m_root_item;

private:
	/**
	 * Map of UUIncD's to AbstractTreeModelItems.
	 * Currently: std::map<UUIncD, std::weak_ptr<AbstractTreeModelItem>> m_model_item_map;
	 */
	item_map_type m_model_item_map;

};


#endif // ABSTRACTTREEMODEL_H
