/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
 * @file PlaceholderTreeModelItem.cpp
 */
#include "PlaceholderTreeModelItem.h"


//std::shared_ptr<PlaceholderTreeModelItem> PlaceholderTreeModelItem::construct(std::vector<QVariant> data, std::shared_ptr<AbstractTreeModel> model, bool isRoot, UUIncD id)
//{
//	std::shared_ptr<PlaceholderTreeModelItem> self(new PlaceholderTreeModelItem(data, model));
//
//	baseFinishConstruct(self);
//	return self;
//}
PlaceholderTreeModelItem::PlaceholderTreeModelItem(const std::vector<QVariant>& data, const std::shared_ptr<AbstractTreeModel>& model, UUIncD id)
	: AbstractTreeModelItem(data, model, id)
{

}

//PlaceholderTreeModelItem::PlaceholderTreeModelItem(std::vector<QVariant> data, const std::shared_ptr<AbstractTreeModel>& model,
//                                                   bool is_root, UUIncD id)
//	: AbstractTreeModelItem(data, model, is_root, id)
//{
//
//}



