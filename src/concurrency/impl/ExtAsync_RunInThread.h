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

// Std C++ helpers.
#include <future/function_traits.hpp>

// Qt5
#include <QThread>

// Ours.
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
			REQUIRES(is_ExtFuture_v<ExtFutureT> && !is_nested_ExtFuture_v<ExtFutureT>)>
	static ExtFutureT run_in_qthread(CallbackType&& callback, Args&& ... args)
	{
		using T = typename ExtFutureT::value_type;
		ExtFutureT retfuture = make_started_only_future<T>();

		auto new_thread = QThread::create(callback, retfuture, args...);

		connect_or_die(new_thread, &QThread::finished, new_thread, &QObject::deleteLater);

		new_thread->start();

		qDb() << __func__ << "RETURNING";

		return retfuture;
	}

} // END namespace ExtAsync.

#endif //AWESOMEMEDIALIBRARYMANAGER_EXTASYNC_RUNINTHREAD_H
