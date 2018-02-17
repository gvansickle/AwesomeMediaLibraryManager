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

#include <memory>
#include <type_traits>
#include "function_traits.hpp"

// Forward declare the ExtAsync namespace
namespace ExtAsync {}

/**
 * A decay_copy for creating a copy of the specified function @a func.
 * @param func
 * @return
 */
template <typename T>
std::decay_t<T> decay_copy(T&& func)
{
	return std::forward<T>(func);
}

template <typename T>
class UniqueIDMixin
{
	static std::atomic_uintmax_t m_next_id_num;

	uintmax_t m_id_num;

public:
	UniqueIDMixin()
	{
		m_id_num = m_next_id_num;
		m_next_id_num++;
	}

	QString id() const { return QString("%1").arg(m_id_num); }
};
//
template <typename T>
std::atomic_uintmax_t UniqueIDMixin<T>::m_next_id_num;

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
class ExtFuture : public QFutureInterface<T>, public UniqueIDMixin<ExtFuture<T>>
{
	using BASE_CLASS = QFutureInterface<T>;

public:

	using ContinuationType = std::function<void(void)>;

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

	ExtFuture(const ExtFuture<T>& other) : QFutureInterface<T>(other), m_continuation_function(other.m_continuation_function)
	{
		qIn() << "Copy Constructor: other.m_continuation_function copied";

//		if(other.m_continuation_function != nullptr)
//		{
//			Q_ASSERT(0);
//		}
	}

	ExtFuture(const QFutureInterface<T> &other) : QFutureInterface<T>(other)
	{
		qDb() << "future state:" << *this;
	}

	ExtFuture(QFuture<T> other_future) : QFutureInterface<T>(other_future.d)
	{
		qDb() << "future state:" << *this;
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
		qDb() << "DESTRUCTOR";

		if(m_continuation_function)
		{
			qWr() << "m_continuation_function == NULLPTR";
		}
		else
		{
			qWr() << "m_continuation_function != NULLPTR";
		}
	}

	/// @name Copy and move operators.
	/// @{

	ExtFuture<T>& operator=(const ExtFuture<T>& other) = default;
	ExtFuture<T>& operator=(ExtFuture<T>&& other) = default;

	/// @}

	/**
	 *
	 * @return
	 */
	T get();

	/**
	 * Attaches a continuation to this ExtFuture.
	 * @note Like std::experimental::future::then(), the continuation function will be run on
	 *       an unspecified thread.
	 * @return A new future for containing the return value of @a continuation_function.
	 */
#if 0
//	template<class ContinuationFunctionType> //, class ReturnType = std::result_of_t<ContinuationFunctionType(ExtFuture<T>&)>>
	ExtFuture<QString> then(ContinuationType continuation_function)
	{
		m_continuation_function = std::make_shared<ContinuationType>(continuation_function);
		ExtFuture<QString> retval = ExtAsync(*m_continuation_function, *this);
		return retval;
	}
#endif

	template <typename R = typename function_traits<ContinuationType>::return_type_t>
	ExtFuture<R> then(ContinuationType continuation_function);

	/**
	 * Attaches a "tap" callback to this ExtFuture.
	 * The callback passed to tap() is invoked with an instance of the new result value of the predecessor's ExtFuture
	 * whenever there is a new result available.
	 *
	 * @note Whoah, what's with that wild preproc macro?!?  K&R meets C++20!!!
	 *
	 * @return  A reference to the predecessor ExtFuture<T>.
	 */
#define M_EXTFUTURE_TAP_DECL(mem_func_name) \
	template <typename TapCallbackType> \
	ExtFuture<T>& mem_func_name(TapCallbackType tap_callback)

	M_EXTFUTURE_TAP_DECL(tap);

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
	QString debug_string() const
	{
		QString retval = "ID: " + this->id();
		retval += ", STATE: (" + state() + ")";
		if(m_continuation_function)
		{
			retval += ", Continuation: nullptr";
		}
		else
		{
			retval += ", Continuation: valid";
		}
		return retval;
	}

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

	std::shared_ptr<ContinuationType> m_continuation_function;

/// @todo
//	std::shared_ptr<typename TapCallbackType> m_tap_function;

};

//
// START IMPLEMENTATION
//

M_WARNING("INCLUDING ExtAsync.h");
#include "ExtAsync.h"

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

//Q_DECLARE_METATYPE(ExtFuture);
//Q_DECLARE_METATYPE_TEMPLATE_1ARG(ExtFuture)

template<typename T>
T ExtFuture<T>::get()
{
	return this->future().result();
}

template<typename T>
template<typename R>
ExtFuture<R> ExtFuture<T>::then(ContinuationType continuation_function)
{
	m_continuation_function = std::make_shared<ContinuationType>(continuation_function);
	ExtFuture<R> retval = ExtAsync::run(*m_continuation_function, *this);
	return retval;
}

template <typename T>
M_EXTFUTURE_TAP_DECL(ExtFuture<T>::tap)
{
M_WARNING("TODO")
	qDb() << "TAP() ENTERED";
	//m_tap_function<TapCallbackType> = tap_callback;
}


template<typename T>
void ExtFuture<T>::wait()
{
	qDb() << "WAIT START, future state:" << *this;
	this->waitForFinished();
	qDb() << "WAIT COMPLETE, future state:" << *this;
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

	dbg << "ExtFuture<T>(" << extfuture.debug_string() << ")";

	return dbg;
}


#endif /* UTILS_CONCURRENCY_EXTFUTURE_H_ */

