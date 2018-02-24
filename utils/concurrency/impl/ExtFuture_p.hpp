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

#ifndef UTILS_CONCURRENCY_IMPL_EXTFUTURE_P_HPP_
#define UTILS_CONCURRENCY_IMPL_EXTFUTURE_P_HPP_


/**
 * For unwrapping an ExtFuture<ExtFuture<T>> to a ExtFuture<T>.
 * Implementation based on Facebook's "folly" library's Future (Apache 2.0)
 */
template <typename T>
template <typename F>
std::enable_if_t<isExtFuture_v<F>, ExtFuture<typename isExtFuture<T>::inner_t>>
ExtFuture<T>::unwrap()
{
	using InternalExtFutureType = ExtFuture<typename isExtFuture<T>::inner_t>;
	InternalExtFutureType internal_extfuture;
	return this->then([=](InternalExtFutureType internal_extfuture) -> InternalExtFutureType {
		return internal_extfuture;
		});
}

template<typename T>
T ExtFuture<T>::get()
{
	return this->future().result();
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

template<typename T>
QString ExtFuture<T>::state() const
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

	std::vector<std::pair<QFutureInterfaceBase::State, const char*>> list = {
		{QFutureInterfaceBase::NoState, "NoState"},
		{QFutureInterfaceBase::Running, "Running"},
		{QFutureInterfaceBase::Started,  "Started"},
		{QFutureInterfaceBase::Finished,  "Finished"},
		{QFutureInterfaceBase::Canceled,  "Canceled"},
		{QFutureInterfaceBase::Paused,   "Paused"},
		{QFutureInterfaceBase::Throttled, "Throttled"}
	};

	QString retval = "";
	for(auto i : list)
	{
		if(QFutureInterfaceBase::queryState(i.first))
		{
			if(retval.size() != 0)
			{
				// Add a separator.
				retval += " | ";
			}
			retval += toqstr(i.second);
		}
	}

	if(retval.size() == 0)
	{
		return QString("UNKNOWN");
	}
	else
	{
		return retval;
	}
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

	}
}

/**
 * ThenHelper which takes a callback which returns an ExtFuture<>.
 */
//template <typename T>
//template <typename F, typename R, typename... Args>
//std::enable_if_t<R::returns_future::value, typename R::return_type>
//ExtFuture<T>::ThenHelper(F&& func, arg_result<F, Args...>)
//{
//	static_assert(sizeof...(Args) <= 1, "Too many args");
//
//	using B = typename isExtFuture<R>::inner_t;
//
//	ExtFuture<B> promise;
//	auto future = promise.future();
//
//	M_WARNING("TODO");
//
//
//	return future;
//
//}

#if 0
/**
 * This is the function which actually is called by QtConcurrent::run() for the continuation.
 */
template<class T>
static QString ThenHelper(ExtFuture<T>* predecessor_future)
{
	qDb() << "THEN CALLED, WAITING";
	predecessor_future->wait();
	qDb() << "THEN CALLED, WAIT OVER, CALLING CALLBACK";
	Q_CHECK_PTR(predecessor_future);
	Q_CHECK_PTR(predecessor_future->m_continuation_function);
	(*(predecessor_future->m_continuation_function))();
	return QString("THEN DONE");
}
#endif
#endif /* UTILS_CONCURRENCY_IMPL_EXTFUTURE_P_HPP_ */
