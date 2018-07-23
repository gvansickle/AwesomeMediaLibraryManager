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

#ifndef DEDUCED_TYPE_HPP
#define DEDUCED_TYPE_HPP

#include <type_traits>

// Extracted from https://www.boost.org/doc/libs/1_67_0/boost/thread/future.hpp

template <typename T>
struct deduced_type_impl
{
	using type = T;
};

template <typename T>
struct deduced_type_impl<std::reference_wrapper<T>>
{
	using type = T&;
};

template <typename T>
struct deduced_type
{
	using type = typename deduced_type_impl<std::decay_t<T>>::type;
};

template <typename T>
using deduced_type_t = typename deduced_type<T>::type;

#endif // DEDUCED_TYPE_HPP
