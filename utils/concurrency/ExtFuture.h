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
namespace ExtAsync { namespace detail {} }

template <class T>
class ExtFuture;

struct ExtAsyncProgress
{
	int min;
	int val;
	int max;
	QString text;
};

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

// Stuff that ExtFuture.h needs to have declared/defined prior to the ExtFuture<> declaration.
#include "impl/ExtFuture_fwddecl_p.h"


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

	/// Member alias ala boost::future<T>.
	using value_type = T;

	using ContinuationType = std::function<QString(QString)>;

	/// Type 1 tap() callback.
	/// Takes a value of type T, returns void.
	using TapCallbackType1 = std::function<void(QString)>;

	using TapCallbackTypeProgress = std::function<void(ExtAsyncProgress)>;


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

	/**
	 * Unwrapping constructor, ala std::experimental::future::future, boost::future.
	 */
//	inline explicit ExtFuture(ExtFuture<ExtFuture<T>>&&	other);


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

		qWr() << "m_continuation_function:" << (bool)m_continuation_function;
		qWr() << "m_extfuture_watcher:" << m_extfuture_watcher;
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
	std::enable_if_t<isExtFuture_v<F>, ExtFuture<typename isExtFuture<T>::inner_t>>
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
	 *
	 * @tparam F = Continuation function type.
	 * @tparam R = Return value of continuation F(ExtFuture<T>).
	 *
	 * @return A new future for containing the return value of @a continuation_function.
	 */
	template <typename F, typename R = std::result_of_t<std::decay_t<F>(ExtFuture<T>)> >
	ExtFuture<R> then(QObject* context, F&& func)
	{
//		m_continuation_function = std::make_shared<ContinuationType>(std::move(func));
		return ThenHelper(context, func, *this);//*m_continuation_function);
	}

	/**
	 * Overload for callback returning non-ExtFuture<>.
	 */
//	template <typename F, typename R = function_return_type_t<F>>
//	std::enable_if_t<!isExtFuture_v<R>, ExtFuture<R>>
//	then(F&& func)
//	{
//		return then(QApplication::instance(), func);
//	}

	/**
	 * then() overload taking a callback of type R(*func)(T).
	 *
	 * @tparam R a non-ExtFuture<> type.
	 * @tparam T a non-ExtFuture<> type.
	 *
	 * @param then_callback  Callback of type R(*)(T).
	 *
	 * @returns ExtFuture<R>
	 */
	template <typename R = T>
	std::enable_if_t<!isExtFuture_v<R>
		&& !isExtFuture_v<T>
		&& !std::is_same_v<R, void>
		&& !std::is_same_v<T, void>, ExtFuture<R>>
	then( R(*then_callback)(T) )
	{
//		std::function<R(T)> the_then_callback = then_callback;
		return then(QApplication::instance(), then_callback);
	}

	/// std::/boost::-ish then()'s.
	/**
	 * then_callback(ExtFuture<T>) -> R
	 * Returns an ExtFuture<R>.
	 *
	 * @param then_callback
	 * @return
	 */
	template <typename F>
	auto then( F&& then_callback ) -> ExtFuture<decltype(then_callback(*this))>
	{
//		std::function<R(T)> the_then_callback = then_callback;
		return then(QApplication::instance(), then_callback);
	}
//	template <typename F>
//	ExtFuture<typename std::result_of<F(ExtFuture<T>)>::type>
//	then(F&& then_callback) //-> ExtFuture<decltype(then_callback(*this))>
//	{
//		return then(QApplication::instance(), std::forward<F>(then_callback));
//	}
	/**
	 * U = Return type of continuation.
	 * @param context
	 * @param then_callback
	 * @return
	 */
//	template <typename F, typename U = std::result_of_t<std::decay_t<F>(ExtFuture<T>)> >
//	ExtFuture<U>
//	then(QObject* context, F&& then_callback)
//	{
//		Q_ASSERT(0);
//		//return then(QApplication::instance(), then_callback);
//	}


	/**
	 * Overload for an empty .then().
	 * @return
	 */
