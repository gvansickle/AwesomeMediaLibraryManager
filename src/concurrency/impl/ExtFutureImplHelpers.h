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

// Qt5
#include <QFutureWatcher>
#include <QList>
#include <QThreadPool>

// Ours
#include <utils/DebugHelpers.h>
#include <future/Unit.hpp>
#include <concurrency/ExtAsyncExceptions.h>
//#include "ExtAsync_RunInThread.h"
#include <concurrency/ExtAsync.h>
#include <concurrency/ExtFutureWatcher.h>

// Generated
#include "logging_cat_ExtFuture.h"

template <class T>
class ExtFuture;

template <class T, class CallbackType, class R,  class... Args>
void exception_propagation_helper_spinwait(ExtFuture<T> this_future_copy, ExtFuture<R> ret_future_copy,
		CallbackType&& callback, Args&&... args)
{
	// Have to be at least started prior to getting in here.
	Q_ASSERT(this_future_copy.isStarted());
	Q_ASSERT(ret_future_copy.isStarted());

	// We should never end up calling then_callback_copy with a non-finished future; this is the code
	// which will guarantee that.
	// This could throw a propagated exception from upstream (this_future_copy).
	// Per @link https://medium.com/@nihil84/qt-thread-delegation-esc-e06b44034698, we can't just use
	// this_future_copy.waitForFinished() here because it will return immediately if the thread hasn't
	// "really" started (i.e. if isRunning() == false).
	/// @todo Is that correct though?
//				if(!this_future_copy.isRunning())
	{
//		qCDebug(EXTFUTURE) << "START SPINWAIT";
		// Blocks (busy-wait with yield) until one of the futures is canceled or finished.
		spinWaitForFinishedOrCanceled(QThreadPool::globalInstance(), this_future_copy, ret_future_copy);

//		qCDebug(EXTFUTURE) << "END SPINWAIT";
	}

	/// Spinwait is over, now we have ??? combinations to dispatch on along at least 4 dimensions:
	/// - this_future_copy / ret_future_copy (/both???)
	/// - hasException() (== also canceled) vs. no exception
	/// - isCanceled() with no exception vs. not canceled
	/// - isFinished() vs. not finished.
	///
	/// ...and we haven't even gotten to the actual callback yet.


	if(ret_future_copy.hasException())
	{
		// Downstream threw, which .cancels() (doesn't have to be a cancel exception).
		// We need to propagate the exception to this_future_copy.
		/// @todo Assuming anything attached to ret_future_copy will worry about it's throwing.
		// Trigger an immediate rethrow.  @todo Is this really correct?
		// This will be caught below.
		Q_ASSERT(ret_future_copy.isCanceled());
		/// @todo We're losing the original exception here, and it's not clear if there's any way to get it other than trigger
		/// a throw; that will change some state to "has thrown", which I don't know what that effects.
		/// Not sure what we can do about this atm.
		this_future_copy.reportException(ExtAsyncCancelException());
		this_future_copy.reportFinished();
		this_future_copy.waitForFinished();
		Q_ASSERT_X(0, __func__, "Attempt to rethrow exception from returned future failed.");
	}
	// Was downstream canceled without an exception?
	else if(ret_future_copy.isCanceled())
	{
		// Downstream was canceled without an exception.
		/// @todo Assuming anything attached to ret_future_copy will worry about it's throwing.
		qDb() << "returned_future_copy CANCELED WITHOUT EXCEPTION:" << ret_future_copy;
		// Propagate cancel to this_future_copy.  Since ret_future_copy is already canceled, we can't get an exception through
		// it here.
		this_future_copy.reportException(ExtAsyncCancelException());
		this_future_copy.reportFinished();
		this_future_copy.waitForFinished();
		Q_ASSERT_X(0, __func__, "Attempt to rethrow exception from returned future failed.");
	}
	else if(ret_future_copy.isFinished())
	{
		// Downstream is already Finished, but not canceled.
		// Not clear that this is a valid, or even possible, state here.
		qWr() << "returned_future_copy FINISHED?:" << ret_future_copy;
		Q_ASSERT_X(0, __func__, "Future returned by then() is finished first, shouldn't be possible.");
	}

	if(this_future_copy.hasException())
	{
		// This threw, which cancels (doesn't have to be a cancel exception).
		// Trigger an immediate rethrow.  This will be caught below.
		Q_ASSERT(this_future_copy.isCanceled());
		this_future_copy.reportFinished();
		this_future_copy.waitForFinished();
		Q_ASSERT_X(0, __func__, "Attempt to rethrow exception from this future failed.");
	}
		// Was this_future_copy canceled without an exception?
	else if(this_future_copy.isCanceled())
	{
		// This was canceled without an exception, i.e. with a direct call to .cancel().
		// We can't add a cancel exception to this_future_copy now, it'll be ignored.
		// But we do have to try to propagate this downstream.
		qWr() << "this_future_copy CANCELED WITHOUT EXCEPTION:" << this_future_copy.state();
		ret_future_copy.reportException(ExtAsyncCancelException());
		ret_future_copy.reportFinished();
	}
		// Did this_future_copy Finish first?
	else if(this_future_copy.isFinished())
	{
		// Normal finish of this_future_copy, no cancel or exception to propagate.
//			qDb() << "THIS_FUTURE FINISHED NORMALLY";
	}

	// Now we're either Finished or Canceled, so we can call waitForFinished().  We now know
	// the state of isRunning() does reflect if we're done or not.
	// This call will block, or throw if an exception is reported to this_future_copy.
	this_future_copy.reportFinished();
	this_future_copy.waitForFinished();

	// Ok, so now we're definitely finished and/or canceled.

	Q_ASSERT(this_future_copy.isFinished() || this_future_copy.isCanceled());

	// Got here, so we didn't throw.  We might still be canceled.

}



