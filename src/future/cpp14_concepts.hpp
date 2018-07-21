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

// Std C++
#include <type_traits>
#include <utility> // For std::declval<>.

#include "future_type_traits.hpp"

#if 1 /// @todo if these haven't been standardized/aren't supported.

/// void_t
template <class...>
using void_t = void;
//template<typename... Ts> struct make_void { typedef void type;};
//template<typename... Ts> using void_t = typename make_void<Ts...>::type;

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
template <class B> using negation = bool_constant<!B::value>;

template <class T, class... Ts>
struct conjunction<T, Ts...> :
  bool_constant<T::value && conjunction<Ts...>::value>
{ };
template <> struct conjunction<> : std::true_type { };

template <class T, class... Ts>
struct disjunction<T, Ts...> :
  bool_constant<T::value || disjunction<Ts...>::value>
{ };
template <> struct disjunction<> : std::false_type { };

/// Template variable wrappers.
/// @{

template <bool... Bs>
constexpr bool require = conjunction<bool_constant<Bs>...>::value;

template <bool... Bs>
constexpr bool either = disjunction<bool_constant<Bs>...>::value;

template <bool... Bs>
constexpr bool disallow = !require<Bs...>;

template <template <class...> class Op, class... Args>
constexpr bool exists = is_detected<Op, Args...>::value;

template <class To, template <class...> class Op, class... Args>
constexpr bool converts_to = is_detected_convertible<To, Op, Args...>::value;

template <class Exact, template <class...> class Op, class... Args>
constexpr bool identical_to = is_detected_exact<Exact, Op, Args...>::value;

/// @}

/// @name Detectors.
/// @{



/// Member aliases.
namespace alias
{
	template <class T>
	using value_type = typename T::value_type;
	template <class T>
	using reference = typename T::reference;
	template <class T>
	using pointer = typename T::pointer;
} // namespace alias

/// @}

/// Generic "compiles" and "requires" traits.
/// From here: https://foonathan.net/blog/2016/09/09/cpp14-concepts.html
/// @{
template <typename T, template <typename> class Expression, typename AlwaysVoid = void_t<>>
struct compiles : std::false_type {};

/**
 * @tparam T  type to examine.
 * @tparam Expression  A template/template alias which if it can be compiled, results in returning true_type.
 */
template <typename T, template <typename> class Expression>
struct compiles<T, Expression, void_t<Expression<T>>> : std::true_type {};

/**
 * SFINAE-based "requires".
 * Return type is ResultType if all Concepts applied t CheckType are fulfilled, otherwise it's ill-formed and template
 * will drop out of the resolution set.
 * Example use:
 * @code
 *    template <typename T>
 *    auto function(T t) -> requires<int, T, Concept1, Concept2>;
 * @endcode
 */
template <typename ResultType, typename CheckType, template <typename> class ... Concepts>
using requires = std::enable_if_t<conjunction<Concepts<CheckType>...>::value, ResultType>;

/**
 *
 */
template <typename ResultType, typename CheckType, template <typename> class ... Concepts>
using fallback = std::enable_if_t<conjunction<negation<Concepts<CheckType>>...>::value, ResultType>;
/// @}

namespace concepts
{
	template <class T> constexpr bool Pointer = std::is_pointer<T>::value;

	/// Mishmash of ideas from all over.
	/// SO post: https://stackoverflow.com/a/26533335

	/// "models" trait declarations.
	/// Primary template, handles types which do not model a Concept, which we define as
	/// having a "requires" member.
	template <class Concept, class Enable=void_t<>>
	struct models : std::false_type {};

	/// Specialization which handles types which do have a ".requires()" member, and hence
	/// we say model a Concept.
	template <class Concept, class... Ts>
	struct models<Concept(Ts...), void_t<
									decltype(std::declval<Concept>().requires(std::declval<Ts>()...))
									>
			> : std::true_type {};

	/// Our low-rent C++2x "requires" "keyword".
	#define REQUIRES(...) typename std::enable_if_t<(__VA_ARGS__), int> = 0

	/// Helper function for reducing the need for dectype().
	template<class Concept, class... Ts>
	constexpr auto modeled(Ts&&...)
	{
	    return models<Concept(Ts...)>();
	}

	template <class T>
	constexpr bool Class = std::is_class_v<T>;

	template <class T>
	constexpr bool Function = std::is_function<T>::value;

	struct Callable
	{
		template<class F, class... Ts>
		auto requires(F&& f, Ts&&... xs) -> decltype(f(std::forward<Ts>(xs)...));
	};
}


#endif /// @todo if these haven't been standardized.


#endif /* UTILS_CONCURRENCY_CPP14_CONCEPTS_HPP_ */
