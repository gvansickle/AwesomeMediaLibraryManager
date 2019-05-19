/*
 * Copyright 2018, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef EXTFUTUREIMPLHELPERS_H
#define EXTFUTUREIMPLHELPERS_H

// Std C++
#include <type_traits>
#include <functional>
#include <thread>
#include <future>
#include <condition_variable>
#include <chrono>

// Qt5
#include <QFutureWatcher>
#include <QList>
#include <QThreadPool>

// Boost
#include <boost/thread.hpp>

// Ours
#include <utils/DebugHelpers.h>
#include <future/Unit.hpp>
#include <concurrency/ExtAsyncExceptions.h>
//#include "ExtAsync_RunInThread.h"
#include <concurrency/ExtAsync.h>
#include <concurrency/ExtFutureWatcher.h>
#include <concurrency/impl/ManagedExtFutureWatcher_impl.h>

// Generated
//#include "logging_cat_ExtFuture.h"

template <class T>
class ExtFuture;

namespace ExtFuture_detail
{
/**
 * A helper for ExtFuture<T>.waitForFinished() which ignores isRunning() and only returns based on
 * isCanceled() || isFinished().
 * .waitForFinished() first looks at the isRunning() state and treats it like an already-finished state:
 * @code
 * 	void QFutureInterfaceBase::waitForFinished()
	{
		QMutexLocker lock(&d->m_mutex);
		const bool alreadyFinished = !isRunning();
		lock.unlock();

		if (!alreadyFinished) {
			d->pool()->d_func()->stealAndRunRunnable(d->runnable);
		[...]
 * @endcode
 * You can see that in this case, it will actually "steal the runnable" and run it.
 * Sometimes this is not what we need, e.g. the race between a call to Whatever::run() and the returned
 * future actually getting into the Running state.
 * @note Or am I wrong there?
 *
 * Busy-waits like this are of course gross, there's probably a better way to do this.
 *
 * @note If you have the QThreadPool you're running this function in, use this overload.  In the spin-wait, it does a releaseThread()/yield()/reserveThread() on @p tp in order
 * to prevent the QThreadPool running out of threads and deadlocking.
 */
template <class T, class U>
static inline void spinWaitForFinishedOrCanceled(QThreadPool* tp, const ExtFuture<T>& this_future, const ExtFuture<U>& downstream_future)
{
	/// queryState() does this:
	/// bool QFutureInterfaceBase::queryState(State state) const
	/// {
	///    return d->state.load() & state;
	/// }
	/// So this:
	///     this_future.d.queryState(QFutureInterfaceBase::Canceled | QFutureInterfaceBase::Finished)
	/// Should return true if either bit is set.
	constexpr auto canceled_or_finished = QFutureInterfaceBase::State(QFutureInterfaceBase::Canceled | QFutureInterfaceBase::Finished);
	while(!this_future.d.queryState(canceled_or_finished)
		  && !downstream_future.d.queryState(canceled_or_finished))
	{
		// Temporarily raise tp's maxThreadCount() while we yield, allowing other threads to continue.
		tp->releaseThread();
		QThread::yieldCurrentThread();
		// Lower the maxThreadCount().
		tp->reserveThread();
	}
}

/**
 * Block in a busy-wait loop until one of the futures is fulfilled.
 * Same idea as above, but without a threadpool, and with FutureWatchers and std:: mechanics.
 * @param this_future
 * @param downstream_future
 */
template <class T, class U>
static inline void spinWaitForFinishedOrCanceled(ExtFuture<T> this_future, ExtFuture<U> downstream_future)
{
	/// queryState() does this:
	/// bool QFutureInterfaceBase::queryState(State state) const
	/// {
	///    return d->state.load() & state;
	/// }
	/// So this:
	///     this_future.d.queryState(QFutureInterfaceBase::Canceled | QFutureInterfaceBase::Finished)
	/// Should return true if either bit is set.
	constexpr auto canceled_or_finished = QFutureInterfaceBase::State(QFutureInterfaceBase::Canceled | QFutureInterfaceBase::Finished);

//	boost::shared_future<bool> f1, f2, f4, f8;
//	boost::promise<bool> p0;
	std::atomic_int done_flag = 0;
	std::mutex m;
	std::condition_variable cv;

	auto* fw_down = ManagedExtFutureWatcher_detail::get_managed_qfuture_watcher<U>("[then down->up]");
	auto* fw_up = ManagedExtFutureWatcher_detail::get_managed_qfuture_watcher<T>("[then up->down]");

//	f1 = p0.get_future();
//	f2 = p0.get_future();
//	f4 = p0.get_future();
//	f8 = p0.get_future();

	QObject* temp_parent = new QObject(); //dynamic_cast<ManagedExtFutureWatcher_detail::FutureWatcherParent*>(fw_down->parent())->get_temp_context_object();

	connect_or_die(fw_down, &QFutureWatcher<U>::canceled, temp_parent/*fw_up,*/, [&](){
		done_flag = 1;
//		p0.set_value(true);
		cv.notify_all();
	});
	connect_or_die(fw_down, &QFutureWatcher<U>::finished, temp_parent/*fw_up,*/, [&](){
		done_flag = 2;
//		p0.set_value(true);
		cv.notify_all();
	});
	connect_or_die(fw_up, &QFutureWatcher<T>::canceled, temp_parent/*fw_down,*/, [&](){
		done_flag = 4;
//		p0.set_value(true);
		cv.notify_all();
	});
	connect_or_die(fw_up, &QFutureWatcher<T>::finished, temp_parent/*fw_down,*/, [&](){
		done_flag = 8;
//		p0.set_value(true);
		cv.notify_all();
	});

	// Block until one of the futures is done.
	qDb() << "ABOUT TO BLOCK FOR ANY FINISHED";
	{
		using namespace std::chrono_literals;
		std::unique_lock<std::mutex> lk(m);

		// Set the watchers' futures.
		fw_down->setFuture(downstream_future);
		fw_up->setFuture(this_future);

//		std::cv_status cvs = std::cv_status::no_timeout;
		bool retval = false;
		do
		{
			retval = cv.wait_for(lk, 100ms, [&]{
				return done_flag != 0;
			});
		} while(retval == false);
//		boost::wait_for_any(f1, f2, f4, f8);
	}
	qDb() << "Wait over, result was:" << done_flag;

	temp_parent->disconnect();
	temp_parent->deleteLater();

	/// @todo Destroy fw's here?

}

//	template <class T, class R>
//	void exception_propagation_helper_spinwait(ExtFuture<T> upstream, ExtFuture<R> downstream)
//	{
//		// Have to be at least started prior to getting in here.
//		Q_ASSERT(upstream.isStarted());
//		Q_ASSERT(downstream.isStarted());

//		// Blocks (busy-wait with yield) until one of the futures is canceled or finished.
//		spinWaitForFinishedOrCanceled(upstream, downstream);

//		/// Spinwait is over, now we have ??? combinations to dispatch on along at least 4 dimensions:
//		/// - this_future_copy / ret_future_copy (/both???)
//		/// - hasException() (== also canceled) vs. no exception
//		/// - isCanceled() with no exception vs. not canceled
//		/// - isFinished() vs. not finished.
//		///
//		/// ...and we haven't even gotten to the actual callback yet.
//	}





}; // END ns ExtFuture_detail

#endif // EXTFUTUREIMPLHELPERS_H