/**
 * Template to try to get a common handle on exception and cancel handling.
 * CallbackType == ExtFuture<R> callback(ExtFuture<T> this_future, args...)
 *
 * For an ExtFuture<T>::then(), the futures should be *this and the returned_future, resp.
 */
template <class T, class CallbackType, class R,  class... Args>
void exception_propagation_helper_then(ExtFuture<T> this_future_copy, ExtFuture<R> ret_future_copy, CallbackType&& callback, Args&&... args)
{
	bool call_on_cancel = false;

	// Have to be at least started prior to getting in here.
	Q_ASSERT(this_future_copy.isStarted());
	Q_ASSERT(ret_future_copy.isStarted());


	try
	{
		exception_propagation_helper_spinwait(this_future_copy, ret_future_copy, callback, args...);
	}
	// Handle exceptions and cancellation.
	// Exceptions propagate upwards, cancellation propagates downwards.
	/// @note ^^^That is somewhat backwards from what
	/// @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
	/// @link http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0701r1.html#passing-futures-to-then-continuations-is-unwieldy
	/// seem to advocate.  But if we're in a .then() in the middle of a series, I think that we have to do it either this way,
	/// or also throw down to the returned future on cancel, especially since a QFuture<> .cancel() doesn't throw by itself.
	catch(ExtAsyncCancelException& e)
	{
		/**
		 * Per std::experimental::shared_future::then() at @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
		 * "Any value returned from the continuation is stored as the result in the shared state of the returned future object.
		 *  Any exception propagated from the execution of the continuation is stored as the exceptional result in the shared
		 *  state of the returned future object."
		 */
//		qDb() << "CAUGHT CANCEL, THROWING TO DOWSTREAM (RETURNED) FUTURE";
//		qDb() << "CAUGHT CANCEL, THROWING TO UPSTREAM (THIS) FUTURE";
		ret_future_copy.reportException(e);
		this_future_copy.reportException(e);
	}
	catch(QException& e)
	{
//		qDb() << "CAUGHT EXCEPTION, CANCELING DOWSTREAM (RETURNED) FUTURE";
//		qDb() << "CAUGHT EXCEPTION, THROWING TO UPSTREAM (THIS) FUTURE";
		ret_future_copy.reportException(e);
		this_future_copy.reportException(e);
	}
	catch (...)
	{
//		qDb() << "CAUGHT UNKNOWN EXCEPTION, CANCELING DOWSTREAM (RETURNED) FUTURE";
//		qDb() << "CAUGHT UNKNOWN EXCEPTION, THROWING TO UPSTREAM (THIS) FUTURE";
		ret_future_copy.reportException(QUnhandledException());
		this_future_copy.reportException(QUnhandledException());
	}

	///
	/// Oh we are not done yet....
	///

	// One last loose end.  If we get here, one or both of the futures may have been canceled by .cancel(), which
	// doesn't throw.  So:
	// - In run() we have nothing to do but return.
	// - In then() (here) we'll have to do the same thing we do for a cancel exception.
	if(ret_future_copy.isCanceled() || this_future_copy.isCanceled())
	{
		// if constexpr(in_a_then) { <below> };
		qDb() << "CANCELED, CANCELING DOWSTREAM (RETURNED) FUTURE" << ret_future_copy;
		ret_future_copy.reportException(ExtAsyncCancelException());
		qDb() << "CANCELED, THROWING TO UPSTREAM (THIS) FUTURE";
		this_future_copy.reportException(ExtAsyncCancelException());
	}

	//
	// The this_future_copy.waitForFinished() above either returned and the futures weren't canceled,
	// or may have thrown above and been caught and reported.
	//

	Q_ASSERT_X(this_future_copy.isFinished(), "then outer callback", "Should be finished here.");

	// Could have been a normal finish or a cancel.

	// Should we call the then_callback?
	// Always on Finished, maybe not if canceled.
	if(this_future_copy.isFinished() || (call_on_cancel && this_future_copy.isCanceled()))
	{
		///
		/// Ok, this_future_copy is finally finished and we can call the callback.
		/// Man that was almost as bad as what's coming up next.
		///
		qDb() << "THEN: CALLING CALLBACK, this_future_copy:" << this_future_copy;

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
				std::invoke(std::move(callback), this_future_copy);
				retval = unit;
			}
			else
			{
				// then_callback_copy returns non-void, return the callback's return value.
				qDb() << "INVOKING ret type != Unit";
				retval = std::invoke(std::move(callback), this_future_copy);
			}
			qDb() << "INVOKED";

			// Didn't throw, report the result.
			ret_future_copy.reportResult(retval);
		}
		// One more time, Handle exceptions and cancellation, this time of the callback itself.
		// Exceptions propagate upwards, cancellation propagates downwards.
		catch(ExtAsyncCancelException& e)
		{
			qDb() << "CAUGHT CANCEL, THROWING DOWSTREAM (RETURNED) FUTURE";
			ret_future_copy.reportException(e);
			qDb() << "CAUGHT CANCEL, THROWING TO UPSTREAM (THIS) FUTURE";
			this_future_copy.reportException(e);
		}
		catch(QException& e)
		{
			qDb() << "CAUGHT EXCEPTION, CANCELING DOWSTREAM (RETURNED) FUTURE";
			ret_future_copy.reportException(e);
			qDb() << "CAUGHT EXCEPTION, THROWING TO UPSTREAM (THIS) FUTURE";
			this_future_copy.reportException(e);
		}
		catch (...)
		{
			qDb() << "CAUGHT UNKNOWN EXCEPTION, CANCELING DOWSTREAM (RETURNED) FUTURE";
			ret_future_copy.reportException(QUnhandledException());
			qDb() << "CAUGHT UNKNOWN EXCEPTION, THROWING TO UPSTREAM (THIS) FUTURE";
			this_future_copy.reportException(QUnhandledException());
		}

		// One more last loose end.  If we get here, one or both of the futures may have been canceled by .cancel(), which
		// doesn't throw.  So:
		// - In run() we have nothing to do but return.
		// - In then() (here) we'll have to do the same thing we do for a cancel exception.
		if(ret_future_copy.isCanceled() || this_future_copy.isCanceled())
		{
			// if constexpr(in_a_then) { <below> };
			qDb() << "CANCELED, CANCELING DOWSTREAM (RETURNED) FUTURE" << ret_future_copy;
			ret_future_copy.cancel();
			qDb() << "CANCELED, THROWING TO UPSTREAM (THIS) FUTURE";
			this_future_copy.reportException(ExtAsyncCancelException());
//					return;
		}
	}
	else if (call_on_cancel || !(this_future_copy.isFinished() || this_future_copy.isCanceled()))
	{
		// Something went wrong, we got here after .waitForFinished() returned or threw, but
		// the this_future_status isn't Finished or Canceled.
		Q_ASSERT_X(0, __PRETTY_FUNCTION__, ".waitForFinished() returned or threw, but this_future_status isn't Finished or Canceled");
	}
	else
	{
		// Not Finished, have to be Canceled.
		Q_ASSERT(this_future_copy.isCanceled());
	}

	/// See ExtAsync, again not sure if we should finish here if canceled.
	/// @todo I think this is wrong on a cancel.
	ret_future_copy.reportFinished();
}

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

