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

#include "RegisterQtMetatypes.h"

#include <logic/LibraryEntry.h>
#include <logic/PlaylistModelItem.h>
#include <utils/Fraction.h>

void RegisterQtMetatypes()
{
	// Register the types we want to be able to use in Qt's queued signal and slot connections or in QObject's property system.
	qRegisterMetaType<LibraryEntry>();
	qRegisterMetaType<PlaylistModelItem>();
	qRegisterMetaType<std::shared_ptr<LibraryEntry>>();
	qRegisterMetaType<std::shared_ptr<PlaylistModelItem>>();
	
	// Cast std::shared_ptr<PlaylistModelItem> to std::shared_ptr<LibraryEntry>.
	auto PlaylistModelItemToLibraryEntry = [](const std::shared_ptr<PlaylistModelItem> plmi)
		{
			return std::dynamic_pointer_cast<LibraryEntry>(plmi);
		};
	QMetaType::registerConverter< std::shared_ptr<PlaylistModelItem>, std::shared_ptr<LibraryEntry> >(PlaylistModelItemToLibraryEntry);

	qRegisterMetaType<Fraction>();
	qRegisterMetaTypeStreamOperators<Fraction>("Fraction");
}
