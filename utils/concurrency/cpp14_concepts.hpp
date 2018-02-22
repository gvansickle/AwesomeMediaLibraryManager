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

#ifndef UTILS_CONCURRENCY_CPP14_CONCEPTS_HPP_
#define UTILS_CONCURRENCY_CPP14_CONCEPTS_HPP_

#include <type_traits>
#include <utility>

#if 1 /// @todo if these haven't been standardized/aren't supported.

/// void_t
template <class...>
using void_t = void;

template <bool B>
using bool_constant = std::integral_constant<bool, B>;

/**
 * Library Fundamentals TS v2 class used by detected_t to indicate detection failure.
 */
struct nonesuch final
{
  nonesuch () = delete;
  nonesuch (nonesuch const&) = delete;
  ~nonesuch () = delete;
  void operator = (nonesuch const&) = delete;
};

#if !defined(__cpp_lib_experimental_detect) || (__cpp_lib_experimental_detect < 201505)
// No support for detection idiom.  Per http://en.cppreference.com/w/cpp/experimental/lib_extensions_2
namespace detail
{
	template <class Default, class AlwaysVoid,
          template<class...> class Op, class... Args>
	struct detector
	{
	  using value_t = std::false_type;
	  using type = Default;
	};

	template <class Default, template<class...> class Op, class... Args>
	struct detector<Default, void_t<Op<Args...>>, Op, Args...> {
	  using value_t = std::true_type;
	  using type = Op<Args...>;
	};

} // namespace detail

/**
 * is_detected<Op, Args>
 * "Is it valid to instantiate Op with Args?"
 * I.e. is
 */
template <template<class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template <template<class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

template <class Default, template<class...> class Op, class... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

template< template<class...> class Op, class... Args >
constexpr bool is_detected_v = is_detected<Op, Args...>::value;

template< class Default, template<class...> class Op, class... Args >
using detected_or_t = typename detected_or<Default, Op, Args...>::type;

template <class Expected, template<class...> class Op, class... Args>
using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;

template <class Expected, template<class...> class Op, class... Args>
constexpr bool is_detected_exact_v = is_detected_exact<Expected, Op, Args...>::value;

template <class To, template<class...> class Op, class... Args>
using is_detected_convertible = std::is_convertible<detected_t<Op, Args...>, To>;

template <class To, template<class...> class Op, class... Args>
constexpr bool is_detected_convertible_v = is_detected_convertible<To, Op, Args...>::value;

#endif // No support for detection idiom.

template <class...> struct conjunction;
template <class...> struct disjunction;
template <class B> using negation = bool_constant<not B::value>;

template <class T, class... Ts>
struct conjunction<T, Ts...> :
  bool_constant<T::value and conjunction<Ts...>::value>
{ };
template <> struct conjunction<> : std::true_type { };

template <class T, class... Ts>
struct disjunction<T, Ts...> :
  bool_constant<T::value or disjunction<Ts...>::value>
{ };
template <> struct disjunction<> : std::false_type { };

/// Template variable wrappers.
/// @{

template <bool... Bs>
constexpr bool require = conjunction<bool_constant<Bs>...>::value;

template <bool... Bs>
constexpr bool either = disjunction<bool_constant<Bs>...>::value;

template <bool... Bs>
constexpr bool disallow = not require<Bs...>;

template <template <class...> class Op, class... Args>
constexpr bool exists = is_detected<Op, Args...>::value;

template <class To, template <class...> class Op, class... Args>
constexpr bool converts_to = is_detected_convertible<To, Op, Args...>::value;

template <class Exact, template <class...> class Op, class... Args>
constexpr bool identical_to = is_detected_exact<Exact, Op, Args...>::value;

/// @}


namespace concepts
{
	template <class T> constexpr bool Pointer = std::is_pointer<T>::value;
}


#endif /// @todo if these haven't been standardized.


#endif /* UTILS_CONCURRENCY_CPP14_CONCEPTS_HPP_ */
