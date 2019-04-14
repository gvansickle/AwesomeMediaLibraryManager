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
 * @file ExtFuture_when_all.h
 */
#ifndef SRC_CONCURRENCY_IMPL_EXTFUTURE_WHEN_ALL_H_
#define SRC_CONCURRENCY_IMPL_EXTFUTURE_WHEN_ALL_H_

// Std C++
#include <vector>
#include <type_traits>
#include <iterator>


template <class T>
class ExtFuture;

/**
 * C++2x/concurrency TS when_all() implementation for ExtFuture<T>s.
 * @link https://en.cppreference.com/w/cpp/experimental/when_all
 *
 * @todo Probably should be in ::detail.
 *
 * @tparam InputIterator
 * @param first
 * @param last
 * @return
 */
template <class InputIterator>
ExtFuture<std::vector<typename std::iterator_traits<InputIterator>::value_type>>
when_all(InputIterator first, InputIterator last)
{

}

template <class... Futures>
ExtFuture<std::tuple<std::decay_t<Futures>...>> when_all(Futures&&... futures)
{

}

#endif /* SRC_CONCURRENCY_IMPL_EXTFUTURE_WHEN_ALL_H_ */
