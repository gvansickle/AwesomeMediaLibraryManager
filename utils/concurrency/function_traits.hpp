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
#include "future_type_traits.hpp"
#include <tuple>


/**
 * Forward declaration of the primary template.
 */
template <class F>
struct function_traits;

/**
 * Specialization for regular free functions.
 *
 * function_traits<decltype(function)> contains the following members:
 * - ::arity_v        The number of arguments the function takes.
 * - ::return_type_t  The function's return type.
 * - ::arg_t<N>       The type of the function's Nth argument, 0-based.
 * - ::argtype_is_v<N, T>  true if arg_t<N> is of type T.
 * - ::return_type_is_v<T> true if return_type_t is type T.
 */
template<class ReturnType, class... Args>
struct function_traits<ReturnType(Args...)>
{
    using return_type_t = ReturnType;

    static constexpr std::size_t arity_v = sizeof...(Args);

    template <std::size_t N>
    struct arg
    {
        static_assert(N < arity_v, "Parameter index out of range");
        using type = std::tuple_element_t<N,std::tuple<Args...>>;
    };

    /// Helper for providing arg_t<N> vs. arg<N>::type.
    template <std::size_t N>
    using arg_t = typename arg<N>::type;

    /// For checking if the type of arg N is T.
    template <std::size_t N, typename T>
    static constexpr bool argtype_is_v = std::is_same_v<arg_t<N>, T>;

    /// For checking if the return type is T.
    template <typename T>
    static constexpr bool return_type_is_v = std::is_same_v<return_type_t, T>;
};

/**
 * Specialization for function pointers.
 */
template<class ReturnType, class... Args>
struct function_traits<ReturnType(*)(Args...)> : public function_traits<ReturnType(Args...)> {};

/**
 * Specialization for const function pointers.
 */
template<class ReturnType, class... Args>
struct function_traits<ReturnType(* const)(Args...)> : public function_traits<ReturnType(Args...)> {};

/**
 * Specialization for member function pointers.
 */
template<class Class, class ReturnType, class... Args>
struct function_traits<ReturnType(Class::*)(Args...)> : public function_traits<ReturnType(Class&, Args...)> {};

/**
 * Specialization for const member function pointers.
 */
template<class Class, class ReturnType, class... Args>
struct function_traits<ReturnType(Class::*)(Args...) const> : public function_traits<ReturnType(Class&, Args...)> {};

/**
 * Specialization for member object pointers.
 */
template<class Class, class ReturnType>
struct function_traits<ReturnType(Class::*)> : public function_traits<ReturnType(Class&)> {};

/**
 * Specialization for const pointers to member functions.
 */
template <typename Class, typename ReturnType, typename... Args>
struct function_traits<ReturnType(Class::* const)(Args...)> : public function_traits<ReturnType(Class&, Args...)> {};

/**
 * Specialization for const pointers to const member functions.
 */
template <typename Class, typename ReturnType, typename... Args>
struct function_traits<ReturnType(Class::* const)(Args...) const> : public function_traits<ReturnType(Class&, Args...)> {};

/**
 * Specialization for "Callables", i.e. functors, std::function<>'s, anything with an operator().
 */
template <typename CallableType>
struct function_traits
{
	/// Get the function_traits<> of operator().
	using function_call_operator_t = function_traits<decltype(&CallableType::operator())>;

    using return_type_t = typename function_call_operator_t::return_type_t;

    // Adjust arity to ignore the implicit object pointer.
    static constexpr std::size_t arity_v = function_call_operator_t::arity_v - 1;

    template <std::size_t N>
    struct arg
    {
        static_assert(N < arity_v, "Parameter index out of range");
        // Again, adjusting arity to ignore the implicit object pointer.
        using type = typename function_call_operator_t::template arg_t<N+1>;
    };

    /// Helper for providing arg_t<N> vs. arg<N>::type.
    template <std::size_t N>
    using arg_t = typename arg<N>::type;

    /// For checking if the type of arg N is T.
    template <std::size_t N, typename T>
    static constexpr bool argtype_is_v = std::is_same_v<arg_t<N>, T>;

    /// For checking if the return type is T.
    template <typename T>
    static constexpr bool return_type_is_v = std::is_same_v<return_type_t, T>;
};

/**
 * Specializations for "Callables" to strip ref qualifiers.
 * @todo This could probably be done with std::decay<> in the specialization above.
 */
template <typename CallableType>
struct function_traits<CallableType&> : public function_traits<CallableType> {};

template <typename CallableType>
struct function_traits<CallableType&&> : public function_traits<CallableType> {};


/// @name Convenience templates, when you don't need all the function_traits<>.
/// @{

/**
 * Return type of a function with type F.
 */
template <typename F>
using function_return_type_t = typename function_traits<F>::return_type_t;

template <typename F, typename R>
static constexpr bool function_return_type_is_v = std::is_same_v<function_return_type_t<F>, R>;

//template<typename F, typename... Args>
//struct result_type_given_args
//{
//	using
//};


/// @} // Convenience templates.

#endif /* UTILS_CONCURRENCY_FUNCTION_TRAITS_HPP_ */
