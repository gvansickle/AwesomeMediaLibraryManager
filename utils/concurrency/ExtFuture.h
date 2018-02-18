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
#include <functional>
#include "function_traits.hpp"

#include "ExtFutureWatcher.h"

#if 0
#include "cpp14_concepts.hpp"

// Define some concepts.
template <class T, class FutureRetureType>
constexpr bool TapCallback = require<
	function_traits<T>::return_type_is_v<ExtFuture<FutureRetureType>>
>;
#endif

// Forward declare the ExtAsync namespace
namespace ExtAsync {}

template <class T>
class ExtFuture;

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


template <typename T>
struct isExtFuture : std::false_type
{
	/// @todo Not sure this is correct.
	using inner = void;
};

/// Returns the T from an ExtFuture<T> as isExtFuture<T>::inner_t.
template <typename T>
struct isExtFuture<ExtFuture<T>> : std::true_type
{
	using inner_t = T;
};

template <typename T>
static constexpr bool isExtFuture_v = isExtFuture<T>::value;

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

	using future_value_type = T;

	using ContinuationType = std::function<QString(QString)>;

	/// Type 1 tap() callback.
	/// Takes a value of type T, returns void.
	using TapCallbackType1 = std::function<void(QString)>;

	using OnResultCallbackType1 = std::function<void(QString)>;

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

	ExtFuture(const ExtFuture<T>& other) = default;

#if TEMP
	/**
	 * Implicit unwrapping constructor, ala std::experimental::future::future.
	 * @param rhs
	 */
	ExtFuture(ExtFuture<ExtFuture<R>>&&	rhs) noexcept;
#endif

//	ExtFuture(const ExtFuture<T>& other) : QFutureInterface<T>(other),
//			m_continuation_function(other.m_continuation_function),
//			m_tap_function(other.m_tap_function)
//	{
//		qIn() << "Copy Constructor";
//
////		if(other.m_continuation_function != nullptr)
////		{
////			Q_ASSERT(0);
////		}
//	}

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

		if(m_extfuture_watcher)
		{
			qWr() << "m_extfuture_watcher != NULL";
		}
	}

	/// @name Copy and move operators.
	/// @{

	ExtFuture<T>& operator=(const ExtFuture<T>& other) = default;
	ExtFuture<T>& operator=(ExtFuture<T>&& other) = default;

	/// @}

	/**
	 * For unwrapping an ExtFuture<ExtFuture<T>> to a ExtFuture<T>.
	 */
	template <typename F = T>
	std::enable_if_t<isExtFuture<F>::value, ExtFuture<typename isExtFuture<T>::inner>>
	unwrap();

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
	template <typename F, typename R = function_return_type_t<F>>
	ExtFuture<R> then(QObject* context, F&& func)
	{
		m_continuation_function = std::make_shared<ContinuationType>(std::move(func));
		return ThenHelper(context, *m_continuation_function);
	}

	template <typename F, typename R = function_return_type_t<F>>
	ExtFuture<R> then(F&& func)
	{
		return then(QApplication::instance(), func);
	}

	/**
	 * Attaches a "tap" callback to this ExtFuture.
	 * The callback passed to tap() is invoked with an reference to an instance of the predecessor's ExtFuture<> (i.e. this).
	 *
	 * @return  A reference to the predecessor ExtFuture<T>.
	 */
	ExtFuture<QString>& tap(QObject* context, TapCallbackType1 tap_callback)
	{
		m_tap_function = std::make_shared<TapCallbackType1>(tap_callback);
		return TapHelper(context, *m_tap_function);
	}

	ExtFuture<T>& tap(TapCallbackType1 tap_callback)
	{
		return tap(QApplication::instance(), tap_callback);
	}

	/**
	 * Block the current thread on the finishing of this ExtFuture, but keep the thread's
	 * event loop running.
	 */
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

	void EnsureFWInstantiated()
	{
		if(!m_extfuture_watcher)
		{
			m_extfuture_watcher = new ExtFutureWatcher<T>();
		}
	}

	template <typename Function>
	ExtFuture<T>& TapHelper(QObject *guard_qobject, Function f)
	{
		qDb() << "ENTER";
		auto watcher = new QFutureWatcher<T>();
		QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, &QObject::deleteLater);
		QObject::connect(watcher, &QFutureWatcherBase::resultReadyAt, guard_qobject, [f, watcher](int index) {
			qDb() << "TAP WRAPPER CALLED";
			f(watcher->future().resultAt(index));
		});
		QObject::connect(watcher, &QFutureWatcherBase::destroyed, [](){ qWr() << "ExtFutureWatcher DESTROYED";});
		watcher->setFuture(this->future());
		qDb() << "EXIT";
		return *this;
	}

	template <typename Function>
	ExtFuture<T> ThenHelper(QObject *guard_qobject, Function f)
	{
		qDb() << "ENTER";
		auto watcher = new QFutureWatcher<T>();
		QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, [f, watcher](){
			// Call the then() callback function.
			qDb() << "THEN WRAPPER CALLED";
			f("dummy");
			watcher->deleteLater();
		});
		QObject::connect(watcher, &QFutureWatcherBase::destroyed, [](){ qWr() << "ThenHelper ExtFutureWatcher DESTROYED";});
		watcher->setFuture(this->future());
		qDb() << "EXIT";
		return *this;
	}

	ExtFutureWatcher<T>* m_extfuture_watcher = nullptr;

	std::shared_ptr<ContinuationType> m_continuation_function;

	std::shared_ptr<TapCallbackType1> m_tap_function;


};

#if 0
/**
 * Create and return a finished future of type ExtFuture<T>.
 * @param value
 * @return
 */
template<typename T>
ExtFuture<typename std::decay<T>::type> make_ready_future(T&& value)
{
	return detail::make_ready_future<typename std::decay<T>::type>(value);
}

/**
 * void specialization of the above.
 * @return
 */
inline future<void> make_ready_future()
{
  return detail::made_ready_future<void>();
}
#endif

//
// START IMPLEMENTATION
//

M_WARNING("INCLUDING ExtAsync.h");
#include "ExtAsync.h"


template <typename T>
QDebug operator<<(QDebug dbg, const ExtFuture<T> &extfuture)
{
	QDebugStateSaver saver(dbg);

	dbg << "ExtFuture<T>(" << extfuture.debug_string() << ")";

	return dbg;
}





//Q_DECLARE_METATYPE(ExtFuture);
//Q_DECLARE_METATYPE_TEMPLATE_1ARG(ExtFuture)

template<typename T>
T ExtFuture<T>::get()
{
	return this->future().result();
}

#if 0
template<typename T>
ExtFuture<QString> ExtFuture<T>::then(ContinuationType continuation_function)
{
	m_continuation_function = std::make_shared<ContinuationType>(continuation_function);
	ExtFuture<QString> retval = ExtAsync::run_then(*m_continuation_function, *this).unwrap();
	qDb() << "THEN Entered, returning:" << &retval << retval;
	return retval;
}
#endif

template<typename T>
void ExtFuture<T>::wait()
{
    while (!this->isFinished())
    {
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

// Include the implementation.
#include "ExtFuture_p.hpp"

#endif /* UTILS_CONCURRENCY_EXTFUTURE_H_ */

