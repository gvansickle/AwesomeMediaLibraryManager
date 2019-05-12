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

	template <class T, class R>
	void exception_propagation_helper_spinwait(ExtFuture<T> upstream, ExtFuture<R> downstream)
	{
		// Have to be at least started prior to getting in here.
		Q_ASSERT(upstream.isStarted());
		Q_ASSERT(downstream.isStarted());

		// Blocks (busy-wait with yield) until one of the futures is canceled or finished.
		spinWaitForFinishedOrCanceled(upstream, downstream);

		/// Spinwait is over, now we have ??? combinations to dispatch on along at least 4 dimensions:
		/// - this_future_copy / ret_future_copy (/both???)
		/// - hasException() (== also canceled) vs. no exception
		/// - isCanceled() with no exception vs. not canceled
		/// - isFinished() vs. not finished.
		///
		/// ...and we haven't even gotten to the actual callback yet.
	}

#if 1
	/**
	 * Template to try to get a common handle on exception and cancel handling.
	 * CallbackType == R callback(ExtFuture<T> this_future, args...)
	 *
	 * For an ExtFuture<T>::then(), the futures should be *this and the returned_future, resp.
	 */
	template <class T, class R, class CallbackType, class... Args>
	void exception_propagation_helper_then(ExtFuture<T> upstream, ExtFuture<R> downstream, CallbackType&& callback, Args&&... args)
	{
		bool call_on_cancel = false;

		// Have to be at least started prior to getting in here.
		Q_ASSERT(upstream.isStarted());
		Q_ASSERT(downstream.isStarted());

		// Wait until one of the futures is fulfilled.
		// In a try-catch because this could throw, and we need to propagate cancel and exceptions from upstream to downstream.
		/// @note actually not sure this can ever throw.
		try
		{
			exception_propagation_helper_spinwait(upstream, downstream);
		}
		catch(...)
		{
			Q_ASSERT_X(0, __func__, "This shouldn't have thrown.");
		}

		// Handle exceptions and cancellation.
		// Exceptions propagate downwards, cancellation propagates upwards.

		bool down_to_up_cancel = false;
		if(downstream.isCanceled())
		{
			if(downstream.has_exception())
			{
				/// @note Not sure if this is possible, or if throwing it back up is correct.
				ManagedExtFutureWatcher_detail::trigger_exception_and_propagate(downstream, upstream);
			}
			upstream.reportCanceled();
			upstream.reportFinished();
			down_to_up_cancel = true;
		}
		else if(downstream.isFinished())
		{
			// This really should be happening.
			Q_ASSERT_X(0, __func__, "Downstream shouldn't be finished.");
		}
		// Now upstream may have been canceled by the above, but we need to propagate it down so treelike
		// .then() chains get canceled properly.
		if(upstream.isCanceled())
		{
			if(upstream.has_exception())
			{
				ManagedExtFutureWatcher_detail::trigger_exception_and_propagate(upstream, downstream);
			}
			downstream.cancel();
		}

		try
		{
			// Now we just have to wait on upstream to finish, then call the callback.
			upstream.wait();

			R retval;
			if constexpr(std::is_same_v<R, Unit>)
			{
				std::invoke(callback, upstream);
			}
			else
			{
				retval = std::invoke(callback, upstream);
			}
			downstream.reportResult(retval);
			downstream.reportFinished();
			// And we're done.
		}
		catch(...)
		{
			// The upstream.wait() or the callback threw an exception.
			// Send it downstream.
			/**
			 * Per std::experimental::shared_future::then() at @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
			 * "Any value returned from the continuation is stored as the result in the shared state of the returned future object.
			 *  Any exception propagated from the execution of the continuation is stored as the exceptional result in the shared
			 *  state of the returned future object."
			 */
			std::exception_ptr eptr = std::current_exception();
			ManagedExtFutureWatcher_detail::propagate_eptr_to_future(eptr, downstream);
		}

		// One way or another, downstream is now finished.
		downstream.reportFinished();

#if 0
		///
		/// Oh we are not done yet....
		///

		// One last loose end.  If we get here, one or both of the futures may have been canceled by .cancel(), which
		// doesn't throw.  So:
		// - In run() we have nothing to do but return.
		// - In then() (here) we'll have to do the same thing we do for a cancel exception.
		if(downstream.isCanceled() || upstream.isCanceled())
		{
			// if constexpr(in_a_then) { <below> };
			qDb() << "CANCELED, CANCELING DOWSTREAM (RETURNED) FUTURE" << downstream;
			downstream.reportException(ExtAsyncCancelException());
			qDb() << "CANCELED, THROWING TO UPSTREAM (THIS) FUTURE";
			upstream.reportException(ExtAsyncCancelException());
		}

		//
		// The upstream.waitForFinished() above either returned and the futures weren't canceled,
		// or may have thrown above and been caught and reported.
		//

		Q_ASSERT_X(upstream.isFinished(), "then outer callback", "Should be finished here.");

		// Could have been a normal finish or a cancel.

		// Should we call the then_callback?
		// Always on Finished, maybe not if canceled.
		if(upstream.isFinished() || (call_on_cancel && upstream.isCanceled()))
		{
			///
			/// Ok, this_future_copy is finally finished and we can call the callback.
			/// Man that was almost as bad as what's coming up next.
			///
			qDb() << "THEN: CALLING CALLBACK, this_future_copy:" << upstream;

			try
			{
				// Call the callback with the results- or canceled/exception-laden this_future_copy.
				// Could throw, hence we're in a try.
				qDb() << "THENCB: Calling then_callback_copy(this_future_copy).";
				R retval;

				if constexpr(std::is_same_v<R, Unit>)
				{
					// then_callback_copy returns void, return a Unit separately.
					qDb() << "INVOKING ret type == Unit";
					std::invoke(std::move(callback), upstream);
					retval = unit;
				}
				else
				{
					// then_callback_copy returns non-void, return the callback's return value.
					qDb() << "INVOKING ret type != Unit";
					retval = std::invoke(std::move(callback), upstream);
				}
				qDb() << "INVOKED";

				// Didn't throw, report the result.
				downstream.reportResult(retval);
			}
			// One more time, Handle exceptions and cancellation, this time of the callback itself.
			// Exceptions propagate upwards, cancellation propagates downwards.
			catch(ExtAsyncCancelException& e)
			{
				qDb() << "CAUGHT CANCEL, THROWING DOWSTREAM (RETURNED) FUTURE";
				downstream.reportException(e);
				qDb() << "CAUGHT CANCEL, THROWING TO UPSTREAM (THIS) FUTURE";
				upstream.reportException(e);
			}
			catch(QException& e)
			{
				qDb() << "CAUGHT EXCEPTION, CANCELING DOWSTREAM (RETURNED) FUTURE";
				downstream.reportException(e);
				qDb() << "CAUGHT EXCEPTION, THROWING TO UPSTREAM (THIS) FUTURE";
				upstream.reportException(e);
			}
			catch (...)
			{
				qDb() << "CAUGHT UNKNOWN EXCEPTION, CANCELING DOWSTREAM (RETURNED) FUTURE";
				downstream.reportException(QUnhandledException());
				qDb() << "CAUGHT UNKNOWN EXCEPTION, THROWING TO UPSTREAM (THIS) FUTURE";
				upstream.reportException(QUnhandledException());
			}

			// One more last loose end.  If we get here, one or both of the futures may have been canceled by .cancel(), which
			// doesn't throw.  So:
			// - In run() we have nothing to do but return.
			// - In then() (here) we'll have to do the same thing we do for a cancel exception.
			if(downstream.isCanceled() || upstream.isCanceled())
			{
				// if constexpr(in_a_then) { <below> };
				qDb() << "CANCELED, CANCELING DOWSTREAM (RETURNED) FUTURE" << downstream;
				downstream.cancel();
				qDb() << "CANCELED, THROWING TO UPSTREAM (THIS) FUTURE";
				upstream.reportException(ExtAsyncCancelException());
			}
		}
		else if (call_on_cancel || !(upstream.isFinished() || upstream.isCanceled()))
		{
			// Something went wrong, we got here after .waitForFinished() returned or threw, but
			// the this_future_status isn't Finished or Canceled.
			Q_ASSERT_X(0, __PRETTY_FUNCTION__, ".waitForFinished() returned or threw, but this_future_status isn't Finished or Canceled");
		}
		else
		{
			// Not Finished, have to be Canceled.
			Q_ASSERT(upstream.isCanceled());
		}
		/// See ExtAsync, again not sure if we should finish here if canceled.
		/// @todo I think this is wrong on a cancel.
		downstream.reportFinished();
#endif

	}
