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
 * @file DebugHelpers.cpp
 */

#include "DebugHelpers.h"

#include <string>

std::string class_name(const char * pretty_function)
{
	std::string pf { pretty_function };
	auto end_of_colons = pf.find_last_of(':');

	if(end_of_colons < 3)
	{
		return "CLASS NAME ERROR: " + pf;
	}

	return pf.substr(0, end_of_colons-2);
}



QDebug& operator<<(QDebug& d, const std::string& s)
{
	return d << toqstr(s);
}
