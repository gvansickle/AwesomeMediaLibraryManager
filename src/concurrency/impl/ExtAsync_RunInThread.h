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

// Std C++ helpers.
#include <future/function_traits.hpp>
#include <future/Unit.hpp>

// Qt5
#include <QFutureWatcher>
#include <QThread>

// Ours.
#include "../ExtAsync_traits.h"
#include "ExtFutureImplHelpers.h"
#include "../ExtFuture.h"

template <typename T>
ExtFuture<T> make_started_only_future();

#define EXTASYNC_USE_QTHREAD 1

namespace ExtAsync
{
	/**
	 * Run a callback in a QThread.
	 * The returned ExtFuture<> will be reported as Finished when the callback returns, or as Exception on either cancel or error.
	 *
	 * @note On the correct usage of std::invoke_result_t<> in this situation:
	 * @link https://stackoverflow.com/a/47875452
	 * "There's no difference between std::invoke_result_t<F&&, Args&&...> and std::invoke_result_t<F, Args...>"
	 */
	template <class CallbackType, class... Args,
			  class R = Unit::LiftT<std::invoke_result_t<CallbackType, Args...>>,
			  class ExtFutureR = ExtFuture<R>,
			  REQUIRES(!is_ExtFuture_v<R>)
			  >
	ExtFutureR qthread_async(CallbackType&& callback, Args&&... args)
	{
		ExtFutureR retfuture = make_started_only_future<R>();//typename ExtFutureR::value_type>();

		qDb() << "ENTER" << __func__ << ", retfuture:" << retfuture;

#if EXTASYNC_USE_QTHREAD == 1
		QThread* new_thread = QThread::create
#else
		QtConcurrent::run
#endif
						([=, fd_callback=DECAY_COPY(std::forward<CallbackType>(callback)),
												  retfuture_cp=/*std::forward<ExtFutureR>*/(retfuture)
										  ]() mutable {

			Q_ASSERT(retfuture == retfuture_cp);
#if 0
			exception_propagation_helper(retfuture_cp,
					[=](ExtFutureR future_ref, Args... args) mutable {
						qDb() << "ENTER IN1, retfuture_cp:" << retfuture_cp;
						Q_ASSERT(retfuture_cp == retfuture);
						retfuture_cp.reportStarted();
						if constexpr(std::is_void_v<Unit::DropT<typename ExtFutureR::value_type>>)
						{
							std::invoke(std::move(fd_callback), args...);
							retfuture_cp.reportFinished();
						}
						else
						{
							auto retval = std::invoke(std::move(fd_callback), args...);
							static_assert(!is_ExtFuture_v<decltype(retval)>, "Callback return value cannot be a future type.");
							retfuture_cp.reportFinished(&retval);
						}
						qDb() << "EXIT IN1";
					},
//					std::move(fd_callback),
					args...);
#else
				try
				{
//					qDb() << "ENTER IN1, retfuture_cp:" << retfuture_cp;
					Q_ASSERT(retfuture_cp == retfuture);
//					retfuture_cp.reportStarted();
					if constexpr(std::is_void_v<Unit::DropT<typename ExtFutureR::value_type>>)
					{
						std::invoke(std::move(fd_callback), args...);
						retfuture_cp.reportFinished();
					}
					else
					{
						auto retval = std::invoke(std::move(fd_callback), args...);
						static_assert(!is_ExtFuture_v<decltype(retval)>, "Callback return value cannot be a future type.");
						retfuture_cp.reportFinished(&retval);
					}
//					qDb() << "EXIT IN1";
				}
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
					// I think we don't want to rethrow with a throw() here.  This will throw to the wrong future.
				}
				catch(QException& e)
				{
				qDb() << "CAUGHT QEXCEPTION";
					retfuture_cp.reportException(e);
				}
				catch (...)
				{
				qDb() << "CAUGHT UNKNOWN EXCEPTION";
					retfuture_cp.reportException(QUnhandledException());
				}
#endif
					qDb() << "Leaving Thread," << M_ID_VAL(retfuture_cp) << M_ID_VAL(retfuture);
					retfuture_cp.reportFinished();
					return;
			});
#if EXTASYNC_USE_QTHREAD == 1
		// Make a self-delete connection for the QThread.
		// This is per-Qt5 docs, minus the lambda, which shouldn't make a difference.
		connect_or_die(new_thread, &QThread::finished, new_thread, [=](){
			qDb() << "DELETING QTHREAD:" << new_thread;
			new_thread->deleteLater();
		});

		/// @todo Set thread name before starting it.
//		new_thread->setObjectName();

		// Start the thread.
		new_thread->start();
#endif
		qDb() << "EXIT" << __func__ << ", retfuture:" << retfuture;
		return retfuture;
	}


	/**
	 * Run a Controllable and Reporting (CnR) callback in a new QThread.
	 * Callback is passed an ExtFuture<T>, which it should use for all control and reporting.
	 * @returns A copy of the ExtFuture<T> passed to the callback.
	 */
	template<class CallbackType, class ExtFutureT = argtype_t<CallbackType, 0>, class... Args,
		REQUIRES(is_ExtFuture_v<ExtFutureT>
			 && !is_nested_ExtFuture_v<ExtFutureT>
			 && std::is_invocable_r_v<void, CallbackType, ExtFutureT, Args...>)>
	ExtFutureT qthread_async_with_cnr_future(CallbackType&& callback, Args&& ... args)
	{
		ExtFutureT retfuture = make_started_only_future<typename ExtFutureT::value_type>();

		qDb() << "ENTER" << __func__ << ", retfuture:" << retfuture;

		// Ignoring the returned ExtFuture<>.
		/// @note We couldn't do this with C++11-17 std::async()-returned futures, because they will block on delete:
		/// @link https://en.cppreference.com/w/cpp/thread/async
		/// "If the std::future obtained from std::async is not moved from or bound to a reference, the destructor of
		/// the std::future will block at the end of the full expression until the asynchronous operation completes"
		/*auto inner_retfuture =*/ qthread_async(callback, retfuture, args...);

		qDb() << "EXIT" << __func__ << ", retfuture:" << retfuture;

		return retfuture;
	};


//	/**
//	 * Attach a Sutteresque .then()-like continuation to a run_in_qthread().
//	 */
//	template <class InFutureT, class CallbackType, class OutFutureU>
//	static OutFutureU then_in_main_thread(InFutureT in_future, CallbackType&& then_callback)
//	{
//		using U = typename OutFutureU::value_type;
//		OutFutureU retfuture = make_started_only_future<U>();
//
//		QFutureWatcher<U>* watcher = new QFutureWatcher<U>(qobject_cast<QObject>(qApp));
//
//		connect_or_die(watcher, &QFutureWatcher<U>::finished, watcher, &QFutureWatcher<U>::deleteLater);
//
//		return retfuture;
//	};
//
//	template <class Fut, class Work>
//	auto then_in_qthread(Fut&& f, Work&& w) -> ExtFuture<decltype(w(f.get()))>
//	{
//		return ExtAsync::qthread_async([=]{ w(f.get());});
//	}

}; // END namespace ExtAsync.

#endif //AWESOMEMEDIALIBRARYMANAGER_EXTASYNC_RUNINTHREAD_H
