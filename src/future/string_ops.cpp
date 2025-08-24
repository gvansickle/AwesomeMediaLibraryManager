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
/** @file */

#include "string_ops.h"

// C++
#include <cctype>
#include <algorithm>
#include <ranges>

/**
 * Trim whitespace from both ends of a string_view
 * @param str View to the string to trim.
 * @return The trimmed string_view.
 */
std::string trim(std::string_view str)
{
	/// @note This may be subject to locale:
	auto is_space = [](char c) { return std::isspace(c); };
	auto start = std::ranges::find_if_not(str, is_space);
	auto end = std::find_if_not(str.rbegin(), str.rend(), is_space).base();
	return (start < end) ? std::string(start, end) : std::string();
}

std::string trim_quotes(std::string_view str)
{
	// Find the first and last quote chars, if any.
	auto start = str.cbegin();
	auto end = str.crbegin();

	if(*start != '\"' || *end != '\"')
	{
		return std::string(str);
	}

	return (start < end.base()) ? std::string(start + 1, end.base()-1) : std::string(str);
}
