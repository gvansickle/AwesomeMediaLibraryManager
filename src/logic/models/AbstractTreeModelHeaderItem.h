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

#ifndef SRC_LOGIC_MODELS_ABSTRACTTREEMODELHEADERITEM_H_
#define SRC_LOGIC_MODELS_ABSTRACTTREEMODELHEADERITEM_H_

/**
 * @file AbstractTreeModelHeaderItem.h
 */

// Std C++
#include <vector>
#include <deque>

// Qt
#include <QVector>
#include <QVariant>

// Ours
#include <future/enable_shared_from_this_virtual.h>
#include "AbstractTreeModelItem.h"
#include "AbstractHeaderSection.h"
#include "ScanResultsTreeModelItem.h"
class AbstractTreeModel;
#include <models/ColumnSpec.h>


/**
 * Type representing a tree model's invisible root item which also doubles as the model's header item.
 * KDEN doesn't use a special derived class for this, just the base class.
 */
class AbstractTreeModelHeaderItem: public AbstractTreeModelItem, public enable_shared_from_this_virtual<AbstractTreeModelHeaderItem>
{
	using BASE_CLASS = AbstractTreeModelItem;

protected:

	friend class AbstractTreeModel;

	/**
     * Note: This type is always the root item of a tree model, no parent item.
	 * @param column_specs
	 * @param parent_model
	 * @param id
	 */
    AbstractTreeModelHeaderItem(std::initializer_list<ColumnSpec> column_specs = {},
	                                     const std::shared_ptr<AbstractTreeModel>& parent_model = nullptr);
protected:
    AbstractTreeModelHeaderItem();

public:
    /**
     * Named constructor.
     */
    static std::shared_ptr<AbstractTreeModelHeaderItem> create(std::initializer_list<ColumnSpec> column_specs = {},
                                                                  const std::shared_ptr<AbstractTreeModel>& parent_model = nullptr);
	~AbstractTreeModelHeaderItem() override;

	void clear() override;

	 /**
	  * Replaces any existing column_specs with the given @a column_specs.
	  * @warning This must be called before any child items are added to the model.
	  */
	virtual bool setColumnSpecs(std::vector<ColumnSpec> column_specs);

	QVariant data(int column, int role = Qt::DisplayRole) const override;

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

	std::shared_ptr<AbstractHeaderSection> getHeaderSection(int column);

private:

	/**
	 * This header's (and hence the model's) ColumnSpecs.
	 */
//	std::vector<ColumnSpec> m_column_specs;

};

Q_DECLARE_METATYPE(AbstractTreeModelHeaderItem);
Q_DECLARE_METATYPE(std::shared_ptr<AbstractTreeModelHeaderItem>)

#endif /* SRC_LOGIC_MODELS_ABSTRACTTREEMODELHEADERITEM_H_ */
