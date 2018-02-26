/*
 * future_type_traits.hpp
 *
 *  Created on: Feb 17, 2018
 *      Author: gary
 */

#ifndef UTILS_CONCURRENCY_FUTURE_TYPE_TRAITS_HPP_
#define UTILS_CONCURRENCY_FUTURE_TYPE_TRAITS_HPP_

#include <type_traits>

#if 1 /// @todo Conditionalize on C++17 or the appropriate __has_whatever.


namespace std // Yeah, this is bad.
{
	/// C++17 is_same_v
	template <typename T, class U>
	constexpr bool is_same_v = std::is_same<T, U>::value;

	/// C++17 void_t
	template <typename... >
	using void_t = void;

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
};



#endif


#endif /* UTILS_CONCURRENCY_FUTURE_TYPE_TRAITS_HPP_ */
