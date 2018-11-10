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
 * @file ExtFuture.cpp
 *
 * Notes:
 *
 * - QFuture<> lost a lot of special member functions on this commit:
 * @link https://git.qt.io/consulting-usa/qtbase-xcb-rendering/commit/9c016cefe9a81de3b01019cb1eb1363b05e3b448
 * That's why there's no real copy constructor etc. defined - it now relies on the compiler-generated
 * (but not = default, those fail) ones.
 *
 * - The QMutex
 * There's a QMutex which lives in the QFutureInterfaceBasePrivate d instance of the QFutureInterfaceBase.
 * It's private, but a pointer to it is avalable via "QMutex *QFutureInterfaceBase::mutex() const".
 * Most of the public QFuture{Interface} interfaces lock this mutex, with the notable exception of the isFinished()/isCanceled()/etc.
 * state query functions, which simply query the bits in an atomic variable.
 *
 * - Regarding the initial state
 * From a comment in QtCreator's runextensions.h::AsyncJob constructor:
 * "we need to report it as started even though it isn't yet, because someone might
 * call waitForFinished on the future, which does _not_ block if the future is not started"
 * QFuture<T>() defaults to Started | Canceled | Finished.  Not sure we want that, or why that is.

 * That code also does this:
 *
 * 		m_future_interface.setRunnable(this);
 *
 * Not sure if we need to do that here or not, we don't have a QRunnable to give it.
 *
 * This is the code we're fighting:
 *
 * @code
 * void QFutureInterfaceBase::waitForFinished()
	{
		QMutexLocker lock(&d->m_mutex);
		/// GRVS: == NotInTheRunningState, i.e. we could never have started running, or be finished running.
		const bool alreadyFinished = !isRunning();
		lock.unlock();

		if (!alreadyFinished)
		{
			/// GRVS: Not finished, so start running it?
			d->pool()->d_func()->stealAndRunRunnable(d->runnable);

			lock.relock();

			while (isRunning())
				d->waitCondition.wait(&d->m_mutex);
		}

		d->m_exceptionStore.throwPossibleException();
	}
 * @endcode
 *
 * @param initialState  Defaults to State(Started | Running).  Does not appear to waitForFinished()
 *        if it isn't both Started and Running.
 *
 *
 */

// Associated header.
#include "ExtFuture.h"

// Std C++
#include <shared_mutex>
#include <map>
#include <algorithm>

// Qt5
#include <QFuture>
#include <QThread>

// Ours
#include "ExtFuturePropagationHandler.h"



/// @name Explicit instantiations to try to get compile times down.
template class ExtFuture<Unit>;
template class ExtFuture<bool>;
template class ExtFuture<int>;
template class ExtFuture<long>;
template class ExtFuture<std::string>;
template class ExtFuture<double>;
template class ExtFuture<QString>;
template class ExtFuture<QByteArray>;