namespace ExtFuture_detail
{

	/**
	 * Mostly just a stub QObject-derived class at the moment for use as a parent in the future watcher thread.
	 */
	class FutureWatcherParent : public QObject
	{
		Q_OBJECT

	public:
		explicit FutureWatcherParent(QObject* parent = nullptr) : QObject(parent) {};

	public Q_SLOTS:
		void doWork(int param)
		{
			/// @todo Do something in the other thread...

			// Emit the result as a signal.
//			Q_EMIT resultReady(fw);
		};

	Q_SIGNALS:
		void resultReady(int result);
	};

	/// @todo DELETE THIS
//	class FutureWatcherLifecycleController : public QObject
//	{
//		Q_OBJECT
//
//	public:
//		explicit FutureWatcherLifecycleController(QObject* parent = nullptr) : QObject(parent)
//		{
//			m_fwthread.setObjectName("FutureWatcherLifecycleThread");
//			FutureWatcherParent* fwp = new FutureWatcherParent;
//			fwp->moveToThread(&m_fwthread);
//			connect_or_die(&m_fwthread, &QThread::finished, fwp, &QObject::deleteLater);
//			connect_or_die(this, &FutureWatcherLifecycleController::operate, fwp, &FutureWatcherParent::doWork);
//			connect_or_die(fwp, &FutureWatcherParent::resultReady, this, &FutureWatcherLifecycleController::handleResults);
//			m_fwthread.start();
//		}
//		~FutureWatcherLifecycleController() override
//		{
//			m_fwthread.quit();
//			m_fwthread.wait();
//		}
//
//
//
//	public Q_SLOTS:
//		void handleResults(int result)
//		{
//			/// @todo
//		}
//
//	Q_SIGNALS:
//		void operate(int param);
//
//	private:
//
//		QThread m_fwthread;
//	};