#endif



}; // END ns ExtFuture_detail

/**
 * @todo doc fixes:
 * Template to try to get a common handle on exception and cancel handling.
 * CallbackType == void callback(ExtFuture<T> this_future, int begin, int end, args...)
 *
 * For an ExtFuture<T>::stap(), the futures should be *this and the returned_future, resp.
 *
 * @param context  The QObject*, and hence the thread, within which @a ret_future_copy lives.
 * @param this_future_copy  The upstream future.  May or may not live in the same thread as ret_future_copy, may not have an event loop.
 * @param ret_future_copy   The downstream future.  Must live in the same thread as @a context.
 */
template <class T, class CallbackType, class R,  class... Args>
void streaming_tap_helper_watcher(QObject* context, ExtFuture<T> this_future_copy, ExtFuture<R> ret_future_copy,
								  CallbackType&& callback, Args&&... args)
{
	static_assert(std::is_void_v<std::invoke_result_t<CallbackType, ExtFuture<T>, int, int/*, Args...*/>>, "Callback must return void.");

	ManagedExtFutureWatcher_detail::connect_or_die_backprop_cancel_watcher(this_future_copy, ret_future_copy);

//	ExtFutureWatcher_impl::SetBackpropWatcher(this_future_copy, ret_future_copy,
//										 context, context,
//										 DECAY_COPY(std::forward<CallbackType>(callback)));

};

