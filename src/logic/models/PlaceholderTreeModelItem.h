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
 * @file PlaceholderTreeModelItem.h
 */
#ifndef SRC_LOGIC_MODELS_PLACEHOLDERTREEMODELITEM_H_
#define SRC_LOGIC_MODELS_PLACEHOLDERTREEMODELITEM_H_

// Ours
#include "AbstractTreeModelItem.h"
class AbstractTreeModel;

/*
 *
 */
class PlaceholderTreeModelItem : public AbstractTreeModelItem, public enable_shared_from_this_virtual<PlaceholderTreeModelItem>
{
public:
	/**
	 * Named constructor.
	 */
//	static std::shared_ptr<PlaceholderTreeModelItem> construct(std::vector<QVariant> data, std::shared_ptr<AbstractTreeModel> model,
//	                                                           bool isRoot = false, UUIncD id = UUIncD::null());

//protected:
	/// Sets the model and UUIncD.
//	PlaceholderTreeModelItem(std::vector<QVariant> data, const std::shared_ptr<AbstractTreeModel>& model,
//	                         bool is_root = false, UUIncD id = UUIncD::null());
	explicit PlaceholderTreeModelItem(const std::vector<QVariant>& data, const std::shared_ptr<AbstractTreeModelItem>& parent = nullptr, UUIncD id = UUIncD::null());

public:
	~PlaceholderTreeModelItem() override = default;

protected:
//	QVector<QVariant> m_item_data;

private:
};

#endif /* SRC_LOGIC_MODELS_PLACEHOLDERTREEMODELITEM_H_ */
