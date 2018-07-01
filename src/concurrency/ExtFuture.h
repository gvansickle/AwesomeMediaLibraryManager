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

// Std C++
#include <memory>
#include <type_traits>
#include <functional>

// Qt5
#include <QFutureInterface>


#include <utils/StringHelpers.h>
#include <utils/DebugHelpers.h>
#include "function_traits.hpp"
#include "cpp14_concepts.hpp"
#include <utils/UniqueIDMixin.h>
#include "ExtFutureWatcher.h"
#include "ExtFutureState.h"

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
 *
 * Note that QFuture<T> is a ref-counted object which can be safely passed by value; intent is that ExtFuture<T>
 * has the same properties.  In this, they're both more similar to std::experimental::shared_future than ::future,
 * the latter of which has a deleted copy constructor.
 *
 * QFuture<T> itself only implements the following:
 * - Default constructor, which initializes its "private" (actually currently public) "mutable QFutureInterface<T> d;" underlying
 *   QFuterInterface<> object like so:
 *     @code
 *     	... : d(QFutureInterface<T>::canceledResult())
 *     @endcode
 * - An expicit QFuture(QFutureInterface<T> *p) constructor commented as "internal".
 *
 * QFuture<T> and QFutureInterfaceBase don't inherit from anything.  QFutureInterface<T> only inherits
 * from QFutureInterfaceBase.  Specifically, nothing inherits from QObject, so we're pretty free to use templating
 * and multiple inheritance.
 */
template <typename T>
class ExtFuture : public QFutureInterface<T>, public UniqueIDMixin<ExtFuture<T>>
{
	using BASE_CLASS = QFutureInterface<T>;

	static_assert(!std::is_void<T>::value, "ExtFuture<void> not supported, use ExtFuture<Unit> instead.");

	/// Like QFuture<T>, T must have a default constructor and a copy constructor.
	static_assert(std::is_default_constructible<T>::value, "T must be default constructible.");
	static_assert(std::is_copy_constructible<T>::value, "T must be copy constructible.");

public:

	/// Member alias for the contained type, ala boost::future<T>, Facebook's Folly Futures.
	using value_type = T;
	using is_ExtFuture = std::true_type;
	static constexpr bool is_ExtFuture_v = is_ExtFuture::value;

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

            if (!alreadyFinished)
            {
                /// GRVS: Not finsihed, so start running it?
				d->pool()->d_func()->stealAndRunRunnable(d->runnable);

				lock.relock();

				while (isRunning())
					d->waitCondition.wait(&d->m_mutex);
			}

			d->m_exceptionStore.throwPossibleException();
		}
	 *
	 *
	 * @param initialState  Defaults to State(Started | Running)
	 */
    ExtFuture(QFutureInterfaceBase::State initialState = QFutureInterfaceBase::State(QFutureInterfaceBase::State::Started /*| QFutureInterfaceBase::State::Running*/))
		: QFutureInterface<T>(initialState)
	{
		qDb() << "Passed state:" << initialState << "ExtFuture state:" << state();
	}

	ExtFuture(const ExtFuture<T>& other) = default;

	/**
	 * Unwrapping constructor, ala std::experimental::future::future, boost::future.
	 *
	 * @todo Not sure if we need this or not.  .then() has an unwrapping overload.
	 */
//	inline explicit ExtFuture(ExtFuture<ExtFuture<T>>&&	other);


	// Do we need a non-default copy constructor?

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
//		qDb() << "DESTRUCTOR";
		/// @note Since we're based on QFutureInterface<T> <- QFutureInterfaceBase, it handles the underlying
		/// refcounting for us.  So we may be getting destroyed after we've been copied, which is ok.

		/// @todo Find a way to assert if we're still running/not finished and haven't been copied.
		/// This would indicate a dangling future.
	}

	/// @name Copy and move operators.
	/// @{

	ExtFuture<T>& operator=(const ExtFuture<T>& other) = default;
	ExtFuture<T>& operator=(ExtFuture<T>&& other) = default;

	/// @}

	/**
	 * For explicitly unwrapping an ExtFuture<ExtFuture<T>> to a ExtFuture<T>.
	 * Inspired by Facebook's Folly Futures.
	 */