	/// This is semi-gross, it's the QObject which will be the parent of all managed future watchers.
	static inline FutureWatcherParent* f_the_managed_fw_parent = nullptr;

	/// Returns the pointer to the QThread which is to be used for running QFutureWatchers which
	/// implement inter-future status propagation (cancellation and exceptions).
	static inline QThread* get_backprop_qthread()
	{
		static QThread* backprop_thread = []{
			QThread* new_thread = new QThread;
			new_thread->setObjectName("ExtFutureBackpropThread");
			PerfectDeleter::instance().addQThread(new_thread, [](QThread* the_qthread){
				// Call exit(0) on the QThread.  We use Qt's invokeMethod() here.
				ExtAsync::detail::run_in_event_loop(the_qthread, [the_qthread](){
					qDb() << "Calling quit()+wait() on managed FutureWatcher QThread";
					the_qthread->quit();
					the_qthread->wait();
				});
			});

			// No parent, this will be deleted by the signal below.
			f_the_managed_fw_parent = new FutureWatcherParent();
			// Create an push the future watcher parent object into the new thread.
			f_the_managed_fw_parent->moveToThread(new_thread);
			connect_or_die(new_thread, &QThread::finished, f_the_managed_fw_parent, &QObject::deleteLater);

			// Start the thread.
			new_thread->start();
			return new_thread;
		}();
		return backprop_thread;
	}



