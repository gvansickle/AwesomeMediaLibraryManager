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
 * - Cancellation and Exceptions
 *
 * Per std::experimental::shared_future::then() at @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
 * "Any value returned from the continuation is stored as the result in the shared state of the returned future object.
 *  Any exception propagated from the execution of the continuation is stored as the exceptional result in the shared
 *  state of the returned future object."
 *
 * Per @link https://software.intel.com/en-us/node/506075 (tbb), referring to task_group_context objects:
 * "Exceptions propagate upwards. Cancellation propagates downwards. The opposition interplays to cleanly stop a nested
 * computation when an exception occurs."
 * [tree a->b->c etc.]
 * "Suppose that the algorithm in C throws an exception and no node catches the exception. Intel TBB propagates the exception
 *  upwards, cancelling related subtrees downwards, as follows:
 *

    Handle exception in C:

        Capture exception in C.

        Cancel tasks in C.

        Throw exception from C to B.

    Handle exception in B:

        Capture exception in B.

        Cancel tasks in B and, by downwards propagation, in D.

        Throw an exception out of B to A.

    Handle exception in A:

        Capture exception in A.

        Cancel tasks in A and, by downwards propagation, in E, F, and G.

        Throw an exception upwards out of A.
 *
 *  If your code catches the exception at any level, then Intel TBB does not propagate it any further."
 *
 *  For us that would mean that in pre-then(), we should:
 *  @code
 *  try
 *  {
 *  	waitForFinished();
 *  }
 *  catch(ExtAsyncCancelException& e)
 *  {
 *		// Cancel returned ExtFuture<T>.
 *		ret_future_copy.cancel();
 *
 *		// Throw an exception upwards, i.e. reportException() to this.
 *		this->reportException(e);
 *  }
 *  @endcode
 *
 *  Canceling a string of .then()'s would have to start either with:
 *  - At the outermost returned future, causing it to throw an exception, which will do as outlined above.
 *  - At the innermost future (the root async "generator"), calling .cancel(), which will propagate down.
 */

// Associated header.
#include "ExtFuture.h"

// Std C++
#include <shared_mutex>
#include <map>
#include <algorithm>
#include <type_traits>

// Qt5
#include <QFuture>
#include <QThread>

// Ours
//#include "ExtFuturePropagationHandler.h"

static std::atomic_uint64_t f_future_id_ctr {2};

/**
 *
 */
static QThreadPool s_cancel_threadpool = QThreadPool();

std::atomic_uint64_t get_next_id()
{
	return f_future_id_ctr.fetch_add(1);
}

/// @name Explicit instantiations to try to get compile times down.
template class ExtFuture<Unit>;
template class ExtFuture<bool>;
template class ExtFuture<int>;
template class ExtFuture<long>;
template class ExtFuture<unsigned long>;
template class ExtFuture<unsigned long long>;
template class ExtFuture<std::string>;
template class ExtFuture<double>;
template class ExtFuture<QString>;
template class ExtFuture<QByteArray>;

/// Static checks.
static_assert(std::is_class_v<ExtFuture<QList<int>>>);
static_assert(std::is_convertible_v<QList<int>, ExtFuture<int>>);
//static_assert(std::is_convertible_v<ExtFuture<QList<int>>, ExtFuture<int>>);


