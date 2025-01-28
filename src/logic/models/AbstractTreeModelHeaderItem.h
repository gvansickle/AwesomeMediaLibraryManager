/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
 * @file AbstractTreeModelHeaderItem.h
 */
#ifndef SRC_LOGIC_MODELS_ABSTRACTTREEMODELHEADERITEM_H_
#define SRC_LOGIC_MODELS_ABSTRACTTREEMODELHEADERITEM_H_

// Std C++
#include <vector>

// Qt5
#include <QVector>
#include <QVariant>
#include <logic/ColumnSpec.h>

// Ours
#include "AbstractTreeModelItem.h"
#include "AbstractHeaderSection.h"
#include "ScanResultsTreeModelItem.h"
class AbstractTreeModel;

/**
 * Abstract base class for tree model header items.
 */
class AbstractTreeModelHeaderItem : public AbstractTreeModelItem
{
	using BASE_CLASS = AbstractTreeModelItem;

public:
	explicit AbstractTreeModelHeaderItem(AbstractTreeModel* parent_model, bool is_root = false);
	~AbstractTreeModelHeaderItem() override;

	 /**
	  * @warning This must be called before any child items are added to the model.
	  * @param column_specs
	  * @return
	  */
	virtual bool setColumnSpecs(std::initializer_list<QString> column_specs);

	QVariant data(int column, int role = Qt::DisplayRole) const override;

	int columnCount() const override;


	/// @name Serialization
	/// @{

	/**
	 * Serialize this item and any children to a QVariant.
	 * Override this in derived classes to do the right thing.
	 */
	QVariant toVariant() const override;

	void fromVariant(const QVariant& variant) override;

	/// @}

protected:

	/// @todo THESE do_create_'s ARE ALL NOW TAKING A MODEL vs. AN ITEM, SO ARE CERTAINLY BROKEN.
	ScanResultsTreeModelItem* do_create_default_constructed_child_item(AbstractTreeModel *parent_model, int num_columns);

	/// @name Virtual functions called by the base class to complete certain operations.
	///       The base class will have error-checked function parameters.
	/// @{
	bool derivedClassSetData(int column, const QVariant &value) override;
	bool derivedClassInsertColumns(int insert_before_column, int num_columns) override;
	bool derivedClassRemoveColumns(int first_column_to_remove, int num_columns) override;
	/// @}

	/// @todo This is where we're ultimately headed, but QStrings in the interim.
//	std::vector<ColumnSpec> m_column_specs;
	std::vector<QString> m_column_specs {};

	// The model we belong to.
	/// @note Not sure we actually need this for anything.
	AbstractTreeModel* m_parent_model;
};

#endif /* SRC_LOGIC_MODELS_ABSTRACTTREEMODELHEADERITEM_H_ */
