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

// Forward declare the ExtAsync namespace
namespace ExtAsync { namespace detail {} }

template <class T>
class ExtFuture;

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

/**
 * @returns true if T is an ExtFuture.
 */
template <typename T>
constexpr bool is_ExtFuture_v = isExtFuture<T>::value;

/**
 * The type T of an ExtFuture<T>.
 */
template <typename T>
using ExtFuture_inner_t = typename isExtFuture<T>::inner_t;

/// Our "ExtFuture" concepts, helper templates, etc.
template <class T>
constexpr bool IsExtFuture = require<concepts::Class<T>, is_ExtFuture_v<T>>;

template <class T>
constexpr bool NonNestedExtFuture = require<
		IsExtFuture<T>,
		!IsExtFuture<ExtFuture_inner_t<T>>
	>;

template <class T>
constexpr bool NestedExtFuture = require<
		IsExtFuture<T>,
		IsExtFuture<ExtFuture_inner_t<T>>
	>;

template <class T>
constexpr bool is_nested_ExtFuture_v = NestedExtFuture<T>;

template <class T>
constexpr bool is_non_void_non_ExtFuture_v = require<!std::is_void_v<T> && !is_ExtFuture_v<T>>;


template <class F, class T>
using has_extfuture_as_first_param_type = decltype(std::declval<F>()(std::declval<ExtFuture<T>>()));

template <class F, class T>
using has_extfuture_as_first_param = future_detection::is_detected<has_extfuture_as_first_param_type, F, T>;

template <class F, class T>
using has_extfuture_ref_as_first_param_type = decltype(std::declval<F>()(std::declval<ExtFuture<T>&>()));

template <class F, class T>
using has_extfuture_ref_as_first_param = future_detection::is_detected<has_extfuture_ref_as_first_param_type, F, T>;

/**
 * .then() helper for ExtFuture implicit unwrapping.  Determines the type of the ExtFuture returned by .then().
 *
 * We're somewhat following the latest standards thinking here:
 * @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
 * "Let U be the return type of the continuation [...].  If U is std::experimental::future<T2> for some type T2,
 * then the return type of then is std::experimental::future<T2>, [else it's]
 * std::experimental::future<U>."
 * Main difference is no differentiation between a future and a shared_future (they're all shared).
 *
 * Not sure, at one point .then() was specced to only do a single level of unwrapping.
 *
 * @tparam U  Return type of the continuation passed to .then().
 * @returns   Return type of the .then() member function.
 */
template <class U>
using then_return_future_type_t = std::conditional_t<is_ExtFuture_v<U>, // == U::inner_t == T2.
        /* true */
        ExtFuture<ExtFuture_inner_t<U>>, // Unwrapping, T2 should be a non-ExtFuture?
        /* false */
        ExtFuture<U> // Non-nested case, no unwrapping.
        >;
/**
 * .then() helper, returns the ExtFuture type which should be returned by a .then() call,
 * based on the return type of the .then() callback.
 * @note If callback returns void, it will be lifted to Unit here.
 */
template <class ThenCallbackType, class ExtFutureType>
using then_return_type_from_callback_and_future_t = then_return_future_type_t<Unit::LiftT<
        std::invoke_result_t<ThenCallbackType, ExtFutureType>
        >>;

#endif /* UTILS_CONCURRENCY_IMPL_EXTASYNC_TRAITS_H_ */
