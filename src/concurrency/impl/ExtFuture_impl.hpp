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
#define EXTFUTURE_IMPL_HAS_BEEN_INCLUDED

#ifndef EXTFUTURE_H_HAS_BEEN_INCLUDED
#include "../ExtFuture.h"
//#include "../ExtAsync_traits.h"
#endif

template <class T>
class ExtFuture;

//#if 0
//#include <config.h>
//#include "../ExtFutureState.h"
//#endif


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
		this->m_progress_unit = other.m_progress_unit;
	}
	return *this;
}

template<typename T>
ExtFuture<T>& ExtFuture<T>::operator=(const ExtFuture::BASE_CLASS& other)
{
	if(this != &other)
	{
		this->BASE_CLASS::operator=(other);
	}
	return *this;
}

template<typename T>
bool ExtFuture<T>::HandlePauseResumeShouldICancel()
{
	if (this->isPaused())
	{
		this->waitForResume();
	}
	if (this->isCanceled())
	{
		// The job should be canceled.
		// The calling runFunctor() should break out of while() loop.
		return true;
	}
	else
	{
		return false;
	}
}

template<typename T>
T ExtFuture<T>::qtget_first()
{
	wait();
	return this->result();
}

template<typename T>
void ExtFuture<T>::wait()
{
	while (!this->isFinished())
	{
		// Pump the event loop.
		QCoreApplication::processEvents(QEventLoop::AllEvents);
		QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
	}
}

#if 0

template<typename T>
ExtFutureState::States ExtFuture<T>::state() const
{
	// States from QFutureInterfaceBase.
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

    const std::vector<std::pair<QFutureInterfaceBase::State, const char*>> list = {
		{QFutureInterfaceBase::NoState, "NoState"},
		{QFutureInterfaceBase::Running, "Running"},
		{QFutureInterfaceBase::Started,  "Started"},
		{QFutureInterfaceBase::Finished,  "Finished"},
		{QFutureInterfaceBase::Canceled,  "Canceled"},
		{QFutureInterfaceBase::Paused,   "Paused"},
		{QFutureInterfaceBase::Throttled, "Throttled"}
	};

    ExtFutureState::States current_state = ExtFutureState::state(*this);

    return current_state;
}


namespace ExtAsync
{
	namespace detail
	{
		template<typename T>
		ExtFuture<typename std::decay_t<T>> make_ready_future(T&& value)
		{
			ExtFuture<T> extfuture;

			extfuture.reportStarted();
			extfuture.reportResult(std::forward<T>(value));
			extfuture.reportFinished();

			return extfuture;
		}

		/// @todo
//		inline ExtFuture<void> make_ready_future()
//		{
//			ExtFuture<void> extfuture;
//
//			extfuture.reportStarted();
//			extfuture.reportFinished();
//
//			return extfuture;
//		}

		template <typename T, typename R = typename std::decay_t<T>>
		ExtFuture<R> make_exceptional_future(const QException& exception)
		{
			ExtFuture<R> extfuture;

			extfuture.reportStarted();
			extfuture.reportException(exception);
			extfuture.reportFinished();

		    return extfuture;
		}

	}
}
#endif

#endif /* UTILS_CONCURRENCY_IMPL_EXTFUTURE_IMPL_HPP_ */
