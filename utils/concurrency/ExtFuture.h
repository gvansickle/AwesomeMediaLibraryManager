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
 * @file
 * An extended QFutureInterface<T> class.
 */

#ifndef UTILS_CONCURRENCY_EXTFUTURE_H_
#define UTILS_CONCURRENCY_EXTFUTURE_H_


#include <QFutureInterface>

#include <utils/StringHelpers.h>
#include <utils/DebugHelpers.h>

/**
 * An extended QFutureInterface<T> class.
 * Actually more like a combined promise and future.
 *
 * Promise (producer/writer) functionality:
 *   Most functionality provided by QFutureInterfaceBase, including:
 * - reportResult()
 * - reportFinished()
 * - setProgressValue() and other progress reporting.
 * - pause()/resume()
 * - cancel()
 * - results()
 * - future()
 *
 * Future (consumer/reader) functionality:
 * - get()
 * - then()
 * - wait()
 * - await()
 * - tap()
 */
template <typename T>
class ExtFuture : public QFutureInterface<T>
{
	using BASE_CLASS = QFutureInterface<T>;

public:
	/**
	 * Default constructor.
	 *
	 * @note Regarding the initial state: From a comment in QtCreator's runextensions.h::AsyncJob constructor:
	 * "we need to report it as started even though it isn't yet, because someone might
	 * call waitForFinished on the future, which does _not_ block if the future is not started"
	 * That code also does this:
	 *
	 * 		m_future_interface.setRunnable(this);
	 *
	 * Not sure if we need to do that here or not.
	 *
	 * This is the code we're fighting:
	 *
	 * void QFutureInterfaceBase::waitForFinished()
		{
			QMutexLocker lock(&d->m_mutex);
			const bool alreadyFinished = !isRunning();
			lock.unlock();

			if (!alreadyFinished) {
				d->pool()->d_func()->stealAndRunRunnable(d->runnable);

				lock.relock();

				while (isRunning())
					d->waitCondition.wait(&d->m_mutex);
			}

			d->m_exceptionStore.throwPossibleException();
		}
	 *
	 *
	 * @param initialState
	 */
	ExtFuture(QFutureInterfaceBase::State initialState = QFutureInterfaceBase::State(QFutureInterfaceBase::State::Started | QFutureInterfaceBase::State::Running))
		: QFutureInterface<T>(initialState)
	{
		qDb() << "Passed state:" << initialState << "ExtFuture state:" << state();
	}

	ExtFuture(const QFutureInterface<T> &other)
		: QFutureInterface<T>(other)
	{
		qDb() << "future state:" << state();
	}

	/**
	 * Virtual destructor.
	 * @note QFutureInterface<> is derived from QFutureInterfaceBase, which has a virtual destructor.
	 * QFutureInterface<>'s destructor isn't marked either virtual or override.  By the
	 * rules of C++, QFutureInterface<>'s destructor is in fact virtual ("once virtual always virtual"),
	 * so we're good.  Marking this override to avoid further confusion.
	 */
	~ExtFuture() override
	{
		qDebug() << "DESTRUCTOR";
	}

	/**
	 *
	 * @return
	 */
	T get();

	/**
	 * Attaches a continuation to this ExtFuture.
	 * @return A new future for containing the return value of @a continuation_function.
	 */
	ExtFuture<void> then(std::function<ExtFuture<void>(ExtFuture<T>&)> continuation_function)
	{
		m_continuation_function = std::move(continuation_function);
		return m_continuation_function(*this);
	}

	void wait();

	/// @todo
	void await();

	/**
	 * Get this' current state as a string.
	 *
	 * @return QString describing the current state of the ExtFuture.
	 */
	QString state() const;

	/**
	 * queryState
	 * @return The value of the otherwise-protected state member variable.
	 */
//	QFutureInterfaceBase::State queryState() const;

	/**
	 * Get a string describing this ExtFuture<>, suitable for debug output.
	 * @return
	 */
	QString debug_state() const;

	/// @name Operators
	/// @{

	/**
	 * Returns true if @a other is a copy of this ExtFuture, else returns false.
	 * @param other
	 * @return
	 */
//	bool operator==(const ExtFuture<T>& other) const;

//	bool operator==(const QFuture<T>& other) const { return this->future() == other; }

	/// @} // END Operators

protected:

	std::function<ExtFuture<void>()> m_continuation_function {nullptr};

};

//Q_DECLARE_METATYPE(ExtFuture);
Q_DECLARE_METATYPE_TEMPLATE_1ARG(ExtFuture)

template<typename T>
T ExtFuture<T>::get()
{

}

template<typename T>
void ExtFuture<T>::wait()
{
	qDb() << "WAIT START, future state:" << state();
	this->waitForFinished();
	this->results();
	this->cancel();
	qDb() << "WAIT COMPLETE, future state:" << state();
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
//			qDebug() << "CHECKING" << i.first << ": TRUE";

			if(retval.size() != 0)
			{
				// Add a separator.
				retval += " | ";
			}
			retval += toqstr(i.second);
		}
		else
		{
//			qDebug() << "CHECKING" << i.first << ": FALSE";
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

//template<typename T>
//bool ExtFuture<T>::operator==(const ExtFuture<T>& other) const
//{
//	return QFutureInterface<T>::operator==(other);
//}

#if 0
template<typename T>
QFutureInterfaceBase::State ExtFuture<T>::queryState() const
{
	/// @todo This can't actually work.

	/// We need this butchery to get the value of this->d->p->state.  Qt 5.10 provides no interface
	/// or other known way to get the future's state contents atomically, only the queryState() function,
	/// which returns a bool which is the result of &'ing the state with the input param.
	/// This is the queryState() code from qfutureinterface.cpp:
	///
	///     bool QFutureInterfaceBase::queryState(State state) const
	///	    {
	///		    return d->state.load() & state;
	///	    }
	///
	/// The actual state variable is a public member of QFutureInterfaceBasePrivate (in qfutureinterface_p.h),
	/// but the instance of that class in QFutureInterfaceBase is private:
	///
	///			#ifndef QFUTURE_TEST
	///			private:
	///			#endif
	///				QFutureInterfaceBasePrivate *d;
	///
	/// Trying to #define QFUTURE_TEST in various ways doesn't work, I did try.

#if 0 // DOESN'T WORK
	QFutureInterfaceBase::State retval;
	QFutureInterfaceBase::State mask = (QFutureInterfaceBase::State)0xFF;
	// Do the unthinkable: Cast a bool to an int.
	int state_bits = *((int*)(&QFutureInterfaceBase::queryState(mask)));
	retval = (QFutureInterfaceBase::State)(state_bits);
#endif
	Q_ASSERT(0);
	return QFutureInterfaceBase::State::NoState;
}
#endif

template <typename T>
QDebug operator<<(QDebug dbg, const ExtFuture<T> &extfuture)
{
	QDebugStateSaver saver(dbg);

	dbg << "ExtFuture<T>(" /*<< extfuture.future()*/ << "STATE:" << extfuture.state() << ")";

	return dbg;
}


#endif /* UTILS_CONCURRENCY_EXTFUTURE_H_ */
