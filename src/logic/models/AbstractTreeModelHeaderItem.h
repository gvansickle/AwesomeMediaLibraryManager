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
#include <future/enable_shared_from_this_virtual.h>
#include "AbstractTreeModelItem.h"
#include "AbstractHeaderSection.h"
#include "ScanResultsTreeModelItem.h"
class AbstractTreeModel;

/**
 * Type representing a tree model's invisible root item which also doubles as the model's header item.
 */
class AbstractTreeModelHeaderItem: public AbstractTreeModelItem, public enable_shared_from_this_virtual<AbstractTreeModelHeaderItem>
{
	using BASE_CLASS = AbstractTreeModelItem;

protected:
	explicit AbstractTreeModelHeaderItem(const std::shared_ptr<AbstractTreeModel>& parent_model, bool isRoot, UUIncD id = UUIncD::null());

public:
	/**
	 * Named constructor.
	 */
	static std::shared_ptr<AbstractTreeModelHeaderItem> construct(const std::shared_ptr<AbstractTreeModel>& model, bool isRoot = true,
	                                                              UUIncD id = UUIncD::null());
	AbstractTreeModelHeaderItem() {};
	~AbstractTreeModelHeaderItem() override;

	 /**
	  * @warning This must be called before any child items are added to the model.
	  * @param column_specs
	  * @return
	  */
	virtual bool setColumnSpecs(std::initializer_list<QString> column_specs);

	QVariant data(int column, int role = Qt::DisplayRole) const override;

//	int columnCount() const override;


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

};

Q_DECLARE_METATYPE(AbstractTreeModelHeaderItem);
Q_DECLARE_METATYPE(std::shared_ptr<AbstractTreeModelHeaderItem>)

#endif /* SRC_LOGIC_MODELS_ABSTRACTTREEMODELHEADERITEM_H_ */
