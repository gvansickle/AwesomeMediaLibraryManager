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

#ifndef UTILS_CONCURRENCY_IMPL_EXTFUTURE_IMPL_HPP_
#define UTILS_CONCURRENCY_IMPL_EXTFUTURE_IMPL_HPP_

template <class T>
class ExtFuture;

namespace ExtAsync
{
namespace detail
{

template<typename F, typename W, typename R>
struct then_helper
{
	F f;
	W w;

	then_helper(F f, W w)
		: f(std::move(f))
		, w(std::move(w))
	{
	}

	then_helper(const then_helper& other)
		: f(other.f)
		, w(other.w)
	{
	}

	then_helper(then_helper&& other)
		: f(std::move(other.f))
		, w(std::move(other.w))
	{
	}

	then_helper& operator=(then_helper other)
	{
		f = std::move(other.f);
		w = std::move(other.w);
		return *this;
	}

	R operator()()
	{
		f.wait();
		return w(std::move(f));
	}
};

} // END detail



} // END ExtAsync

#if 0 // templates.....
#define REAL_ONE_DOESNT_WORK
#ifdef REAL_ONE_DOESNT_WORK
template <typename T>
template <class ExtFutureExtFutureT,
		  REQUIRES(NestedExtFuture<ExtFutureExtFutureT>)>
ExtFuture<T>::ExtFuture(ExtFuture<ExtFuture<T>>&& other)
{
	Q_UNUSED(other);
	static_assert(NestedExtFuture<ExtFutureExtFutureT>, "Nested ExtFutures not supported");
}
#elif 0 // !REAL_ONE_DOESNT_WORK
	template <class ExtFutureExtFutureT,
			  REQUIRES(is_nested_ExtFuture_v<ExtFutureExtFutureT>)>
	ExtFuture(ExtFuture<ExtFuture<T>>&&	other)
	{
		/// @note Going by the description here @link https://en.cppreference.com/w/cpp/experimental/shared_future/shared_future
		/// "becomes ready when one of the following happens:
		/// - other and other.get() are both ready.
		///     The value or exception from other.get() is stored in the shared state associated with the resulting shared_future object.
		/// - other is ready, but other.get() is invalid. An exception of type std::future_error with an error condition of
		///     std::future_errc::broken_promise is stored in the shared state associated with the resulting shared_future object.
		/// "

		try
		{
			// This will either become ready or throw.
			QList<T> results = other.get();
			// Didn't throw, we've reached the first bullet.
			this->reportResults(results.toVector());
			this->reportFinished();
		}
		catch (...)
		{
			// Inner ExtFuture threw, we've reached the second bullet.  Rethrow.
			throw;
		}

	}
#endif // REAL_ONE_DOESNT_WORK
#endif // 0, disable the whole works.

template<typename T>
inline ExtFuture<T>& ExtFuture<T>::operator=(const ExtFuture<T>& other)
{
	if(this != &other)
	{
		this->BASE_CLASS::operator=(other);
		this->m_extfuture_id_no.store(other.m_extfuture_id_no);
//		this->m_progress_unit = other.m_progress_unit;
	}
	return *this;
}

template<typename T>
ExtFuture<T>& ExtFuture<T>::operator=(const ExtFuture::BASE_CLASS& other)
{
	if(this != &other)
	{
		this->BASE_CLASS::operator=(other);
		this->m_extfuture_id_no.store(0);
	}
	return *this;
}

template<typename T>
T ExtFuture<T>::qtget_first()
{
	wait();
	return this->result();
}


#if 0
/**
 * The root then() implementation.
 * Takes a context QObject and a then_callback where then_callback's signature is:
 * 	@code
 * 		then_callback(ExtFuture<T>) -> R
 * 	@endcode
 * where R is a non-ExtFuture<> return value.
 * If context == nullptr, then_callback will be run in an arbitrary thread.
 * If context points to a QObject, then_callback will be run in its event loop.
 *
 * Canceling
 * The returned future can be canceled, and the cancelation will propagate upstream (i.e. to this).
 *
 * Exceptions from callback
 * Per @link https://en.cppreference.com/w/cpp/experimental/shared_future/then:
 * "Any exception propagated from the execution of the continuation is stored as the exceptional result
 * in the shared state of the returned future object."
 * So we need to wrap the call to then_callback with a try/catch, and send any exceptions we catch
 * to the returned future.
 *
 * @param dont_call_on_cancel  If true, don't call the callback on a downstream cancel.
 */
template <typename T>
template <typename ThenCallbackType,
		  typename R,
		  REQUIRES(!is_ExtFuture_v<R>
		  && ct::is_invocable_r_v<R, ThenCallbackType, ExtFuture<T>>)>
