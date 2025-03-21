/*
* Copyright 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef ITEMFACTORY_H
#define ITEMFACTORY_H

// Std C++
#include <memory>
#include <functional>
#include <string>

// Qt
#include <QMap>

// Ours.
#include "AbstractTreeModelItem.h"


class ItemFactory
{
public:
  	using Creator = std::function<std::unique_ptr<AbstractTreeModelItem>()>;

	static ItemFactory& instance();

	void registerItemCreator(const std::string classname, Creator creator);

	std::unique_ptr<AbstractTreeModelItem> createItem(std::string classname) const;

private:
	QMap<std::string, Creator> m_creators;
};


#endif //ITEMFACTORY_H
