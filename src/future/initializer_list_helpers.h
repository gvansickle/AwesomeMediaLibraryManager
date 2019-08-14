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
 * @file initializer_list_helpers.h
 */
#ifndef SRC_FUTURE_INITIALIZER_LIST_HELPERS_H_
#define SRC_FUTURE_INITIALIZER_LIST_HELPERS_H_

// Std C++
#include <initializer_list>
#include <vector>

/**
 * Some helpers for handling the std::initializer_list<> object.
 */

template <class T>
std::vector<T> to_vector(const std::initializer_list<T> &init_list)
{
	std::vector<T> retval;

	retval.insert(retval.end(), init_list.begin(), init_list.end());

	return retval;
}

#endif /* SRC_FUTURE_INITIALIZER_LIST_HELPERS_H_ */
