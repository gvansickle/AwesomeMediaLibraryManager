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

#ifndef UTILS_CONCURRENCY_IMPL_EXTASYNC_TRAITS_H_
#define UTILS_CONCURRENCY_IMPL_EXTASYNC_TRAITS_H_

#include <type_traits>
#include <future/future_type_traits.hpp>
#include <future/function_traits.hpp>
#include <future/cpp14_concepts.hpp>
#include <future/Unit.hpp>

template <typename T>
struct isExtFuture : std::false_type
{
	using inner_t = typename Unit::Lift<T>::type;
};


template <typename T>
struct isExtFuture<ExtFuture<T>> : std::true_type
{
	/// The wrapped type of this ExtFuture<T>.
	using inner_t = T;
};

/// Facebook's folly futures call this "isFutureOrSemiFuture<>".
/// There's probably a good reason why, but I don't see it...
/// ...oh wait, there it is: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0783r0.html
/// SemiFutures are futures without continuations.
/// Anyway, this is the primary template, inheriting from std::false_type
/// So, not real clear on why the two types.
template <typename T>
struct isExtFuture2 : std::false_type
{
	using inner_t = typename Unit::Lift<T>::type;
	using return_t = inner_t;
};

template <typename T>
struct isExtFuture2<ExtFuture<T>> : std::true_type
{
	/// The wrapped type of this ExtFuture<T>.
	using inner_t = T;
	using return_t = ExtFuture<T>;
};


template <typename T>
constexpr bool isExtFuture_v = isExtFuture<T>::value;

template <typename T>
using isExtFuture_t = typename isExtFuture<T>::inner_t;

/// Our "ExtFuture" concepts.
template <class T>
constexpr bool IsExtFuture = require<concepts::Class<T>, isExtFuture_v<T>>;

template <class T>
constexpr bool NonNestedExtFuture = require<
		IsExtFuture<T>,
		!IsExtFuture<isExtFuture_t<T>>
	>;

template <class T>
constexpr bool NestedExtFuture = require<
		IsExtFuture<T>,
		IsExtFuture<isExtFuture_t<T>>
	>;


template <class F, class T>
using has_extfuture_as_first_param_type = decltype(std::declval<F>()(std::declval<ExtFuture<T>>()));

template <class F, class T>
using has_extfuture_as_first_param = std::is_detected<has_extfuture_as_first_param_type, F, T>;

template <class F, class T>
using has_extfuture_ref_as_first_param_type = decltype(std::declval<F>()(std::declval<ExtFuture<T>&>()));

template <class F, class T>
using has_extfuture_ref_as_first_param = std::is_detected<has_extfuture_ref_as_first_param_type, F, T>;

/// END concepts

#endif /* UTILS_CONCURRENCY_IMPL_EXTASYNC_TRAITS_H_ */
