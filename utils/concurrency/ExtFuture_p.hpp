/*
 * ExtFuture_p.hpp
 *
 *  Created on: Feb 17, 2018
 *      Author: gary
 */

#ifndef UTILS_CONCURRENCY_EXTFUTURE_P_HPP_
#define UTILS_CONCURRENCY_EXTFUTURE_P_HPP_


/**
 * For unwrapping an ExtFuture<ExtFuture<T>> to a ExtFuture<T>.
 */
template <typename T>
template <typename F>
std::enable_if_t<isExtFuture<F>::value, ExtFuture<typename isExtFuture<T>::inner>>
ExtFuture<T>::unwrap()
{
	return then([](ExtFuture<typename isExtFuture<T>::inner> internal_extfuture) {
		return internal_extfuture;
		;});
}


#endif /* UTILS_CONCURRENCY_EXTFUTURE_P_HPP_ */