#if 0 /// @todo
	template <typename F>
	std::enable_if_t<isExtFuture_v<F>, ExtFuture<typename isExtFuture<T>::inner_t>>
	unwrap();
#endif

	/**
	 * Waits until the ExtFuture is finished, and returns the result.
	 * Essentially the same semantics as std::future::get().
	 *
	 * @note Calls .wait() then returns this->future().result().  This keeps Qt's event loops running.
	 *       Not entirely sure if that's what we should be doing or not, std::future::get() doesn't
	 *       work like that, but this is Qt, so... when in Rome....
	 *
	 * @return The result value of this ExtFuture.
	 */
	T get();

	/// @name .then() overloads.
	/// Various C++2x/"C++ Extensions for Concurrency" TS (ISO/IEC TS 19571:2016) std::experimental::future-like
	/// .then() overloads for Qt5.
	/// @{

	/**
	 * std::experimental::future-like .then() which attaches a continuation function @a then_callback to @a this,
	 * where then_callback's signature is:
	 * 	@code
	 * 		then_callback(ExtFuture<T>) -> R
	 * 	@endcode
	 * where R != [void, ExtFuture<>]
	 *
	 * The continuation must take *this as its first parameter.  It will only be called when
	 * the ExtFuture is finished.
	 *
	 * @note Like std::experimental::future::then(), the continuation function will be run on
	 *       an unspecified thread.
	 * @note ...but as currently implemented, it's always run on the app's main thread.
	 *
	 * @see The various .tap() overloads if you want to pass in a callback which receives the results as they
	 *      are reported to this.
	 *
	 * @tparam F = Continuation function type.  Must accept *this by value as the first parameter.
	 * @tparam R = Return value of continuation F(ExtFuture<T>).
	 *
	 * @return A new future for containing the return value of @a continuation_function.
	 */
	template <typename F, typename R = ct::return_type_t<F> >
	auto then(QObject* context, F&& then_callback)
		-> std::enable_if_t<!std::is_same_v<R, void> && !IsExtFuture<R>,
		ExtFuture<R>>
	{
		static_assert(!std::is_same_v<R, void> && !IsExtFuture<R>, "Wrong overload deduced, then_callback returns ExtFuture<> or void");
		return ThenHelper(context, std::forward<F>(then_callback), *this);
	}

	/**
	 * std::experimental::future-like .then() which takes a continuation function @a then_callback,
	 * of signature:
	 * 	@code
	 * 		then_callback(ExtFuture<T>) -> R
	 * 	@endcode
	 * where R != [void, ExtFuture<>]
	 *
	 * and returns an ExtFuture<R>.
	 *
	 * @tparam R a non-ExtFuture<> type.
	 * @tparam T a non-ExtFuture<> type.
	 *
	 * @param then_callback
	 * @returns ExtFuture<R>
	 */
	template <class F, class R = ct::return_type_t<F> , REQUIRES(!IsExtFuture<R>)>
	ExtFuture<R> then( F&& then_callback )
	{
		// then_callback is always an lvalue.  Pass it to the next function as an lvalue or rvalue depending on the type of F.
		return then(QApplication::instance(), std::forward<F>(then_callback));
	}

	/// @} // END .then() overloads.

	/// @name .tap() overloads.
	/// @{

	/**
	 * Attaches a "tap" callback to this ExtFuture.
	 *
	 * The callback passed to tap() is invoked with results from this, of type T, as they become available.
	 *
	 * @param tap_callback  Callback with the signature void(T).
	 *
	 * @return  A reference to *this, i.e. ExtFuture<T>&.
	 */
	template <typename F, typename R = ct::return_type_t<F>, REQUIRES(argtype_n_is_v<F, 0, T>)>
	ExtFuture<T>& tap(QObject* context, F&& tap_callback)
	{
		static_assert(function_return_type_is_v<decltype(tap_callback), void>, "");

		return TapHelper(context, std::forward<F>(tap_callback)); // *m_tap_function);
	}

	template <typename F, typename R = ct::return_type_t<F>, REQUIRES(argtype_n_is_v<F, 0, T>)>
	ExtFuture<T>& tap(F&& tap_callback)
	{
		qWr() << "ENTER ExtFuture<T>& tap(F&& tap_callback)";
		auto retval = tap(QApplication::instance(), std::forward<F>(tap_callback));
		qWr() << "EXIT ExtFuture<T>& tap(F&& tap_callback)";
		return *this;
	}

