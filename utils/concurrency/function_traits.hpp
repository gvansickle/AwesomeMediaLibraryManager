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

#include <cstddef>
#include <type_traits>
// Include some type_traits from the future (i.e. C++17+).
#include "future_type_traits.hpp"
#include <tuple>

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
	static constexpr std::size_t arity_v = std::tuple_size<ct::args_t<T>>::value;

	using return_type_t = ct::return_type_t<T>;

    /// Helper for providing arg_t<N> vs. arg<N>::type.
	template <std::size_t i>
	using arg_t = typename std::tuple_element_t<i, ct::args_t<T>>;

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

/// Helper for providing argtype_t<F, N>.
template <class F, std::size_t i>
using argtype_t = typename std::tuple_element_t<i, ct::args_t<F>>;

/// For checking if the type of arg N is T.
template <class F, std::size_t i, class Expected>
static constexpr bool argtype_n_is_v = std::is_same_v<argtype_t<F, i>, Expected>;

/// @} // Convenience templates.

#endif /* UTILS_CONCURRENCY_FUNCTION_TRAITS_HPP_ */
