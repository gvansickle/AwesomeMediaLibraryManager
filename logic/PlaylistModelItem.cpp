/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "PlaylistModelItem.h"

PlaylistModelItem::PlaylistModelItem(const LibraryEntry& other) : LibraryEntry(other), m_user_rating(0)
{

}

PlaylistModelItem::PlaylistModelItem(const PlaylistModelItem& other) : LibraryEntry(other), m_user_rating(other.m_user_rating)
{
}

PlaylistModelItem::~PlaylistModelItem()
{
	// Just here to make sure the class has a vtable.
}

std::shared_ptr<PlaylistModelItem> PlaylistModelItem::createFromLibraryEntry(std::shared_ptr<LibraryEntry> item)
{
	std::shared_ptr<PlaylistModelItem> pitem;

	// First let's make sure item isn't already a PlaylistModelItem*.
	//const PlaylistModelItem* pitem_ptr = dynamic_cast<const PlaylistModelItem*>(item);
	const std::shared_ptr<PlaylistModelItem> pitem_ptr = std::dynamic_pointer_cast<PlaylistModelItem>(item);
	if(pitem_ptr)
	{
		// item is really a PlaylistModelEntry.  Forward to the copy constructor.
		pitem = std::make_shared<PlaylistModelItem>(*pitem_ptr);
	}
	else
	{
		// item is a LibraryEntry.
		pitem = std::make_shared<PlaylistModelItem>(*item);
	}
	return pitem;
}

std::vector<std::shared_ptr<PlaylistModelItem>>
toNewPlaylistModelItems(const std::vector<std::shared_ptr<LibraryEntry>>& libentries)
{
	std::vector<std::shared_ptr<PlaylistModelItem>> retval;

	for(auto i : libentries)
	{
		auto playlist_model_item = std::make_shared<PlaylistModelItem>(*i);
		if(!playlist_model_item)
		{
			qCritical() << "COULD NOT CONVERT LibraryEntry" << i << "TO PlaylistModelItem";
			continue;
		}
		retval.push_back(playlist_model_item);
	}

	return retval;
}

std::vector<std::shared_ptr<LibraryEntry>>
toLibraryEntrySharedPtrs(const std::vector<std::shared_ptr<PlaylistModelItem>> playlist_model_items)
{
	std::vector<std::shared_ptr<LibraryEntry>> retval;

	std::transform(playlist_model_items.begin(), playlist_model_items.end(),
				   std::back_inserter(retval), [](auto pitem) -> std::shared_ptr<LibraryEntry> {
												return std::dynamic_pointer_cast<LibraryEntry>(pitem); });

	return retval;
}