#if 0
	ExtFuture<T>& tap(QObject* context, TapCallbackTypeProgress prog_tap_callback)
	{
//		m_tap_progress_function = std::make_shared<TapCallbackTypeProgress>(prog_tap_callback);
		return TapProgressHelper(context, std::forward<TapCallbackTypeProgress>(prog_tap_callback));//*m_tap_progress_function);
	}

	ExtFuture<T>& tap(TapCallbackTypeProgress prog_tap_callback)
	{
		return tap(QApplication::instance(), std::forward<TapCallbackTypeProgress>(prog_tap_callback));
	}
#endif

	/**
	 * Degenerate .tap() case where no callback is specified.
	 * Basically a no-op, simply returns a reference to *this.
	 *
	 * @return Reference to this.
	 */
	ExtFuture<T>& tap()
	{
		return *this;
	}

    /// @} // END .tap() overloads.

	/**
	 * Registers a callback of type void(void) which is always called when this is finished, regardless
	 * of success or failure.
	 * Useful for RAII-like cleanup, etc.
	 *
	 * @note This is sort of a cross between .then() and .tap().  It should return a copy of this,
	 * not a reference to this like .tap() does (shouldn't it?), but the callback should be called when this is finished,
	 * and before the returned ExtFuture<> is finished.
	 *
	 * @param finally_callback
	 * @return
	 */
	template <class F>
	ExtFuture<T> finally(F&& finally_callback)
	{
		// - Create a wrapper lambda for the actual finally callback which takes a reference to this.
		// - Pass the wrapper to .then().
		// -
		auto retval = this->then([func = std::forward<F>(finally_callback)](ExtFuture<T> thiz) mutable {
			// Call the finally_callback.
			std::move(func)();

			// This is Qt5 for .get().  Qt's futures always contain a QList of T's, not just a single result.
/// @todo "TODO: should return .results()"
			return thiz.get();
//			return thiz;
			});

		return retval;
	}



	/**
	 * Block the current thread on the finishing of this ExtFuture, but keep the thread's
	 * event loop running.
	 *
	 * Effectively the same semantics as std::future::wait(), but with Qt's-event-loop pumping, so it only
	 * semi-blocks the thread.
	 */
	void wait() const;

	/// @todo
//	void await();

	/**
	 * Get this' current state as a string.
	 *
	 * @return QString describing the current state of the ExtFuture.
	 */
    ExtFutureState::States state() const;

	/**
	 * Get a string describing this ExtFuture<>, suitable for debug output.
	 * @return
	 */
