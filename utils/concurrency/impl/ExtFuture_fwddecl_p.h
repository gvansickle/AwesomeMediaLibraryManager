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

#ifndef UTILS_CONCURRENCY_IMPL_EXTFUTURE_FWDDECL_P_H_
#define UTILS_CONCURRENCY_IMPL_EXTFUTURE_FWDDECL_P_H_

template <typename T>
struct isExtFuture : std::false_type
{
	/// @todo Not sure this is correct.
	using inner_t = void;
};

/// Returns the T from an ExtFuture<T> as isExtFuture<T>::inner_t.
template <typename T>
struct isExtFuture<ExtFuture<T>> : std::true_type
{
	using inner_t = T;
};

template <typename T>
static constexpr bool isExtFuture_v = isExtFuture<T>::value;


#endif /* UTILS_CONCURRENCY_IMPL_EXTFUTURE_FWDDECL_P_H_ */
