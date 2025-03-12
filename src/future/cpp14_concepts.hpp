/*
 * Copyright 2018, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

// Future Std C++
#include "future_type_traits.hpp" // For is_detected<> etc.

/// Our low-rent C++2x "requires" "keyword".
#define REQUIRES(...) typename std::enable_if_t<(__VA_ARGS__), int> = 0

#if 0 /// @todo if these haven't been standardized/aren't supported.

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
template <typename T, template <typename> class Expression, typename AlwaysVoid = std::void_t<>>
struct compiles : std::false_type {};

/**
 * @tparam T  type to examine.
 * @tparam Expression  A template/template alias which if it can be compiled, results in returning true_type.
 */
template <typename T, template <typename> class Expression>
struct compiles<T, Expression, std::void_t<Expression<T>>> : std::true_type {};

#if __cpp_lib_concepts < 202207L
/**
 * SFINAE-based "requires".
 * Return type is ResultType if all Concepts applied to CheckType are fulfilled, otherwise it's ill-formed and template
 * will drop out of the resolution set.
 * Example use:
 * @code
 *    template <typename T>
 *    auto function(T t) -> requires<int, T, Concept1, Concept2>;
 * @endcode
 */
template <typename ResultType, typename CheckType, template <typename> class ... Concepts>
using requires = std::enable_if_t<std::conjunction<Concepts<CheckType>...>::value, ResultType>;
#endif

/**
 * fallback is valid only if all conditions are false.  You want this as the compliment to
 * a number of requires<>, e.g.:
 *
 */
template <typename ResultType, typename CheckType, template <typename> class ... Concepts>
using fallback = std::enable_if_t<std::conjunction<std::negation<Concepts<CheckType>>...>::value, ResultType>;
/// @}

namespace concepts
{

	/**
	 * An always-false template for use in static_assert()'s, to make them SFINAE-friendly.
	 * E.g. in a fallback template:
	 * template<class T>
	 * auto func(T whatever) -> fallback<void, T, ConceptA>
	 * {
	 * 		static_assert(always_false<T>::error, "Type T doesn't model ConceptA");
	 * }
	 *
	 */
	template <class T>
	struct always_false
	{
		static constexpr bool error = false;
	};

	template <class T> constexpr bool Pointer = std::is_pointer<T>::value;

	/// Mishmash of ideas from all over.
	/// SO post: https://stackoverflow.com/a/26533335

#if 0///
	/// "models" trait declarations.
	/// Primary template, handles types which do not model a Concept, which we define as
	/// having a "requires" member.
	template <class Concept, class Enable=std::void_t<>>
	struct models : std::false_type {};

	/// Specialization which handles types which do have a ".requires()" member, and hence
	/// we say model a Concept.
	template <class Concept, class... Ts>
	struct models<Concept(Ts...), std::void_t<
									decltype(std::declval<Concept>().requires(std::declval<Ts>()...))
									>
			> : std::true_type {};
#endif///


#if 0///
	/// Helper function for reducing the need for dectype().
	template<class Concept, class... Ts>
	constexpr auto modeled(Ts&&...)
	{
	    return models<Concept(Ts...)>();
	}
#endif///

	template <class T>
	constexpr bool Class = std::is_class_v<T>;

	template <class T>
	constexpr bool Function = std::is_function<T>::value;

#if 0///
	struct Callable
	{
		template<class F, class... Ts>
		auto requires(F&& f, Ts&&... xs) -> decltype(f(std::forward<Ts>(xs)...));
	};
#endif///

}

#endif /// @todo if these haven't been standardized.



#endif /* UTILS_CONCURRENCY_CPP14_CONCEPTS_HPP_ */
