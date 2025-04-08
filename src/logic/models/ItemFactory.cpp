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

#include "ItemFactory.h"

#include <QMetaType>
#include <QDebug>

#include "ScanResultsTreeModelItem.h"
#include "AbstractTreeModelHeaderItem.h"
#include "SRTMItemLibEntry.h"


struct ItemCreatorRegistration
{
	ItemCreatorRegistration()
	{
		ItemFactory::instance().registerItemCreator("AbstractTreeModelItem", []()
		{
            return AbstractTreeModelItem::create();
		});
		ItemFactory::instance().registerItemCreator("AbstractTreeModelHeaderItem", []()
		{
            return AbstractTreeModelHeaderItem::create();
		});
		ItemFactory::instance().registerItemCreator("ScanResultsTreeModelItem", []()
		{
            return ScanResultsTreeModelItem::create();
		});
		ItemFactory::instance().registerItemCreator("SRTMItem_LibEntry", []()
		{
            return SRTMItem_LibEntry::create();
		});
	}
};

static ItemCreatorRegistration f_itemCreatorRegistration;

ItemFactory& ItemFactory::instance()
{
	static ItemFactory instance;
	return instance;
}

void ItemFactory::registerItemCreator(const std::string classname, Creator creator)
{
	m_creators[classname] = creator;
}

std::shared_ptr<AbstractTreeModelItem> ItemFactory::createItem(std::string classname) const
{
	if (m_creators.contains(classname))
	{
		return m_creators[classname]();
	}
	else
	{
		Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown Item class name");
	}
}
