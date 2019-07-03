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
 * @file future_algorithms.h
 */
#ifndef SRC_FUTURE_FUTURE_ALGORITHMS_H_
#define SRC_FUTURE_FUTURE_ALGORITHMS_H_

/**
 * Library Fundamentals v2.
 */

/// @name Uniform container erasure.
/// These really come from the specific containers' headers, not <algorithms>, but I don't want to make a hundred headers for this.
/// @{

#if __has_include(<experimental/deque>)
#include <experimental/deque>
#else
#define BACKFILL 1
#endif
#if __has_include(<experimental/vector>)
#include <experimental/vector>
#else
#define BACKFILL 1
#endif

#if BACKFILL == 1
// Backfill.
#include <deque>
#include <vector>
namespace std // Yeah yeah.
{
namespace experimental
{

template <class ContainerType, class T, class A, class U>
void erase(ContainerType<T, A>& c, const U& value)
{
	c.erase(std::remove(c.begin(), c.end(), value), c.end());
}

template <class ContainerType, class T, class Alloc, class Pred >
void erase_if(ContainerType<T, Alloc>& c, Pred pred)
{
	c.erase(std::remove_if(c.begin(), c.end(), pred), c.end());
}

} // END experimental

} // END std
#endif

/// @}


#endif /* SRC_FUTURE_FUTURE_ALGORITHMS_H_ */