//	ExtFuture<Unit> then()
//	{
////		return then([] () {});
//	}

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

	ExtFuture<T>& tap(QObject* context, TapCallbackTypeProgress prog_tap_callback)
	{
		m_tap_progress_function = std::make_shared<TapCallbackTypeProgress>(prog_tap_callback);
		return TapProgressHelper(context, *m_tap_progress_function);
	}

	ExtFuture<T>& tap(TapCallbackTypeProgress prog_tap_callback)
	{
		return tap(QApplication::instance(), prog_tap_callback);
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
		QObject::connect(watcher, &QFutureWatcherBase::destroyed, [](){ qWr() << "TAP ExtFutureWatcher DESTROYED";});
		watcher->setFuture(this->future());
		qDb() << "EXIT";
		return *this;
	}

	template <typename Function>
	ExtFuture<T>& TapProgressHelper(QObject *guard_qobject, Function f)
	{
		qDb() << "ENTER";
		auto watcher = new ExtFutureWatcher<T>();
		QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, &QObject::deleteLater);
		watcher->onProgressChange([f, watcher](int min, int val, int max, QString text){
			f({min, val, max, text});
			;});
		QObject::connect(watcher, &QFutureWatcherBase::destroyed, [](){ qWr() << "TAPPROGRESS ExtFutureWatcher DESTROYED";});
		watcher->setFuture(*this);
		qDb() << "EXIT";
		return *this;
	}

	template <typename R = T, typename F, typename... Args>
	ExtFuture<R> ThenHelper(QObject* context, F&& then_callback, Args&&... args)
	{
//		qDb() << "ENTER";
		auto watcher = new QFutureWatcher<T>();
M_WARNING("TODO: LEAKS THIS");
		auto retval = new ExtFuture<R>();
		qDb() << "NEW EXTFUTURE:" << *retval;
		QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, [then_callback, retval, args..., watcher](){
			// Call the then() callback function.
			qDb() << "THEN WRAPPER CALLED";
			// f() takes void, val, or ExtFuture<T>.
			// f() returns void, a type R, or an ExtFuture<R>
			retval->reportResult(then_callback(args...));
			retval->reportFinished();
			qDb() << "RETVAL STATUS:" << *retval;
			watcher->deleteLater();
		});
		QObject::connect(watcher, &QFutureWatcherBase::destroyed, [](){ qWr() << "ThenHelper ExtFutureWatcher DESTROYED";});
		watcher->setFuture(this->future());
//		qDb() << "EXIT";
		return *retval;
	}

//	template <typename FunctionType, typename... Args, typename Ftraits = function_traits<FunctionType>>
//	auto ThenHelper(QObject *guard_qobject, FunctionType f, Args... args) -> ExtFuture<typename Ftraits::return_type_t>
//	{
//		// The callback can only take 0 or 1 arg.
//		static_assert(sizeof...(Args) <= 1, "Too many args");
//
//		qDb() << "ENTER";
//		auto watcher = new QFutureWatcher<T>();
//		QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, [f, watcher](){
//			// Call the then() callback function.
//			qDb() << "THEN WRAPPER CALLED";
//			// f() takes void, val, or ExtFuture<T>.
//			// f() returns void, a type R, or an ExtFuture<R>
//			f();
//			watcher->deleteLater();
//		});
//		QObject::connect(watcher, &QFutureWatcherBase::destroyed, [](){ qWr() << "ThenHelper ExtFutureWatcher DESTROYED";});
//		watcher->setFuture(this->future());
//		qDb() << "EXIT";
//		return *this;
//	}

	/**
	 * ThenHelper which takes a callback which returns an ExtFuture<>.
	 */
//	template <typename F, typename R, typename... Args>
//	std::enable_if_t<R::returns_future::value, typename R::return_type>
//	ThenHelper(F&& func, arg_result<F, Args...>);

	ExtFutureWatcher<T>* m_extfuture_watcher = nullptr;

	std::shared_ptr<ContinuationType> m_continuation_function;

	std::shared_ptr<TapCallbackType1> m_tap_function;

	std::shared_ptr<TapCallbackTypeProgress> m_tap_progress_function;


};


//
// START IMPLEMENTATION
//

//M_WARNING("INCLUDING ExtAsync.h");
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



// Include the implementation.
#include <utils/concurrency/impl/ExtFuture_p.hpp>

/**
 * Create and return a finished future of type ExtFuture<T>.
 *
 * Intended to be a mostly-work-alike to std::experimental::make_ready_future.
 * @see http://en.cppreference.com/w/cpp/experimental/make_ready_future
 *
 * @todo Specialize for void, or use Unit.
 * @todo return type decay rules when decay<T> is ref wrapper.
 *
 * @param value
 * @return
 */
#if 0
template<typename T, typename X, typename U = std::decay_t<T>, typename V = std::conditional<std::is_same_v<U, std::reference_wrapper<X>>,X&,U>>
ExtFuture</*typename std::decay_t<T>*/V> make_ready_future(T&& value)
{
	return ExtAsync::detail::make_ready_future<typename std::decay<T>::type>(value);
}
#else
template <typename T>
struct deduced_type_impl
{
	using type = T;
};
template <typename T>
struct deduced_type_impl<std::reference_wrapper<T>>
{
	using type = T&;
};
template <typename T>
struct deduced_type
{
	using type = typename deduced_type_impl<std::decay_t<T>>::type;
};
template <typename T>
using deduced_type_t = typename deduced_type<T>::type;

/**
 * Creates a completed future containing the value @a value.
 *
 * @param value
 * @return
 */
template <int = 0, int..., class T>
ExtFuture<deduced_type_t<T>> make_ready_future(T&& value)
{
	return /*ExtFuture<deduced_type_t<T>>();*/ ExtAsync::detail::make_ready_future(std::forward<T>(value));
}

/// overload for ExtFutue<void>.
/// @todo
//inline ExtFuture<void> make_ready_future()
//{
//	return ExtAsync::detail::make_ready_future();
//}

#endif

#endif /* UTILS_CONCURRENCY_EXTFUTURE_H_ */

