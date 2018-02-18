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

#if 1 /// @todo if these haven't been standardized.

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

template <template<class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template <template<class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

template <class Default, template<class...> class Op, class... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

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
template <bool... Bs>
constexpr bool require = conjunction<bool_constant<Bs>...>::value;

template <bool... Bs>
constexpr bool either = disjunction<bool_constant<Bs>...>::value;

template <template <class...> class Op, class... Args>
constexpr bool exists = is_detected<Op, Args...>::value;

#endif /// @todo if these haven't been standardized.


#endif /* UTILS_CONCURRENCY_CPP14_CONCEPTS_HPP_ */
