/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef AWESOMEMEDIALIBRARYMANAGER_EXTASYNC_RUNINTHREAD_H
#define AWESOMEMEDIALIBRARYMANAGER_EXTASYNC_RUNINTHREAD_H

// Std C++
#include <type_traits>
#include <functional>

// Std C++ helpers.
#include <future/function_traits.hpp>
#include <future/Unit.hpp>

// Qt5
#include <QFutureWatcher>
#include <QThread>

// Ours.
//#include "../ExtAsync_traits.h"
#include "ExtFuture_make_xxx_future.h"
#include "ExtFutureImplHelpers.h"
#include "../ExtFuture.h"
#include <logic/PerfectDeleter.h>


namespace ExtAsync
{
namespace detail
{
	/**
	 * Common QThread-based async() executor.
	 *
	 * Runs @a callback(args...) in a new QThread.  If return type is not void, it will be reported to
	 * the returned future as a result.
	 *
	 * @param retfuture  The future which will be both copied to the first-level callback as the first parameter,
	 *                   and immediately returned to the caller.
	 * @param callback
	 * @param args
	 * @return
	 */
	template <class CallbackType, class... Args,
			  class CBReturnType = Unit::LiftT<std::invoke_result_t<CallbackType, Args...>>,
			  class R = Unit::LiftT<std::invoke_result_t<CallbackType, Args...>>,
			  class ExtFutureR = ExtFuture<R>,
			  REQUIRES(!is_ExtFuture_v<R>)
			  >
	ExtFutureR qthread_async(ExtFutureR retfuture, CallbackType&& callback, Args&&... args)
	{
//		static_assert(std::is_void_v<std::invoke_result_t<CallbackType, Args...>>
//				|| (std::is_same_v<R, CBReturnType> && !std::is_void_v<R>));

		Q_ASSERT(retfuture.isStarted());

		QThread* new_thread = QThread::create
						([=, fd_callback=FWD_DECAY_COPY(CallbackType, callback),
												  retfuture_cp=/*std::forward<ExtFutureR>*/(retfuture)
										  ]() mutable {
			Q_ASSERT(retfuture == retfuture_cp);
			Q_ASSERT(retfuture_cp.isStarted());
			// Catch any exceptions from the callback and propagate them to the returned future.
			try
			{
				static_assert(!is_ExtFuture_v<CBReturnType>, "Callback return value cannot be a future type.");
				CBReturnType retval;
				if constexpr(std::is_convertible_v<Unit, CBReturnType>)
				{
					// Callback returns void, caller has to arrange for retfuture results/finished reporting.
					std::invoke(std::move(fd_callback), args...);
				}
				else if constexpr(!is_ExtFuture_v<CBReturnType>)
				{
					// Callback returns a value.  Report it to the future.
					retval = std::invoke(std::move(fd_callback), args...);
					qDb() << "Reporting results:" << retval;
					retfuture_cp.reportResult(retval);
				}
				else
				{
					static_assert(dependent_false_v<CBReturnType>, "Callback return type not void or non-ExtFuture.");
				}
			}
			catch(...)
			{
				// Catch any exceptions and throw them to the returned future.
				std::exception_ptr eptr = std::current_exception();
				ManagedExtFutureWatcher_detail::propagate_eptr_to_future(eptr, retfuture_cp);
			}

			retfuture_cp.reportFinished();

#if 0
			catch(ExtAsyncCancelException& e)
			{
				/**
				 * Per std::experimental::shared_future::then() at @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
				 * "Any value returned from the continuation is stored as the result in the shared state of the returned future object.
				 *  Any exception propagated from the execution of the continuation is stored as the exceptional result in the shared
				 *  state of the returned future object."
				 */
				qDb() << "CAUGHT CANCEL EXCEPTION";
				retfuture_cp.reportException(e);
				Q_ASSERT(retfuture_cp.isCanceled());
			}
			catch(QException& e)
			{
				qDb() << "CAUGHT QEXCEPTION";
				retfuture_cp.reportException(e);
			}
			catch (...)
			{
				/// @todo Need to report the actual exception here.
				qWr() << "CAUGHT UNKNOWN EXCEPTION";
				retfuture_cp.reportException(QUnhandledException());
			}

			qDb() << "Leaving Thread:" << M_ID_VAL(retfuture_cp) << M_ID_VAL(retfuture);
			// Even in the case of exception, we need to reportFinished() or we just hang.
			/// @note Not clear what is happening here.  reportException() sets canceled, so why finished?
			/// @todo Not sure if this also applies if we're already finished or canceled.
			if(retfuture_cp.hasException())
			{
				qDb() << "Future has exception:" << retfuture_cp;
				Q_ASSERT(retfuture_cp.isCanceled());
			}
			// We always have to report finished, regardless of exception or cancel status.
			retfuture_cp.reportFinished();
			qDb() << "Reported finished:" << retfuture_cp;
			return;
#endif
		});

		// Make a self-delete connection for the QThread.
		// This is per-Qt5 docs, minus the lambda, which shouldn't make a difference.
		connect_or_die(new_thread, &QThread::finished, new_thread, [=](){
			qDb() << "DELETING QTHREAD:" << new_thread;
			new_thread->deleteLater();
		});

		/// @todo Set thread name before starting it.
//		new_thread->setObjectName();

		// Add to PerfectDeleter.
		PerfectDeleter::instance().addQThread(new_thread);

		// Start the thread.
		new_thread->start();

		return retfuture;
	}
} // namespace detail

