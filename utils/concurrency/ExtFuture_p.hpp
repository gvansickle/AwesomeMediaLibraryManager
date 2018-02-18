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

#ifndef UTILS_CONCURRENCY_EXTFUTURE_P_HPP_
#define UTILS_CONCURRENCY_EXTFUTURE_P_HPP_


/**
 * For unwrapping an ExtFuture<ExtFuture<T>> to a ExtFuture<T>.
 * Implementation based on Facebook's "folly" library's Future (Apache 2.0)
 */
template <typename T>
template <typename F>
std::enable_if_t<isExtFuture<F>::value, ExtFuture<typename isExtFuture<T>::inner_t>>
ExtFuture<T>::unwrap()
{
	return then([](ExtFuture<typename isExtFuture<T>::inner_t> internal_extfuture) {
		return internal_extfuture;
		;});
}

#if 0
/**
 * This is the function which actually is called by QtConcurrent::run() for the continuation.
 */
template<class T>
static QString ThenHelper(ExtFuture<T>* predecessor_future)
{
	qDb() << "THEN CALLED, WAITING";
	predecessor_future->wait();
	qDb() << "THEN CALLED, WAIT OVER, CALLING CALLBACK";
	Q_CHECK_PTR(predecessor_future);
	Q_CHECK_PTR(predecessor_future->m_continuation_function);
	(*(predecessor_future->m_continuation_function))();
	return QString("THEN DONE");
}
#endif
#endif /* UTILS_CONCURRENCY_EXTFUTURE_P_HPP_ */
