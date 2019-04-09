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
#include "../ExtFuture.h"

template <typename T>
ExtFuture<T> make_started_only_future();

namespace ExtAsync
{
	/**
	 * Run a callback in a QThread.
	 * @note On the correct usage of std::invoke_result_t<> in this situation:
	 * @link https://stackoverflow.com/a/47875452
	 * "There's no difference between std::invoke_result_t<F&&, Args&&...> and std::invoke_result_t<F, Args...>"
	 */
	template <class CallbackType, /*class ExtFutureR,*/ class... Args,
			  class R = Unit::LiftT<std::invoke_result_t<CallbackType, Args...>>,
			  class ExtFutureR = std::conditional_t<is_ExtFuture_v<R>, R, ExtFuture<R>>
			  >
	static ExtFutureR qthread_async(CallbackType&& callback, Args&&... args)
	{
//		static_assert(std::is_invocable_r_v<void, CallbackType, Args...>);

		ExtFutureR retfuture = make_started_only_future<typename ExtFutureR::value_type>();
		qDb() << "ENTER" << __func__ << ", retfuture:" << retfuture;
		auto new_thread = QThread::create([=, callback=DECAY_COPY(callback),
												  retfuture_cp=/*std::forward<ExtFutureR>*/(retfuture)
										  ]() mutable {
				qDb() << "ENTER IN1, retfuture_cp:" << retfuture_cp;
				Q_ASSERT(retfuture_cp == retfuture);
				retfuture_cp.reportStarted();
				if constexpr(std::is_void_v<Unit::DropT<typename ExtFutureR::value_type>>)
				{
					std::invoke(std::move(callback), args...);
					retfuture_cp.reportFinished();
				}
				else
				{
					auto retval = std::invoke(std::move(callback), args...);
					static_assert(!is_ExtFuture_v<decltype(retval)>, "Callback return value cannot be a future type.");
					retfuture_cp.reportFinished(&retval);
				}
				qDb() << "EXIT IN1";
			;});

		connect_or_die(new_thread, &QThread::finished, new_thread, [=](){
			qDb() << "DELETING QTHREAD:" << new_thread;
			new_thread->deleteLater();
		});

		// Start the thread.
		new_thread->start();

		qDb() << "EXIT" << __func__ << ", retfuture:" << retfuture;
		return retfuture;
	}


	/**
	 * Run a callback in a QThread.  Callback is passed an ExtFuture<T>.
	 */
#if 0
	template<class CallbackType,
		class ExtFutureT = argtype_t<CallbackType, 0>,
		class... Args,
		class U = Unit::LiftT<std::invoke_result_t<CallbackType, ExtFutureT, Args...>>, // callback return type.
		REQUIRES(is_ExtFuture_v<ExtFutureT> && !is_nested_ExtFuture_v<ExtFutureT>)>
	ExtFuture<U> run_in_qthread(CallbackType&& callback, Args&& ... args)
	{
	//		class U = Unit::Lift<std::invoke_result_t<CallbackType, ExtFutureT, Args...>>;
		ExtFuture<U> retfuture = make_started_only_future<U>();

		auto new_thread = QThread::create(callback, retfuture, args...);

	//		connect_or_die(new_thread, &QThread::finished, new_thread, &QObject::deleteLater);

		new_thread->start();

		qDb() << __func__ << "RETURNING";

		return retfuture;
	};
#else
	template<class CallbackType, class ExtFutureT = argtype_t<CallbackType, 0>, class... Args,
		REQUIRES(is_ExtFuture_v<ExtFutureT> && !is_nested_ExtFuture_v<ExtFutureT>)>
	static ExtFutureT run_in_qthread(CallbackType&& callback, Args&&... args)
	{
		ExtFutureT retfuture = make_started_only_future<typename ExtFutureT::value_type>();

		qDb() << "ENTER" << __func__ << ", retfuture:" << retfuture;

		// Ignoring the returned ExtFuture<>.
		/// @todo Can we really do this?
		/*auto inner_retfuture =*/ qthread_async(callback, retfuture, args...);

		qDb() << "EXIT" << __func__ << ", retfuture:" << retfuture;// << M_ID_VAL(inner_retfuture);

		return retfuture;
	};
#endif

	/**
	 * Attach a Sutteresque .then()-like continuation to a run_in_qthread().
	 */
	template <class InFutureT, class CallbackType, class OutFutureU>
	static OutFutureU then_in_main_thread(InFutureT in_future, CallbackType&& then_callback)
	{
		using U = typename OutFutureU::value_type;
		OutFutureU retfuture = make_started_only_future<U>();

		QFutureWatcher<U>* watcher = new QFutureWatcher<U>(qobject_cast<QObject>(qApp));

		connect_or_die(watcher, &QFutureWatcher<U>::finished, watcher, &QFutureWatcher<U>::deleteLater);

		return retfuture;
	};
}; // END namespace ExtAsync.

#endif //AWESOMEMEDIALIBRARYMANAGER_EXTASYNC_RUNINTHREAD_H