ExtFuture<R> ExtFuture<T>::then(QObject* context, bool call_on_cancel, ThenCallbackType&& then_callback)
{
	if(context != nullptr)
	{
		// If non-null, make sure context has an event loop.
		/// @todo Use context.
		QThread* ctx_thread = context->thread();
		Q_ASSERT(ctx_thread != nullptr);
		Q_ASSERT(ctx_thread->eventDispatcher() != nullptr);
	}

	// The future we'll immediately return.  We copy this into the then_callback ::run() context.
	ExtFuture<R> returned_future;

	QtConcurrent::run(
		[=, then_callback_copy = std::decay_t<ThenCallbackType>(then_callback)]
				(ExtFuture<T> this_future_copy, ExtFuture<R> returned_future_copy) {

		// Ok, we're now running in the thread which will call then_callback_copy(this_future_copy).
		// At this point:
		// - The outer then() call may have already returned.
		// -- Hence retfuture, context may be gone off the stack.
		// - this_future_copy may or may not be finshed, canceled, or canceled with exception.
		// - this may be destructed and deleted already.

		Q_ASSERT(returned_future_copy != this_future_copy);

		qDb() << "ADDING DOWNSTREAM CANCELLER with RETFUTURE:" << returned_future_copy.state();
		AddDownstreamCancelFuture(this_future_copy, returned_future_copy);
		qDb() << "ADDED DOWNSTREAM CANCELLER with RETFUTURE:" << returned_future_copy.state();

		try
		{
			qDb() << "In .then() outer callback try block. this_future_copy:" << this_future_copy;
			// We should never end up calling then_callback_copy with a non-finished future; this is the code
			// which will guarantee that.
			// This could throw a propagated exception from upstream.
			// Per @link https://medium.com/@nihil84/qt-thread-delegation-esc-e06b44034698, we can't use
			// this_future.waitForFinished(); here because it will return immediately if the thread hasn't
			// "really" started (i.e. if isRunning() == false).
			spinWaitForFinishedOrCanceled(this_future_copy);

			// Ok, so now we're finished and/or canceled.
			// This call will block, or throw if an exception is reported to this_future_copy.
			this_future_copy.waitForFinished();

			qDb() << "Leaving Then callback Try, waitForFinished() finished:" << this_future_copy.state();
			Q_ASSERT(this_future_copy.isFinished() || this_future_copy.isCanceled());
		}
		catch(ExtAsyncCancelException& e)
		{
			// this_future_copy (upstream) threw a cancel exception.
			/// @todo So... do we .cancel() or .reportException() here?
			/// Or has the AddDownstreamCancelFuture() already handled this?
//				qDb() << "Rethrowing ExtAsyncCancelException downstream";
//				ret_future.reportException(e);
			returned_future_copy.cancel();
		}
		catch(QException& e)
		{
			qDb() << "Rethrowing QException downstream";
			returned_future_copy.reportException(e);
		}
		catch(...)
		{
			qDb() << "Rethrowing QUnhandledException downstream";
			returned_future_copy.reportException(QUnhandledException());
		};

		//
		// The waitForFinished() on this_future_copy either returned, or may have thrown above and been caught.
		//

		qDb() << "THENCB: WAITFINISHED:" << this_future_copy.state();
		// Could have been a normal finish or a cancel.

		// Should we call the then_callback?
		if(this_future_copy.isFinished() || (call_on_cancel && this_future_copy.isCanceled()))
		{
			qDb() << "THEN: CALLING CALLBACK, this_future_copy:" << this_future_copy;

			try
			{
				// Call the callback with the results- or canceled/exception-laden this_future_copy.
				// Could throw, hence we're in a try.
				qDb() << "THENCB: Calling then_callback_copy(this_future_copy).";
				R retval = std::invoke(then_callback_copy, this_future_copy);
				// Didn't throw, report the result.
				returned_future_copy.reportResult(retval);
			}
			catch(QException& e)
			{
				returned_future_copy.reportException(e);
			}
			catch(...)
			{

			}
		}
		else if (call_on_cancel || !(this_future_copy.isFinished() || this_future_copy.isCanceled()))
		{
			// Something went wrong, we got here after .waitForFinished() returned or threw, but
			// the this_future_status isn't Finsihed or Canceled.

			Q_ASSERT(0);
		}

		returned_future_copy.reportFinished();

		}, *this, returned_future);

	return returned_future;
}
#endif

template<typename T>
void ExtFuture<T>::wait()
{
	Q_ASSERT(this->valid());
	this->waitForFinished();
}

template<typename T>
ExtFutureState::State ExtFuture<T>::state() const
{
	// State from QFutureInterfaceBase.
	/// @note The actual state variable is a public member of QFutureInterfaceBasePrivate (in qfutureinterface_p.h),
	///       but an instance of that class is a private member of QFutureInterfaceBase, i.e.:
	///			#ifndef QFUTURE_TEST
	///			private:
	///			#endif
	///				QFutureInterfaceBasePrivate *d;
	/// So we pretty much have to use this queryState() loop here, which is unfortunate since state is
	/// actually a QAtomicInt, so we're not thread-safe here.
	/// This is the queryState() code from qfutureinterface.cpp:
	///
	///     bool QFutureInterfaceBase::queryState(State state) const
	///	    {
	///		    return d->state.load() & state;
	///	    }
	///
	/// Also, QFutureInterface<T>::reportResult() does this:
	///     QMutexLocker locker(mutex());
	///     if (this->queryState(Canceled) || this->queryState(Finished)) {
	///        return;
	///     }

	const std::vector<std::pair<QFutureInterfaceBase::State, const char*>> list = {
		{QFutureInterfaceBase::NoState, "NoState"},
		{QFutureInterfaceBase::Running, "Running"},
		{QFutureInterfaceBase::Started,  "Started"},
		{QFutureInterfaceBase::Finished,  "Finished"},
		{QFutureInterfaceBase::Canceled,  "Canceled"},
		{QFutureInterfaceBase::Paused,   "Paused"},
		{QFutureInterfaceBase::Throttled, "Throttled"}
	};

	ExtFutureState::State current_state = ExtFutureState::state(*this);

	return current_state;
}

#include "ExtFutureImplHelpers.h"

#endif /* UTILS_CONCURRENCY_IMPL_EXTFUTURE_IMPL_HPP_ */