	/**
	 * Run a callback in a QThread.
	 * The returned ExtFuture<R> will be reported as Finished when the callback returns, or as Exception on either cancel or error.
	 * Exceptions thrown by @a callback will be propagated to the returned ExtFuture<R>.
	 *
	 * @note On the correct usage of std::invoke_result_t<> in this situation:
	 * @link https://stackoverflow.com/a/47875452
	 * "There's no difference between std::invoke_result_t<F&&, Args&&...> and std::invoke_result_t<F, Args...>"
	 */
	template <class CallbackType, class... Args/*,
			  class R = Unit::LiftT<std::invoke_result_t<CallbackType, Args...>>,
			  REQUIRES(!is_ExtFuture_v<R>)*/
			  >
	auto qthread_async(CallbackType&& callback, Args&&... args) -> ExtFuture<Unit::LiftT<std::invoke_result_t<CallbackType, Args...>>>
	{
		using R = Unit::LiftT<std::invoke_result_t<CallbackType, Args...>>;
		static_assert(!is_ExtFuture_v<R>);

		ExtFuture<R> retfuture = make_started_only_future<R>();
		retfuture.setName("qthread_asyncRetFuture");

		return detail::qthread_async(retfuture, FWD_DECAY_COPY(CallbackType, callback), args...);
	}


	/**
	 * Run a Controllable and Reporting (CnR) callback in a new QThread.
	 * Callback is passed an ExtFuture<T> as the first parameter, which it should use for all control and reporting.
	 * @param callback   Callback function which will be run asynchronously in a new thread.  Signature:
	 *       @code
	 *       void callback(ExtFuture<T>, Args...)
	 *       @endcode
	 * @returns A copy of the same ExtFuture<T> passed to the callback.
	 */
	template<class CallbackType, class ExtFutureT = argtype_t<CallbackType, 0>, class... Args,
		REQUIRES(is_ExtFuture_v<ExtFutureT>
			 && !is_nested_ExtFuture_v<ExtFutureT>
			 && std::is_invocable_r_v<void, CallbackType, ExtFutureT, Args...>
			 && std::is_same_v<void, std::invoke_result_t<CallbackType, ExtFutureT, Args...>>)>
	ExtFutureT qthread_async_with_cnr_future(CallbackType&& callback, Args&& ... args)
	{
		ExtFutureT retfuture = make_started_only_future<typename ExtFutureT::value_type>();
		retfuture.setName("CNRRetfuture");

		return detail::qthread_async(retfuture, callback, retfuture, args...);
	};

//
//	template <class Fut, class Work>
//	auto then_in_qthread(Fut&& f, Work&& w) -> ExtFuture<decltype(w(f.get()))>
//	{
//		return ExtAsync::qthread_async([=]{ w(f.get());});
//	}

}; // END namespace ExtAsync.

#endif //AWESOMEMEDIALIBRARYMANAGER_EXTASYNC_RUNINTHREAD_H