#if 0
template <class T, class U>
static void spinWaitForFinishedOrCanceled(const ExtFuture<T>& this_future, const ExtFuture<U>& downstream_future)
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
		s_cancel_threadpool.releaseThread();
		QThread::yieldCurrentThread();
		s_cancel_threadpool.reserveThread();
	}
}
#endif

#if 0
template <class U>
static void spinWaitForFinishedOrCanceled(const ExtFuture<U>& future)
{
	constexpr auto canceled_or_finished = QFutureInterfaceBase::State(QFutureInterfaceBase::Canceled | QFutureInterfaceBase::Finished);

	while(!future.d.queryState(canceled_or_finished))
	{
		s_cancel_threadpool.releaseThread();
		QThread::yieldCurrentThread();
		s_cancel_threadpool.reserveThread();
	}
}
#endif

#if 0
/**
 * Attach downstream_future to this_future (a copy of this ExtFuture) such that any cancel or exception thrown by
 * downstream_future cancels this_future.
 *
 * @param downstream_future
 */
template <class T, class U>
static QFuture<int> PropagateExceptionsSecondToFirst(ExtFuture<T> this_future, ExtFuture<U> downstream_future)
{
	return QtConcurrent::run(&s_cancel_threadpool, [=](ExtFuture<T> this_future_copy, ExtFuture<U> downstream_future_copy) -> int {

		// When control flow gets here:
		// - this_future_copy may be in any state.  In particular, it may have been canceled or never started.
		// - downstream_future_copy may be in any state.  In particular, it may have been canceled or never started.

		Q_ASSERT(this_future_copy.isStarted());
		Q_ASSERT(downstream_future_copy.isStarted());

		try
		{
			// Give downstream_future_copy.isCanceled() somewhere to break to.
			do
			{
				// Blocks (busy-wait with yield) until one of the futures is canceled or finished.
				spinWaitForFinishedOrCanceled(this_future_copy, downstream_future_copy);

				/// Spinwait is over, now we have four combinations to dispatch on.

				// Spinwait is over, was downstream canceled?
				if(downstream_future_copy.isCanceled())
				{
					// Downstream canceled, this is why we're here.  Break out of the spinwait.
					qDb() << "downstream CANCELED:" << downstream_future_copy;
					break;
				}
				if(downstream_future_copy.isFinished())
				{
					// Downstream is already Finished, but not canceled.
					/// @todo Is that a valid state here?
					qWr() << "downstream FINISHED?:" << downstream_future_copy;
					return 1;
				}

				// Was this_future canceled?
				if(this_future_copy.isCanceled())
				{
					/// @todo Upstream canceled.
					qWr() << "this_future_copy CANCELED:" << this_future_copy.state();
					Q_ASSERT(0); // SHOULD CANCEL DOWNSTREAM.
					return 2;
				}

				// Did this_future_copy Finish first?
				if(this_future_copy.isFinished())
				{
					// Normal finish, no cancel or exception to propagate.
					qDb() << "THIS_FUTURE FINISHED NOT CANCELED, CANCELER THREAD RETURNING";
					return 3;
				}

				/// @note We never should get here.
				Q_ASSERT(0);
			}
			while(true);

			// downstream_future_copy usually == (Running|Started|Canceled) here, but may be in any state.
//			Q_ASSERT(downstream_future_copy.isCanceled());
//			Q_ASSERT(downstream_future_copy.isStarted());
//			Q_ASSERT(downstream_future_copy.isRunning());

			// .waitForFinished() will try to steal and run runnable, so we'll use result() instead.
			/// @todo This is even more heavyweight, maybe try to flip the "Running" state?
			/// Another possibility is waitForResult(int), it throws immediately, then might do a bunch
			/// of other stuff including work stealing.
			qDb() << "Spinwait complete, downstream_future_copy.result()...:" << downstream_future_copy;

			std::exception_ptr eptr; // For rethrowing.
			try /// @note This try/catch is just so we can observe the throw for debug.
			{
				downstream_future_copy.waitForFinished();
//				downstream_future_copy.result();
			}
			catch(ExtAsyncCancelException& e)
			{
				// downstream was canceled, propagate it as an exception to this.
				qDb() << "Caught downstream cancel, throwing to this";
				this_future_copy.reportException(e);
				return 8;
			}
			catch(...)
			{
				qDb() << "downstream_future_copy.result() threw, rethrowing. downstream_future_copy:" << downstream_future_copy;

				// Whatever the exception was, rethrow it out to the outer handler.
				// Capture the exception.
				eptr = std::current_exception();
			}

			// Do we need to rethrow?
			if(eptr)
			{
				qDb() << "rethrowing to outer try.";
				std::rethrow_exception(eptr);
			}

			// Else we didn't rethrow.
			qDb() << "waitForFinished() complete, did not throw:" << downstream_future_copy;

			// Didn't throw, could have been .cancel()ed, which doesn't directly throw.
			if(downstream_future_copy.isCanceled())
			{
				qDb() << "downstream_future CANCELED BY .cancel() CALL, CONVERTING TO EXCEPTION";
				this_future_copy.reportException(ExtAsyncCancelException());
			}
		}
		/// @todo I think this is all dead code.
		catch(ExtAsyncCancelException& e)
		{
			qDb() << "Rethrowing ExtAsyncCancelException";
			this_future_copy.reportException(e);
		}
		catch(QException& e)
		{
			qDb() << "Rethrowing QException";
			this_future_copy.reportException(e);
		}
		catch(...)
		{
			qDb() << "Rethrowing unknown exception";
			this_future_copy.reportException(QUnhandledException());
		};

		// downstream_future is completed one way or another, and above has already handled
		// and reported any exceptions upstream to this.  Or downstream_future has finished.
		// If we get here, there's really nothing for us to do.
		qDb() << "downstream_future_copy finished or canceled:" << downstream_future_copy;

		return 0;

		}, this_future, downstream_future);
}
#endif

#endif // EXTFUTUREIMPLHELPERS_H
