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
#include "ManagedExtFutureWatcher_impl.h"
#include "../ExtFuture.h"
#include <logic/PerfectDeleter.h>


namespace ExtAsync
{
namespace detail
{
	/**
	 * Wrapper helper object for async_qthread_with_event_loop_cnr().
	 */
	class WorkerQObject : public QObject
	{
	Q_OBJECT

	public:
		WorkerQObject() = default;
		~WorkerQObject() override = default;

	public:
		template <class CallbackType, class ExtFutureT, class... Args>
		void process(CallbackType&& callback, ExtFutureT future, Args&&... args)
		{
			std::invoke(std::move(callback), future, args...);
		};

	Q_SIGNALS:
		void resultReady();
		void finished();
		void error(QString err);

	private:
		// add your variables here
	};

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
												  down=DECAY_COPY(retfuture)
										  ]() mutable {
			Q_ASSERT(retfuture == down);
			Q_ASSERT(down.isStarted());

			// Catch any exceptions from the callback and propagate them to the returned future.
			try
			{
				static_assert(!is_ExtFuture_v<CBReturnType>, "Callback return value cannot be a future type.");
				CBReturnType retval; // = std_invoke_and_lift(std::move(fd_callback), args...);
				if constexpr(std::is_convertible_v<Unit, CBReturnType>)
				{
					// Callback returns void, caller has to arrange for retfuture results/finished reporting.
					std::invoke(std::move(fd_callback), args...);
				}
				else if constexpr(!is_ExtFuture_v<CBReturnType>)
				{
					// Callback returns a value.  Report it to the future.
					retval = std::invoke(std::move(fd_callback), args...);
//					qDb() << "Reporting results:" << retval;
					down.reportResult(retval);
				}
				else
				{
					static_assert(dependent_false_v<CBReturnType>, "Callback return type not void or non-ExtFuture.");
				}
			}
			catch(...)
			{
				// Catch any exceptions and throw them to the returned future.
				qDb() << "THROWING EXCEPTION TO RETURNED FUTURE:" << down;
				std::exception_ptr eptr = std::current_exception();
				ManagedExtFutureWatcher_detail::propagate_eptr_to_future(eptr, down);
				qDb() << "THROW COMPLETE:" << down;
			}

			// Always reportFinished(), to catch any callbacks which don't do it.
			down.reportFinished();
			qDb() << "qthread_async() finished, down:" << down;
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
	};

	/**
	 * Run the Controllable and Reporting (CnR) @a callback in a new QThread with its own event loop.
	 * Callback is passed an ExtFuture<T> as the first parameter, which it should use for all control and reporting.
	 * We're using the Qt "worker object" method here.
	 *
	 * @param callback   Callback function which will be run asynchronously in a new thread.  Signature:
	 *       @code
	 *       void callback(ExtFuture<T>, Args...)
	 *       @endcode
	 * @returns A copy of the same ExtFuture<T> passed to the callback.
	 *
	 */
	template <class CallbackType, class... Args,
			class ExtFutureT = argtype_t<CallbackType, 0>,
			REQUIRES(is_ExtFuture_v<ExtFutureT> && !is_nested_ExtFuture_v<ExtFutureT>)>
	ExtFutureT async_qthread_with_event_loop_cnr(CallbackType&& callback, Args&&... args)
	{
		/// All we do here is create a new QThread with an almost-dummy QObject in it,
		/// then call ExtAsync::detail::run_in_event_loop() on it.
		using ExtAsync::detail::WorkerQObject;

		ExtFutureT retfuture = make_started_only_future<typename ExtFutureT::inner_t>();

		QThread* thread = new QThread;
		WorkerQObject* worker = new WorkerQObject();
		// Move this worker QObject to the new thread.
		worker->moveToThread(thread);

		// Connect to the worker's error signal.
		connect_or_die(worker, &WorkerQObject::error, thread, [=](QString errorstr){
			qCr() << "WorkerQObject reported error:" << errorstr;
		});
		// Connection from thread start to actual WorkerQObject process() function start
		connect_or_die(thread, &QThread::started, worker, [=,
				callback_copy = FWD_DECAY_COPY(CallbackType, callback),
				retfuture_copy = retfuture,
				argtuple = std::make_tuple(worker, FWD_DECAY_COPY(CallbackType, callback),
				                           std::forward<ExtFutureT>(retfuture), std::forward<Args>(args)...)
		]() mutable {

			/// @todo Exceptions/cancellation.
			worker->process(callback_copy, retfuture_copy, args...);

			/// @note Unconditional finish here.
			retfuture_copy.reportFinished();
		});
		// When the worker QObject is finished, tell the thread to quit, and register the worker and QThread to be deleted.
		connect_or_die(worker, &WorkerQObject::finished, thread, &QThread::quit);
		connect_or_die(worker, &WorkerQObject::finished, worker, &QObject::deleteLater);
		// Connect the QThread's finished signal to the deleteLater() slot so that it gets scheduled for deletion.
		connect_or_die(thread, &QThread::finished, thread, &QThread::deleteLater);

		// Start the new thread.
		thread->start();

		return retfuture;
	};

} // namespace detail

	/**
	 * Run a callback in a new QThread.
	 * The returned ExtFuture<R> will be reported as Finished when the callback returns.
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

		return detail::qthread_async(retfuture, FWD_DECAY_COPY(CallbackType, callback), retfuture, args...);
	};

	/**
	 * Run the callback in a new QThread with its own event loop.
	 *
	 * @note Replacing ExtAsync::run_in_qthread_with_event_loop().
	 *
	 * @tparam CallbackType  Callback of type:
	 *     @code
	 *     void callback(ExtFutureT [, ...])
	 *     @endcode
	 * @param callback  The callback to run in the new thread.
	 */
//	template <class ExtFutureT = argtype_t<CallbackType, 0>,
//			class... Args,
//			REQUIRES(is_ExtFuture_v<ExtFutureT> && !is_nested_ExtFuture_v<ExtFutureT>)>
//	ExtFutureT async_qthread_with_event_loop(CallbackType&& callback, Args&&... args)
//	{

//	}
	template <class CallbackType, class... Args,
				class ExtFutureT = argtype_t<CallbackType, 0>,
				REQUIRES(is_ExtFuture_v<ExtFutureT> && !is_nested_ExtFuture_v<ExtFutureT>)>
		ExtFutureT async_qthread_with_event_loop_cnr(CallbackType&& callback, Args&&... args)
	{
		ExtFutureT retfuture = make_started_only_future<typename ExtFutureT::value_type>();
		retfuture.setName("EvLoopCNRRetfuture");

		return detail::async_qthread_with_event_loop_cnr(std::move(callback), retfuture, args...);

	}

}; // END namespace ExtAsync.

#endif //AWESOMEMEDIALIBRARYMANAGER_EXTASYNC_RUNINTHREAD_H
