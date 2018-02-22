/*
 * future_type_traits.hpp
 *
 *  Created on: Feb 17, 2018
 *      Author: gary
 */

#ifndef UTILS_CONCURRENCY_FUTURE_TYPE_TRAITS_HPP_
#define UTILS_CONCURRENCY_FUTURE_TYPE_TRAITS_HPP_


#if 1 /// @todo Conditionalize on C++17 or the appropriate __has_whatever.


namespace std
{
	/// C++17 is_same_v
	template <typename T, class U>
	constexpr bool is_same_v = std::is_same<T, U>::value;

	/// C++17 void_t
	template <typename... >
	using void_t = void;
};



#endif


#endif /* UTILS_CONCURRENCY_FUTURE_TYPE_TRAITS_HPP_ */