	/**
	 * Part of the system by which we get an exception from one ExtFuture<> into the state of another.
	 * This rethrows @a eptr, catches it, and finally reports it to @a future.
	 * @test Has test coverage, does work as expected.
	 * @param eptr
	 * @param future
	 */
	template <class R>
	void propagate_eptr_to_future(std::exception_ptr eptr, ExtFuture<R> future)
	{
		if(eptr)
		{
			// We did catch an exception.  Propagate it to the downstream_future.
			try
			{
				std::rethrow_exception(eptr);
			}
			catch(QException& e)
			{
				future.reportException(e);
			}
			catch(...)
			{
				future.reportException(QUnhandledException());
			}
			// future state here is (Started|Canceled|Running)/has_exception == true.
			// We need to finish it or any waits will block forever.
			future.reportFinished();
		}
		else
		{
			// eptr == null, there was no exception.
			Q_ASSERT(0);
		}
	}

	/**
	 * The other part of the exception propagation mechanism.
	 *
	 * @test Has test coverage, does work as expected.
	 */
	/// @note Only use this if you know upstream has an exception to throw to downstream.
	template <class T, class R>
	void trigger_exception_and_propagate(ExtFuture<T> upstream_future, ExtFuture<R> downstream_future)
	{
		std::exception_ptr eptr;
		try
		{
			// Trigger the throw.
			upstream_future.wait();
		}
		catch(...)
		{
			// Capture the exception.
			eptr = std::current_exception();
		}
		propagate_eptr_to_future(eptr, downstream_future);
	}


	template <class T>
	class WatcherDeleter
	{

	public:
		WatcherDeleter() = default;
		explicit WatcherDeleter(QFutureWatcher<T>* f) : m_watcher(f) {};
		~WatcherDeleter() = default;

		void make_delete_connections()
		{
			connect_or_die(m_watcher, &QFutureWatcher<T>::finished, [=]() mutable { this->delete_later_if_canceled_and_finished(); });
			connect_or_die(m_watcher, &QFutureWatcher<T>::canceled, [=]() mutable { this->delete_later_if_canceled_and_finished(); });
		}
	public:
		void delete_later_if_canceled_and_finished()
		{
			if(/*m_watcher->isCanceled() &&*/ m_watcher->isFinished())
			{
				qDb() << "QFutureWatcher<> is canceled/finished, deleting:" << m_watcher;
				m_watcher->deleteLater();
			}
		}

		QPointer<QFutureWatcher<T>> m_watcher {nullptr};
	};



	/**
	 * Factory function for creating managed QFutureWatcher<>s.
	 * @returns A pointer to the new watcher.  This is a raw pointer, and the watcher has no parent.
	 */
	template <class T>
	QFutureWatcher<T>* get_managed_qfuture_watcher()
	{
		QThread* bp_thread = ExtFuture_detail::get_backprop_qthread();
		// Create the new watcher, unparented so we can move it to the backprop thread.
		QFutureWatcher<T>* watcher = new QFutureWatcher<T>();

		// Move the watcher to the backprop thread.
		/// @note Per Qt5: "The object cannot be moved if it has a parent."
		watcher->moveToThread(bp_thread);

		// Ok, now it's in the other thread but has no parent.
		// We'll give it one.
		watcher->setParent(f_the_managed_fw_parent);
		qDb() << "FutureWatcher" << watcher << "has parent:" << watcher->parent();

		return watcher;
	}

