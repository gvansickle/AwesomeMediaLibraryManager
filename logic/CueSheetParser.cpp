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

#include "CueSheetParser.h"

#include <QtGlobal>
#include <utils/StringHelpers.h>


CueSheetParser::CueSheetParser()
{

}

Cd *CueSheetParser::parse_cue_sheet_string(const char *bytes)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// libcue (actually flex) can't handle invalid UTF-8.
	Q_ASSERT_X(isValidUTF8(bytes), __func__, "Invalid UTF-8 cuesheet string.");

	Cd* cd = cue_parse_string(bytes);

	Q_ASSERT_X(cd != nullptr, "cuesheet", "failed to parse cuesheet string");

	return cd;
}
