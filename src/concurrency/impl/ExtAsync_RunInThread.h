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

// Qt5
#include <QFutureWatcher>
#include <QThread>

// Ours.
//#include <AMLMApp.h>
#include "../ExtAsync_traits.h"
#include "../ExtFuture.h"

namespace ExtAsync
{

	/**
	 * Run a callback in a QThread.
	 */
	template<class CallbackType,
			class ExtFutureT = argtype_t<CallbackType, 0>,
			class... Args,
			class U = std::invoke_result_t<CallbackType, ExtFutureT, Args...>, // callback return type.
			REQUIRES(is_ExtFuture_v<ExtFutureT> && !is_nested_ExtFuture_v<ExtFutureT>)>
	static ExtFuture<U> run_in_qthread(CallbackType&& callback, Args&& ... args)
	{
		using T = typename ExtFutureT::value_type;
		ExtFuture<U> retfuture = make_started_only_future<U>();

		auto new_thread = QThread::create(callback, retfuture, args...);

//		connect_or_die(new_thread, &QThread::finished, new_thread, &QObject::deleteLater);

		new_thread->start();

		qDb() << __func__ << "RETURNING";

		return retfuture;
	};

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

} // END namespace ExtAsync.

#endif //AWESOMEMEDIALIBRARYMANAGER_EXTASYNC_RUNINTHREAD_H
