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

#ifndef EXTFUTUREIMPLHELPERS_H
#define EXTFUTUREIMPLHELPERS_H

template <class T>
class ExtFuture;

/**
 * A helper .waitForFinished() replacement which ignores isRunning() and only returns based on
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
 * Busy-waits like this are of course gross, there's probably a better way to do this.
 */
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
		QThread::yieldCurrentThread();
	}
}

/**
 * Attach downstream_future to this_future (a copy of this ExtFuture) such that any cancel or exception thrown by
 * downstream_future cancels this_future.
 *
 * @param downstream_future
 */
template <class T, class U>
static QFuture<int> AddDownstreamCancelFuture(ExtFuture<T> this_future, ExtFuture<U> downstream_future)
{
	return QtConcurrent::run([=](ExtFuture<T> this_future_copy, ExtFuture<U> downstream_future_copy) -> int {

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
//					downstream_future_copy.waitForFinished();
				downstream_future_copy.result();
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

#endif // EXTFUTUREIMPLHELPERS_H
