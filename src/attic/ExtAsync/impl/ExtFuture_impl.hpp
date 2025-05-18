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

// Included by ExtFuture.h.

template <class T>
class ExtFuture;

template<typename T>
inline ExtFuture<T>& ExtFuture<T>::operator=(const ExtFuture<T>& other)
{
	if(this != &other)
	{
		this->BASE_CLASS::operator=(other);
		this->m_extfuture_id_no.store(other.m_extfuture_id_no);
		this->m_name = m_name;
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
		this->m_name = "[unknown]";
	}
	return *this;
}

template<typename T>
T ExtFuture<T>::get_first()
{
	wait();
	return this->result();
}

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
//		{QFutureInterfaceBase::Paused,   "Paused"},
		{QFutureInterfaceBase::Throttled, "Throttled"}
	};

	ExtFutureState::State current_state = ExtFutureState::state(*this);

	return current_state;
}

#endif /* UTILS_CONCURRENCY_IMPL_EXTFUTURE_IMPL_HPP_ */
