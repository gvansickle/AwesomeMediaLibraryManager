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

#ifndef UTILS_CONCURRENCY_FUTURE_TYPE_TRAITS_HPP_
#define UTILS_CONCURRENCY_FUTURE_TYPE_TRAITS_HPP_

#include <config.h>

// Std C++

#include <type_traits>
//#if __has_include(<experimental/type_traits>)
//#include <experimental/type_traits>
//using namespace ns_detection = namespace std::experimental::fundamentals_v2;
//#else
//using ns_detection = namespace std;
//#endif
#include <tuple>
#include <functional> // For std::invoke<>().

/// Backfill for C++17 std::void_t.
#if !defined(__cpp_lib_void_t) || (__cpp_lib_void_t < 201411)
namespace std
{
    /// void_t
    template <class...>
    using void_t = void;
    //template<typename... Ts> struct make_void { typedef void type;};
    //template<typename... Ts> using void_t = typename make_void<Ts...>::type;
}
#endif // std::void_t

/// Backfill for C++17 type trait variable templates (std::is_same_v, etc.)
#if !defined(__cpp_lib_type_trait_variable_templates) || (__cpp_lib_type_trait_variable_templates < 201510)
namespace std // Yeah, this is bad.
{
	/// C++17 is_same_v
	template <typename T, class U>
	constexpr bool is_same_v = std::is_same<T, U>::value;

	/// C++17 is_base_of_v
	template <class Base, class Derived>
	constexpr bool is_base_of_v = std::is_base_of<Base, Derived>::value;

	/// C++17 is_member_function_pointer_v
	template< class T >
	constexpr bool is_member_function_pointer_v = std::is_member_function_pointer<T>::value;

	/// C++17
	template< class T >
	constexpr bool is_class_v = is_class<T>::value;

	/// Not sure if this is in a proposed std or not.
	/// What is this good for?  Eliminating some ambiguities in template type deduction.
	/// The issue is that if you had something like this:
	///    template<class T> T somefunc(const T& a, T b, T* c);
	/// Every "T" would be used in the type-deduction process.  Which could be a problem is the T's were actually
	/// different types.
	/// identity<> uses T as a nested-name-specifier, which creates a non-deduced context.  Therefore, any T's referred
	/// to in there do not participate in type deduction, but simply take on the type deduced by the non-identitied T's.
	template <class T>
	struct identity
	{
		using type = T;
	};
	template <class T>
	using identity_t = typename identity<T>::type;

	/// Static tests.
	static_assert(std::is_same_v<identity_t<int>, int>,"");
	static_assert(std::is_same_v<identity_t<int* const&>, int* const&>,"");

	/// C++17
	template< class T >
	constexpr bool is_object_v = is_object<T>::value;

	/// C++17
	template <class T>
	constexpr bool tuple_size_v = tuple_size<T>::value;
} // END std
#endif // __cpp_lib_type_trait_variable_templates < 201510



/// Backfill for C++17 detection idiom (std::is_detected, etc.)
/// These would be defined in <experimental/type_traits>.
#if !defined(__cpp_lib_experimental_detect) || (__cpp_lib_experimental_detect < 201505)
namespace std // I know, I know.
{
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
		struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
		  using value_t = std::true_type;
		  using type = Op<Args...>;
		};

	} // namespace detail

	/**
	 * Library Fundamentals TS v2 class used by detected_t to indicate detection failure.
	 * @link https://en.cppreference.com/w/cpp/experimental/nonesuch
	 */
	struct nonesuch final
	{
	  nonesuch () = delete;
	  ~nonesuch () = delete;
	  nonesuch (nonesuch const&) = delete;
	  void operator = (nonesuch const&) = delete;
	};

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
} // namespace std
#endif // __cpp_lib_experimental_detect No support for detection idiom.

#if !defined(__cpp_lib_bool_constant) || (__cpp_lib_bool_constant < 201505)
namespace std
{
    template <bool B>
    using bool_constant = std::integral_constant<bool, B>;
} // END std
#endif // __cpp_lib_bool_constant

/// Backfill for C++17 Logical Operator Type Traits
#if !defined(__cpp_lib_logical_traits) || (__cpp_lib_logical_traits < 201510)
namespace std
{
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
} // END std
#endif // __cpp_lib_logical_traits


/// Template variable wrappers.
/// @{

template <bool... Bs>
constexpr bool require = std::conjunction<std::bool_constant<Bs>...>::value;

template <bool... Bs>
constexpr bool either = std::disjunction<std::bool_constant<Bs>...>::value;

template <bool... Bs>
constexpr bool disallow = !require<Bs...>;

template <template <class...> class Op, class... Args>
constexpr bool exists = std::is_detected<Op, Args...>::value;

template <class To, template <class...> class Op, class... Args>
constexpr bool converts_to = std::is_detected_convertible<To, Op, Args...>::value;

template <class Exact, template <class...> class Op, class... Args>
constexpr bool identical_to = std::is_detected_exact<Exact, Op, Args...>::value;

/// @}


/**
 * Static assert helpers which try to print the types involved when the assert fails.
 * @rant Seriously, it's the 21st century and there's no decent way to simply dump this basic type information at compile time?
 */
#if 0
template <bool V, class A>
struct AssertionChecker
{
	static constexpr bool value = V;
	using AssertionValue = value;
};

template <typename Assertion>
struct AssertValue : AssertionChecker<Assertion::value, Assertion>
{
    static_assert(AssertionValue, "Assertion failed <see below for more information>");
    static bool const value = Assertion::value;
};

static_assert(
		AssertValue<
			std::is_same_v<int, int>
		>, "Test");
#endif

template <typename T>
struct deduced_type;
template<typename T>
void show_deduced_type(T&& )
{

    deduced_type<T>::show;
}

/**
 * The mysterious DECAY_COPY template referred to in e.g. std::experimental::shared_future::then() descriptions.
 * @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
 * @link https://www.boost.org/doc/libs/1_68_0/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.decay_copy
 */
template <class T>
std::decay_t<T> DECAY_COPY(T&& v)
{
    return std::forward<T>(v);
}


/**
 * The less mysterious INVOKE, which is actually std::invoke in C++17.
 */
#define INVOKE std::invoke

/**
 * Template for creating catch-all constexpr-if final else's.  This template lets you create a type-dependent
 * expression in a static_assert(), which would otherwise not work as expected:
 * if constexpr(whatever)
 * else if()
 * else
 * {
 *      static_assert(dependent_false<ContextType>::value, "No matching overload");
 * }
 * @tparam T
 */
template<class T>
struct dependent_false : std::false_type {};

#endif /* UTILS_CONCURRENCY_FUTURE_TYPE_TRAITS_HPP_ */
