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
 * @file when_any.h
 */
#ifndef SRC_CONCURRENCY_IMPL_EXTFUTURE_WHEN_ANY_H_
#define SRC_CONCURRENCY_IMPL_EXTFUTURE_WHEN_ANY_H_

// Std C++
#include <type_traits>
#include <vector>

template <class T>
class ExtFuture;

namespace ExtAsync
{

template < class Sequence >
struct when_any_result
{
	std::size_t index;
	Sequence futures;
};

template <class InputIt>
auto when_any(InputIt first, InputIt last)
-> ExtFuture<when_any_result<std::vector<typename std::iterator_traits<InputIt>::value_type>>>;

template < class... Futures >
auto when_any(Futures&&... futures)
-> ExtFuture<when_any_result<std::tuple<std::decay_t<Futures>...>>>;

} // END namespace ExtAsync

#endif /* SRC_CONCURRENCY_IMPL_EXTFUTURE_WHEN_ANY_H_ */
