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
 * @file static_assert()s for various ExtAsync<> properties.
 */

#include "ExtAsync.h"

#include <type_traits>
#include <future/future_type_traits.hpp>

#include "ExtAsync_traits.h"
#include <utils/DebugHelpers.h>




/// Attic for experimental stuff that didn't pan out, but is too good to trash.
#if 0
////// START EXPERIMENTAL

/**
 * Returns a callable object which captures f.
 * Different from async_adapter() in that f is to be called with normal values,
 * not futures.
 */
template <typename F>
static auto asynchronize(F f)
{
	return [f](auto ... xs) {
		return [=] () {
			return std::async(std::launch::async, f, xs...);
		};
	};
}

/**
 * Returned object can be called with any number of future objects as parameters.
 * It then calls .get() on all futures, applies function f to them, and returns the result.
 */
template <typename F>
static auto fut_unwrap(F f)
{
	return [f](auto ... xs) {
		return f(xs.get()...);
	};
}

/**
 * Wraps a synchronous function, makes it wait for future arguments and returns a future result.
 */
template <typename F>
static auto async_adapter(F f)
{
	return [f](auto ... xs) {
		return [=] () {
			// What's going on here:
			// - Everything in parameter pack xs is assumed to be a callable object.  They will be called without args.
			// - fut_unwrap(f) transforms f into a function object which accepts an arbitrary number of args.
			// - When this async finally runs f, f calls .get() on all the xs()'s.
			return std::async(std::launch::async, fut_unwrap(f), xs()...);
		};
	};
}

////// END EXPERIMENTAL

template <class CallbackType>
ExtFuture<decltype(std::declval<CallbackType>()())>
spawn_async(CallbackType&& callback)
{
	// The promise we'll make and extract a future from.
	ExtFuture<decltype(std::declval<CallbackType>()())> promise;

	auto retfuture = promise.get_future();
	std::thread the_thread(
			[promise=std::move(promise), callback=std::decay_t<CallbackType>(callback)]()
			mutable {
				try
				{
					promise.set_value_at_thread_exit(callback());
				}
				catch(QException& e)
				{
					promise.set_exception_at_thread_exit(e);
				}
				catch(...)
				{
					promise.set_exception_at_thread_exit(std::current_exception());
				}
			});
	the_thread.detach();
	return retfuture;
};

#endif
