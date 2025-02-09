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

// Std C++
#include <type_traits>

// Not-so-Std C++
#include <future/future_type_traits.hpp>

// Ours
#include "ExtFuture_traits.h"
#include <utils/DebugHelpers.h>


// Static
static QThread* CreateNewQThread(const char *name = "unknown")
{
	static QThread* backprop_thread = []
	{
		QThread* new_thread = new QThread;
		new_thread->setObjectName("ExtFutureBackpropThread");
		// PerfectDeleter::instance().addQThread(new_thread);
#if 0
		, [](QThread* the_qthread){
			// Call exit(0) on the QThread.  We use Qt's invokeMethod() here.
			ExtAsync::detail::run_in_event_loop(the_qthread, [the_qthread](){
				qDb() << "Calling quit()+wait() on managed FutureWatcher QThread, FWParent has num children:" << ms_the_managed_fw_parent->children().size();
				the_qthread->quit();
				the_qthread->wait();
				qDb() << "Finished quit()+wait() on managed FutureWatcher QThread";
			});
		});
#endif
		// No parent, this will be eventually deleted by the signal below.
		// FutureWatcherParent* the_managed_fw_parent = get_future_watcher_parent();
		// Q_ASSERT(the_managed_fw_parent != nullptr);
		// Create and push the future watcher parent object into the new thread.
		// the_managed_fw_parent->moveToThread(new_thread);
		// Connect QThread::finished to QObject::deleteLater.
		// connect_or_die(new_thread, &QThread::finished, the_managed_fw_parent, &QObject::deleteLater);

		// Start the thread.
		new_thread->start();
		return new_thread;
	}();

	return backprop_thread;
}

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
