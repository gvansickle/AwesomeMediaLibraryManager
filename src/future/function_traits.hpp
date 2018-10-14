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
 * @file
 * Analog to the C++11+ <type_traits> header, for determining function type traits at compile time for SFINAE.
 */

#ifndef UTILS_CONCURRENCY_FUNCTION_TRAITS_HPP_
#define UTILS_CONCURRENCY_FUNCTION_TRAITS_HPP_

#pragma once

#include <config.h>

// Stc C++.
#include <cstddef>
#include <type_traits>
#include <tuple>

// Include some type_traits from the future (i.e. C++17+).
#include "future_type_traits.hpp"

// Boost Callable Traits.
/// This is me giving up on trying to reinvent the function_traits wheel.
#include <boost/callable_traits.hpp>

namespace ct = boost::callable_traits;

/**
 * Non-specialization for all callables.
 *
 * function_traits<decltype(function)> contains the following members:
 * - ::arity_v        The number of arguments the function takes.
 * - ::return_type_t  The function's return type.
 * - ::arg_t<N>       The type of the function's Nth argument, 0-based.
 * - ::argtype_is_v<N, T>  true if arg_t<N> is of type T.
 * - ::return_type_is_v<T> true if return_type_t is type T.
 */
template<class T>
struct function_traits
{
	// Note that ct::args_t will return "std::tuple<>" (i.e. 0-length tuple) for a free function taking void: "void(*)()"
	static constexpr std::size_t arity_v = std::tuple_size_v<ct::args_t<T>>;

	using return_type_t = ct::return_type_t<T>;

    /// Helpers for providing arg_t<N> vs. arg<N>::type.
    template<class... Types>
    struct pack
    {
        static constexpr std::size_t size = sizeof...(Types);
        using to_tuple_plus_void = std::tuple<Types..., void>;
    };
//    template <class Tuple>
//    struct
//    {
//        using append_void_to_tuple_t = std::tuple<>;
//    };

//    template <class Tuple>
//    using sfinae_tuple = decltype(std::tuple_cat(std::declval<Tuple>, std::tuple<void>()));
	template <std::size_t i>
	struct argtype
	{
		using type = std::enable_if_t<arity_v >= i, typename std::tuple_element_t<i, ct::args_t<T>>>;
	};

//    template <std::size_t i>
//	using arg_t = std::enable_if_t<arity_v >= i, typename std::tuple_element_t<i, ct::args_t<T>>>;

	template <std::size_t i>
	using arg_t = typename argtype<i>::type;

    /// For checking if the type of arg N is T.
    template <std::size_t i, class Expected>
    static constexpr bool argtype_is_v = std::is_same_v<arg_t<i>, Expected>;

    /// For checking if the return type is T.
	template <typename Expected>
	static constexpr bool return_type_is_v = std::is_same_v<return_type_t, Expected>;

};

/// @name Convenience templates, when you don't need all the function_traits<>.
/// @{

/**
 * Return type of a function with type F.
 */
template <typename F>
using function_return_type_t = typename function_traits<F>::return_type_t;

template <typename F, typename R>
static constexpr bool function_return_type_is_v = std::is_same_v<function_return_type_t<F>, R>;

/// SFINAE-safe Helper for providing argtype_t<F, N>.
template <class F, std::size_t i>
using argtype_t = std::enable_if_t<function_traits<F>::arity_v >= i, std::tuple_element_t<i, ct::args_t<F>>>;
//using argtype_t = typename function_traits<F>::template arg_t<i>;
//using argtype_t = typename function_traits<F&&>::arg_t<i>;

/// For checking if the type of arg N is T.
template <class F, std::size_t i, class Expected>
static constexpr bool argtype_n_is_v = std::is_same_v<argtype_t<F, i>, Expected>;

/// Numer of args.
template <class F>
struct arity
{
	using arity_v = typename std::tuple_size<ct::args_t<F>>::value;
};

/**
 * Returns the number arguments (including the @c this pointer if applicable) taken by function @a F.
 */
template <class F>
static constexpr std::size_t arity_v = std::tuple_size<ct::args_t<F>>::value;


/// For getting R from T<R>.
template <class T>
struct contained_type_impl
{
	using type = T;
};

template <template<typename> class T, typename R>
struct contained_type_impl<T<R>>
{
	using type = typename contained_type_impl<R>::type;
};

template <class T>
using contained_type_t = typename contained_type_impl<T>::type;

/// @} // Convenience templates.

#endif /* UTILS_CONCURRENCY_FUNCTION_TRAITS_HPP_ */