	template<class R, class T>
	void connect_or_die_down2up_cancel(QPointer<QFutureWatcher<R>> downstream, QObject* context, ExtFuture<T> upstream_future)
	{
		// If down is canceled, cancel upstream.
		// If it was upstream that did the canceling, this effectively is a no-op.
		connect_or_die(downstream, &QFutureWatcher<R>::canceled, context, [upstream_future=upstream_future](){
			upstream_future.cancel();
		});
	}

	template<class R, class T>
	void connect_or_die_down2up_cancel(QPointer<QFutureWatcher<R>> downstream, QPointer<QFutureWatcher<T>> upstream_future_watcher)
	{
		// If down is canceled, cancel upstream.
		// If it was upstream that did the canceling, this effectively is a no-op.
		connect_or_die(downstream, &QFutureWatcher<R>::canceled, upstream_future_watcher, &QFutureWatcher<T>::cancel);
	}

	template <class T>
	void connect_or_die_deleteLater_on_finished(QPointer<QFutureWatcher<T>> fw)
	{
		auto delete_later_if_canceled_and_finished = [](QPointer<QFutureWatcher<T>> fw)
		{
			if(/*m_watcher->isCanceled() &&*/ fw->isFinished())
			{
				qDb() << "QFutureWatcher<> is finished (maybe canceled), deleting:" << fw;
				fw->deleteLater();
			}
		};

		connect_or_die(fw, &QFutureWatcher<T>::finished,
					   [=,
					   delete_later_if_canceled_and_finished=DECAY_COPY(delete_later_if_canceled_and_finished)]() mutable {
			delete_later_if_canceled_and_finished(fw);
		});
		connect_or_die(fw, &QFutureWatcher<T>::canceled, [=]() mutable { delete_later_if_canceled_and_finished(fw); });
	}

