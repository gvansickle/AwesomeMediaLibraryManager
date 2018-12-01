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
 * @file TreeModelRootItem.h
 */

#ifndef SRC_LOGIC_MODELS_TREEMODELROOTITEM_H_
#define SRC_LOGIC_MODELS_TREEMODELROOTITEM_H_

// Ours.
#include "AbstractTreeModelItem.h"
#include "ScanResultsTreeModelItem.h" ///< @todo This is wrong, we have a derived item included here in the very root.
class AbstractTreeModel;
class AbstractTreeModelHeaderItem;
class ScanResultsTreeModelItem;

/**
 *
 */
class TreeModelRootItem: public AbstractTreeModelItem
{
public:
	explicit TreeModelRootItem() = default;
	explicit TreeModelRootItem(AbstractTreeModel* parent_model,
							   AbstractTreeModelItem* parent_item = nullptr);
	~TreeModelRootItem() override;


//	QVariant data(int column) const override;




protected:

	/**
	 * Factory function for creating default-constructed nodes.
	 * Used by insertChildren().  Override in derived classes.
	 * @todo Convert to smart pointer (std::unique_ptr<AbstractTreeModelItem>) return type, retain covariant return.
	 */
//	ScanResultsTreeModelItem*
//	create_default_constructed_child_item(AbstractTreeModelItem *parent = nullptr) override;



};

#endif /* SRC_LOGIC_MODELS_TREEMODELROOTITEM_H_ */