//	QString debug_string() const
//	{
//		QString retval = "ID: " + this->id();
//		retval += ", STATE: (" + state() + ")";
//		return retval;
//	}

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

	/**
	 * ThenHelper which takes a then_callback which returns a non-ExtFuture<> result.
	 */
	template <typename F, typename... Args, typename R = std::result_of_t<F&&(Args...)>>
	ExtFuture<R> ThenHelper(QObject* context, F&& then_callback, Args&&... args)
	{
//		qDb() << "ENTER";
		static std::atomic_bool s_was_ever_called {false};

		static_assert(sizeof...(Args) <= 1, "Too many args");

		auto watcher = new QFutureWatcher<T>();

		auto retval = new ExtFuture<R>();
//		qDb() << "NEW EXTFUTURE:" << *retval;
		QObject::connect(watcher, &QFutureWatcherBase::finished, watcher,
						 [then_cb = std::decay_t<F>(then_callback), retval, args..., watcher]() mutable -> void {
			// Call the then() callback function.
//			qDb() << "THEN WRAPPER CALLED";
			// f() takes void, val, or ExtFuture<T>.
			// f() returns void, a type R, or an ExtFuture<R>
/// @todo ("CRASH ON CANCEL HERE");
			retval->reportResult(then_cb(std::move(args)...));
			s_was_ever_called = true;
			retval->reportFinished();
//			qDb() << "RETVAL STATUS:" << *retval;
			watcher->deleteLater();
		});
		QObject::connect(watcher, &QFutureWatcherBase::destroyed, [](){
			qWr() << "ThenHelper ExtFutureWatcher DESTROYED";
			Q_ASSERT(s_was_ever_called);
		});
		// Start watching this ExtFuture.
		watcher->setFuture(this->future());
//		qDb() << "RETURNING:" << *retval;
		return *retval;
	}

	/**
	 * FinallyHelper which takes a callback which returns an ExtFuture<>.
	 */
	template <typename F, typename R = ct::return_type_t<F>>
	ExtFuture<T> FinallyHelper(QObject* context, F&& finally_callback)
	{
		static_assert(std::tuple_size_v<ct::args_t<F>> == 0, "Too many args");

        auto watcher = new QFutureWatcher<T>();
/// M_WARNING("TODO: LEAKS THIS ExtFuture<>");
		auto retval = new ExtFuture<R>();
		qDb() << "NEW EXTFUTURE:" << *retval;
		QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, [finally_callback, retval, watcher](){
			// Call the then() callback function.
			qDb() << "THEN WRAPPER CALLED";
			// finally_callback() takes void, returns void.
			finally_callback();
			retval->reportFinished();
			qDb() << "RETVAL STATUS:" << *retval;
			watcher->deleteLater();
		});
        QObject::connect(watcher, &QFutureWatcherBase::destroyed, [](){ qWr() << "FinallyHelper ExtFutureWatcher DESTROYED";});
		watcher->setFuture(this->future());
		return *retval;
	}

	/**
	 * TapHelper which calls tap_callback whenever there's a new result ready.
	 * @param guard_qobject
	 * @param tap_callback   callable with signature void(*)(T)
	 * @return
	 */
	template <typename F>
		std::enable_if_t<ct::is_invocable_r_v<void, F, T>, ExtFuture<T>&>
	TapHelper(QObject *guard_qobject, F&& tap_callback)
	{
		static std::atomic_bool s_was_ever_called {false};

		auto watcher = new QFutureWatcher<T>();
		QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, &QObject::deleteLater);
		QObject::connect(watcher, &QFutureWatcherBase::resultReadyAt, guard_qobject,
				[tap_cb = std::decay_t<F>(tap_callback), watcher](int index) mutable {
//					qDb() << "TAP WRAPPER CALLED, ExtFuture state S/R/F:"
//						  << watcher->isStarted() << watcher->isRunning() << watcher->isFinished();
					// Call the tap callback with the incoming result value.
					tap_cb(watcher->future().resultAt(index));
					s_was_ever_called = true;
			});
		QObject::connect(watcher, &QFutureWatcherBase::destroyed, [](){
//			qWr() << "TapHelper ExtFutureWatcher DESTROYED";
			Q_ASSERT(s_was_ever_called);
		});
		watcher->setFuture(this->future());
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


	ExtFutureWatcher<T>* m_extfuture_watcher = nullptr;
};

//
// START IMPLEMENTATION
//

#include "ExtAsync.h"



template <typename T>
QDebug operator<<(QDebug dbg, const ExtFuture<T> &extfuture)
{
	QDebugStateSaver saver(dbg);

    dbg << "ExtFuture<T>(" << extfuture.state() /*.debug_string()*/ << ")";

	return dbg;
}

//Q_DECLARE_METATYPE(ExtFuture);
//Q_DECLARE_METATYPE_TEMPLATE_1ARG(ExtFuture)


// Include the implementation.
#include "impl/ExtFuture_impl.hpp"

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
template <int = 0, int..., class T, REQUIRES(!IsExtFuture<T>)>
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

template <int = 0, int..., class T = void>
ExtFuture<deduced_type_t<T>> make_exceptional_future(const QException &exception)
{
	return ExtAsync::detail::make_exceptional_future(std::forward<T>(exception));
}

#endif

/// Concept checks.
static_assert(IsExtFuture<ExtFuture<int>>, "");
static_assert(NonNestedExtFuture<ExtFuture<int>>, "");
static_assert(!NonNestedExtFuture<ExtFuture<ExtFuture<int>>>, "");
static_assert(NestedExtFuture<ExtFuture<ExtFuture<int>>>, "");
static_assert(!NestedExtFuture<ExtFuture<int>>, "");
static_assert(!IsExtFuture<int>, "");

#endif /* UTILS_CONCURRENCY_EXTFUTURE_H_ */