	template <class T, class R, class ResultsReadyAtCallbackType = std::nullptr_t, class WatcherConnectionCallback = std::nullptr_t>
	void SetBackpropWatcher(ExtFuture<T> upstream_future, ExtFuture<R> downstream_future,
							QObject* upstream_context, QObject* downstream_context,
							ResultsReadyAtCallbackType&& results_ready_callback = nullptr,
							WatcherConnectionCallback&& wc_callback = nullptr)
	{
		QThread* bp_thread = ExtFuture_detail::get_backprop_qthread();
		if(upstream_context == nullptr)
		{
			upstream_context = f_the_managed_fw_parent;
		}
		if(downstream_context == nullptr)
		{
			downstream_context = f_the_managed_fw_parent;
		}

		using FutureWatcherTypeR = QFutureWatcher<R>;
		using FutureWatcherTypeT = QFutureWatcher<T>;

		// Watchers will live in the backprop thread.
		QPointer<FutureWatcherTypeR> downstream_watcher = get_managed_qfuture_watcher<R>();
		QPointer<FutureWatcherTypeT> upstream_watcher = get_managed_qfuture_watcher<T>();

//		/// @todo Deleters, this should be encapsulated better.
//		/// @todo Also this is going to leak.
//		WatcherDeleter<R>* downstream_watcher_deleter = new WatcherDeleter<R>(downstream_watcher);
//		WatcherDeleter<T>* upstream_watcher_deleter = new WatcherDeleter<T>(upstream_watcher);

		// R->T ("upstream")
		// cancel signal.
		connect_or_die_down2up_cancel(downstream_watcher, upstream_watcher);
//		connect_or_die(downstream_watcher, &FutureWatcherTypeR::canceled, upstream_context, [=,
//					   upstream_future_copy=DECAY_COPY(std::forward<ExtFuture<T>>(upstream_future))]() mutable {
//			// Note we directly call cancel() (but from context's thread) because upstream_future may not have an event loop.
//			upstream_future_copy.cancel();
//		});
		// finished signal.
		/// @note Should only ever get this due to an exception thrown into R, and then we should probably have gotten a cancel instead.
		connect_or_die(downstream_watcher, &FutureWatcherTypeR::finished, upstream_context, [=,
					   upstream_future_copy=DECAY_COPY(std::forward<ExtFuture<T>>(upstream_future))]() mutable {
			// Note we directly call cancel() (but from context's thread) because upstream_future may not have an event loop.
			upstream_future_copy.reportFinished();
		});

		// T->R ("downstream") signals.
		// The resultsReadyAt() signal -> results_ready_callback.
//		if constexpr(std::is_invocable_r_v<void, ResultsReadyAtCallbackType, int, int>)
//		{
			connect_or_die(upstream_watcher, &FutureWatcherTypeR::resultsReadyAt, downstream_context,
						   [=,
						   upstream_future_copy=DECAY_COPY(/*std::forward<ExtFuture<T>>*/(upstream_future)),
						   callback_cp=DECAY_COPY(std::forward<ResultsReadyAtCallbackType>(results_ready_callback))](int begin, int end) mutable {
				std::invoke(callback_cp, upstream_future, begin, end);
				/// @note We're temporarily copying to the output future here, we should change that to use a separate thread.
				///       Or maybe we can just return the upstream_future here....
				for(int i = begin; i < end; ++i)
				{
					downstream_future.reportResult(upstream_future_copy, i);
				}
			});
//		}
//		// The T->R data xfer.
//		connect_or_die(upstream_watcher, &FutureWatcherTypeR::resultsReadyAt, downstream_context,
//		               [=,upstream_future_copy=DECAY_COPY(/*std::forward<ExtFuture<T>>*/(upstream_future)),
//				          callback_cp=DECAY_COPY(std::forward<ResultsReadyAtCallbackType>(results_ready_callback))](int begin, int end) mutable {
//			               /// @note We're temporarily copying to the output future here, we should change that to use a separate thread.
//			               ///       Or maybe we can just return the upstream_future here....
//			               for(int i = begin; i < end; ++i)
//			               {
//				               downstream_future.reportResult(upstream_future_copy, i);
//			               }
//		               });
		// Canceled.
		connect_or_die(upstream_watcher, &FutureWatcherTypeT::canceled, downstream_context,
				[=, downstream_future_copy=DECAY_COPY(downstream_future)]() mutable {
			if(upstream_future.has_exception())
			{
				// Throw exception to downstream future.  Takes a bit of shenanigans.
				trigger_exception_and_propagate(upstream_future, downstream_future_copy);
			}
			downstream_future_copy.reportCanceled();
		});
		// Finished.
		connect_or_die(upstream_watcher, &FutureWatcherTypeT::finished, downstream_context,
					   [=]() mutable { downstream_future.reportFinished(); });

		if constexpr(std::is_invocable_r_v<void, WatcherConnectionCallback, QPointer<QFutureWatcher<T>>, QPointer<QFutureWatcher<R>>>)
		{
			// Let the caller do any last-minute connections to the watchers.
			std::invoke(wc_callback, upstream_watcher, downstream_watcher);
		}

		// Connect these up last, they signal the watchers to deleteLater() themselves.
		connect_or_die_deleteLater_on_finished(downstream_watcher);
		connect_or_die_deleteLater_on_finished(upstream_watcher);

		// And set the futures.  It's just that easy.
		downstream_watcher->setFuture(downstream_future);
		upstream_watcher->setFuture(upstream_future);

	}

} // END ns ExtFuture_detail

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

	ExtFuture_detail::SetBackpropWatcher(this_future_copy, ret_future_copy,
										 context, context,
										 DECAY_COPY(std::forward<CallbackType>(callback)));

}

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
