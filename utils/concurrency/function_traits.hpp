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

#include <type_traits>
#include <tuple>


#if 1 /// @todo Conditionalize on C++17 or the appropriate __has_whatever.
namespace std
{
	template< class T, class U >
	constexpr bool is_same_v = std::is_same<T, U>::value;
};
#endif

/**
 * The primary template.
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
        using type = typename std::tuple_element<N,std::tuple<Args...>>::type;
    };

    /// Helper for providing arg_t<N> vs. arg<N>::type.
    template <std::size_t N>
    using arg_t = typename arg<N>::type;

    /// For checking if the type of arg N is T.
    template <std::size_t N, typename T>
    static constexpr bool argtype_is_v = std::is_same_v<typename arg<N>::type, T>;

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

#endif /* UTILS_CONCURRENCY_FUNCTION_TRAITS_HPP_ */
