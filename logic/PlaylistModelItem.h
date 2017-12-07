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

#ifndef PLAYLISTMODELITEM_H
#define PLAYLISTMODELITEM_H

#include "LibraryEntry.h"

class PlaylistModelItem: public LibraryEntry
{
public:
	PlaylistModelItem() : LibraryEntry() {}
	PlaylistModelItem(const LibraryEntry& other);
	PlaylistModelItem(const PlaylistModelItem& other);
	~PlaylistModelItem() override;

	static std::shared_ptr<PlaylistModelItem> createFromLibraryEntry(const LibraryEntry* item);

	int m_user_rating {0};
	bool m_is_blacklisted {false};
};

/// So we can more easily pass ptrs in QVariants.
Q_DECLARE_METATYPE(PlaylistModelItem*);
Q_DECLARE_METATYPE(std::shared_ptr<PlaylistModelItem>);

#endif // PLAYLISTMODELITEM_H
