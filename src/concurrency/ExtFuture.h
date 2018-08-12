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

// Future Std C++
#include <future/future_type_traits.hpp>
#include <future/function_traits.hpp>
#include <future/cpp14_concepts.hpp>
#include <future/deduced_type.hpp>
#include <future/Unit.hpp>

// Qt5
#include <QFuture>
#include <QFutureInterface>

// Ours
#include <utils/QtHelpers.h>
#include <utils/ConnectHelpers.h>
#include <utils/StringHelpers.h>
#include <utils/DebugHelpers.h>
#include <utils/UniqueIDMixin.h>

#include "ExtFutureState.h"
#include "ExtFutureWatcher.h"

// Forward declare the ExtAsync namespace
namespace ExtAsync { namespace detail {} }

template <class T>
class ExtFuture;

// Stuff that ExtFuture.h needs to have declared/defined prior to the ExtFuture<> declaration.
#include "ExtAsync_traits.h"


/**
 * A std::shared_future<>-like class implemented on top of Qt5's QFutureInterface<T> class and other facilities.
 *
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
#if 0 /// @note Old ExtFuture<>
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
     * Not sure if we need to do that here or not, we don't have a QRunnable to give it.
	 *
	 * This is the code we're fighting:
	 *
     * @code
	 * void QFutureInterfaceBase::waitForFinished()
		{
			QMutexLocker lock(&d->m_mutex);
			const bool alreadyFinished = !isRunning();
			lock.unlock();

            if (!alreadyFinished)
            {
                /// GRVS: Not finished, so start running it?
				d->pool()->d_func()->stealAndRunRunnable(d->runnable);

				lock.relock();

				while (isRunning())
					d->waitCondition.wait(&d->m_mutex);
			}

			d->m_exceptionStore.throwPossibleException();
		}
     * @endcode
	 *
     * @param initialState  Defaults to State(Started | Running).  Does not appear to waitForFinished()
     *        if it isn't both started and running.
	 */
    explicit ExtFuture(QFutureInterfaceBase::State initialState = QFutureInterfaceBase::State(QFutureInterfaceBase::State::Started
                                                                                              | QFutureInterfaceBase::State::Running))
        : QFutureInterface<T>(initialState)
    {
        //qDb() << "Passed state:" << initialState << "ExtFuture state:" << state();
//        AMLM_ASSERT_EQ(initialState, QFutureInterfaceBase::State::Started);
	}

    /// Default copy constructor.
    /// @note Do we need a non-default copy constructor?
	ExtFuture(const ExtFuture<T>& other) = default;


	/**
	 * Unwrapping constructor, ala std::experimental::future::future, boost::future.
	 *
	 * @todo Not sure if we need this or not.  .then() has an unwrapping overload.
	 */
//	inline explicit ExtFuture(ExtFuture<ExtFuture<T>>&&	other);



    ExtFuture(const QFutureInterface<T>& other) : QFutureInterface<T>(other)
	{
        //qDb() << "future state:" << *this;
//        Q_ASSERT(this->state() == other.state());
	}

	ExtFuture(QFuture<T> other_future) : QFutureInterface<T>(other_future.d)
	{
        //qDb() << "future state:" << *this;
//        Q_ASSERT(this->state() == other_future.d.state());
	}

	/**
	 * Virtual destructor.
	 * @note QFutureInterface<> is derived from QFutureInterfaceBase, which has a virtual destructor.
	 * QFutureInterface<>'s destructor isn't marked either virtual or override.  By the
	 * rules of C++, QFutureInterface<>'s destructor is in fact virtual ("once virtual always virtual"),
	 * so we're good.  Marking this override to avoid further confusion.
     *
     * @note Since we're based on QFutureInterface<T> <- QFutureInterfaceBase, it handles the underlying
     * refcounting for us.  So we may be getting destroyed after we've been copied, which is ok.
     *
     * @todo Find a way to assert if we're still running/not finished and haven't been copied.
     * This would indicate a dangling future.
	 */
    ~ExtFuture() override = default;

    /// @name Copy and Move Assignment operators.
	/// @{

    ExtFuture<T>& operator=(const ExtFuture<T>& other) = default;
    ExtFuture<T>& operator=(ExtFuture<T>&& other) noexcept = default;

	/// @}

	/**
     * Waits until the ExtFuture is finished, and returns the first result.
	 * Essentially the same semantics as std::future::get().
	 *
	 * @note Calls .wait() then returns this->future().result().  This keeps Qt's event loops running.
	 *       Not entirely sure if that's what we should be doing or not, std::future::get() doesn't
	 *       work like that, but this is Qt, so... when in Rome....
	 *
	 * @return The result value of this ExtFuture.
	 */
    T qtget_first();

    /**
     * Waits until the ExtFuture<T> is finished, and returns the resulting QList<T>.
     * Essentially the same semantics as std::future::get(); shared_future::get() always returns a reference instead.
     *
     * @note Directly calls this->future().results().  This blocks any event loop in this thread.
     *
     * @return The results value of this ExtFuture.
     */
    QList<T> get() //const
    {
        /// @todo Not wild about this const_cast<>, but QFuture<> has a QFutureInterface<T>
        /// as a "private" mutable d value member and does this:
        ///     QList<T> results() const { return d.results(); }
        /// ...so hopefully this should be OK.

        return const_cast<ExtFuture<T>*>(this)->results();
//		return this->future().results();
    }

    QList<T> results() // const
    {
        return this->BASE_CLASS::results();
    }

    /**
     * Like .get(), but only returns the first value in the ExtFuture<>'s QList.
     * Not sure why this doesn't exist in sub-QFuture<> classes, but its doesn't.
     */
    inline T result() //const
    {
//        return const_cast<ExtFuture<T>*>(this)->resultAt(0);
        return this->future().resultAt(0);
    }

    inline T resultAt(int index) //const
    {
//        const_cast<ExtFuture<T>*>(this)->waitForResult(index);
//        return const_cast<ExtFuture<T>*>(this)->resultReference(index);
        return this->future().resultAt(index);
    }

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
     * 		R then_callback(ExtFuture<T>)
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
    template <class F, class R = ct::return_type_t<F>,
            REQUIRES(!IsExtFuture<R> && !std::is_same_v<R, void>)>
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
     * @param tap_callback  Callback with the signature void()(T).
	 *
	 * @return  A reference to *this, i.e. ExtFuture<T>&.
	 */
    template <typename TapCallbackType, typename R = ct::return_type_t<TapCallbackType>,
              REQUIRES(ct::is_invocable_r_v<void, TapCallbackType, T>)>
    ExtFuture<T>& tap(QObject* context, TapCallbackType&& tap_callback)
	{
		static_assert(function_return_type_is_v<decltype(tap_callback), void>, "");

        return TapHelper(context, std::forward<TapCallbackType>(tap_callback)); // *m_tap_function);
	}

	template <typename F, typename R = ct::return_type_t<F>, REQUIRES(argtype_n_is_v<F, 0, T>)>
	ExtFuture<T>& tap(F&& tap_callback)
	{
        qIn() << "ENTER ExtFuture<T>& tap(F&& tap_callback)";
		auto retval = tap(QApplication::instance(), std::forward<F>(tap_callback));
        qIn() << "EXIT ExtFuture<T>& tap(F&& tap_callback)";
		return *this;
	}

    /**
     * tap() overload for "streaming" ExtFutures.
     * Callback takes a reference to this, a begin index, and an end index.
     */
    template<typename TapCallbackType,
             REQUIRES(ct::is_invocable_r_v<void, TapCallbackType, ExtFuture<T>&, int, int>)>
    ExtFuture<T>& tap(TapCallbackType&& tap_callback)
    {
        EnsureFWInstantiated();
        Q_ASSERT(this->resultCount() == 0);
        connect_or_die(m_extfuture_watcher, &ExtFutureWatcher<T>::resultsReadyAt,
                       /*context,*/ [=, tap_cb = std::decay_t<TapCallbackType>(tap_callback)](int begin, int end) {
            qDb() << "IN TAP CALLBACK";
            tap_cb(*this, begin, end);
            ;});

        return *this;
    }

    /**
     * A .tap() variant intended solely for testing.  Allows the callback to set the future's objectName,
     * perhaps register it with a watcher, etc.
     * @note Unlike other .tap()s, the callback is called immediately, not when the ExtFuture has finished.
     */
    template<typename TapCallbackType,
             REQUIRES(ct::is_invocable_r_v<void, TapCallbackType, ExtFuture<T>&>)>
    ExtFuture<T>& test_tap(TapCallbackType&& tap_callback)
    {
        tap_callback(*this);
        return *this;
    }

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
            return thiz.qtget_first();
//            return thiz;
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
    void wait();

	/// @todo
//	void await();

	/**
     * Get this' current state as a ExtFutureState::State.
	 *
     * @return A QFlags<>-derived type describing the current state of the ExtFuture.
	 */
    ExtFutureState::State state() const;

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

    template <class FutureType>
    static ExtFutureState::State state(FutureType future);

protected:

	void EnsureFWInstantiated()
	{
		if(!m_extfuture_watcher)
		{
			m_extfuture_watcher = new ExtFutureWatcher<T>();
            m_extfuture_watcher->setFuture(*this);
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
//		static std::atomic_bool s_was_ever_called {false};

		auto watcher = new QFutureWatcher<T>();
        connect_or_die(watcher, &QFutureWatcherBase::finished, watcher, &QObject::deleteLater);
		QObject::connect(watcher, &QFutureWatcherBase::resultReadyAt, guard_qobject,
				[tap_cb = std::decay_t<F>(tap_callback), watcher](int index) mutable {
//					qDb() << "TAP WRAPPER CALLED, ExtFuture state S/R/F:"
//						  << watcher->isStarted() << watcher->isRunning() << watcher->isFinished();
					// Call the tap callback with the incoming result value.
					tap_cb(watcher->future().resultAt(index));
//					s_was_ever_called = true;
			});
//		QObject::connect(watcher, &QFutureWatcherBase::destroyed, [](){
//            qWr() << "TapHelper ExtFutureWatcher DESTROYED";
////			Q_ASSERT(s_was_ever_called);
//		});
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

template<typename T>
static ExtFutureState::State state(const QFuture<T>& qfuture_derived)
{
    return ExtFutureState::state(qfuture_derived.d);
}

template<typename T>
static ExtFutureState::State state(const ExtFuture<T>& ef)
{
    return ef.state();
}

template <typename T>
QDebug operator<<(QDebug dbg, const ExtFuture<T> &extfuture)
{
	QDebugStateSaver saver(dbg);

    dbg << "ExtFuture<T>(" << extfuture.state() /*.debug_string()*/ << ")";

	return dbg;
}

template <typename T>
std::ostream& operator<<(std::ostream& outstream, const ExtFuture<T> &extfuture)
{
    outstream << "ExtFuture<T>( state=" << qUtf8Printable(toString(extfuture.state())) << ")";

    return outstream;
}

//Q_DECLARE_METATYPE(ExtFuture);
//Q_DECLARE_METATYPE_TEMPLATE_1ARG(ExtFuture)


template<typename T>
T ExtFuture<T>::qtget_first()
{
    wait();
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

#else // New ExtFuture<>

template <typename T>
class ExtFuture : public QFuture<T>, public UniqueIDMixin<ExtFuture<T>>
{
    using BASE_CLASS = QFuture<T>;

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
     * Not sure if we need to do that here or not, we don't have a QRunnable to give it.
	 *
	 * This is the code we're fighting:
	 *
     * @code
	 * void QFutureInterfaceBase::waitForFinished()
		{
			QMutexLocker lock(&d->m_mutex);
			const bool alreadyFinished = !isRunning();
			lock.unlock();

            if (!alreadyFinished)
            {
                /// GRVS: Not finished, so start running it?
				d->pool()->d_func()->stealAndRunRunnable(d->runnable);

				lock.relock();

				while (isRunning())
					d->waitCondition.wait(&d->m_mutex);
			}

			d->m_exceptionStore.throwPossibleException();
		}
     * @endcode
	 *
     * @param initialState  Defaults to State(Started | Running).  Does not appear to waitForFinished()
     *        if it isn't both started and running.
	 */
    explicit ExtFuture(QFutureInterfaceBase::State initialState = QFutureInterfaceBase::State(QFutureInterfaceBase::State::Started
                                                                                              | QFutureInterfaceBase::State::Running))
        : QFuture<T>(new QFutureInterface<T>(initialState))
    {
        //qDb() << "Passed state:" << initialState << "ExtFuture state:" << state();
//        AMLM_ASSERT_EQ(initialState, QFutureInterfaceBase::State::Started);
	}

    /// Default copy constructor.
    /// @note Do we need a non-default copy constructor?
	ExtFuture(const ExtFuture<T>& other) = default;


	/**
	 * Unwrapping constructor, ala std::experimental::future::future, boost::future.
	 *
	 * @todo Not sure if we need this or not.  .then() has an unwrapping overload.
	 */
//	inline explicit ExtFuture(ExtFuture<ExtFuture<T>>&&	other);


    /**
     * ExtFuture<T> constructor from const QFutureInterface<T>&.
     * We need this mostly to fill in for QFutureInterface<T>::future(), which
     * generates a QFuture<T>.
     */
    explicit ExtFuture(const QFutureInterface<T>& other) : QFutureInterface<T>(other)
	{
        //qDb() << "future state:" << *this;
//        Q_ASSERT(this->state() == other.state());
	}

    explicit ExtFuture(QFuture<T> other_future) : QFutureInterface<T>(other_future.d)
	{
        //qDb() << "future state:" << *this;
//        Q_ASSERT(this->state() == other_future.d.state());
	}

	/**
	 * Virtual destructor.
	 * @note QFutureInterface<> is derived from QFutureInterfaceBase, which has a virtual destructor.
	 * QFutureInterface<>'s destructor isn't marked either virtual or override.  By the
	 * rules of C++, QFutureInterface<>'s destructor is in fact virtual ("once virtual always virtual"),
	 * so we're good.  Marking this override to avoid further confusion.
     *
     * @note Since we're based on QFutureInterface<T> <- QFutureInterfaceBase, it handles the underlying
     * refcounting for us.  So we may be getting destroyed after we've been copied, which is ok.
     *
     * @todo Find a way to assert if we're still running/not finished and haven't been copied.
     * This would indicate a dangling future.
	 */
    ~ExtFuture() override = default;

    /// @name Copy and Move Assignment operators.
	/// @{

    ExtFuture<T>& operator=(const ExtFuture<T>& other) = default;
    ExtFuture<T>& operator=(ExtFuture<T>&& other) noexcept = default;

	/// @}

    /// @name Reporting interface
    /// @{

    /// From QFutureInterface<T>

    inline void reportResult(const T* result, int index = -1)
    {
        this->d.reportResult(result, index);
    }

    inline void reportResult(const T& result, int index = -1)
    {
        this->d.reportResult(result, index);
    }

    inline void reportResults(const QVector<T> &results, int beginIndex = -1, int count = -1)
    {
        this->d.reportResults(results, beginIndex, count);
    }

    inline void reportFinished(const T *result = nullptr)
    {
        this->d.reportFinished(result);
    }

    /// From QFutureInterfaceBase

    void reportStarted()
    {
        this->reportStarted();
    }

    void reportFinished()
    {
        this->reportFinished();
    }

    void reportCanceled()
    {
        this->reportCanceled();
    }

    void reportException(const QException &e)
    {
        this->reportException(e);
    }

    void reportResultsReady(int beginIndex, int endIndex)
    {
        this->reportResultsReady(beginIndex, endIndex);
    }

    /// Status reporting

    void setProgressRange(int minimum, int maximum)
    {
        this->setProgressRange(minimum, maximum);
    }
    void setProgressValue(int progressValue)
    {
        this->setProgressValue(progressValue);
    }

    void setProgressValueAndText(int progressValue, const QString &progressText)
    {
        this->setProgressValueAndText(progressValue, progressText);
    }

    /// @}

	/**
     * Waits until the ExtFuture is finished, and returns the first result.
	 * Essentially the same semantics as std::future::get().
	 *
	 * @note Calls .wait() then returns this->future().result().  This keeps Qt's event loops running.
	 *       Not entirely sure if that's what we should be doing or not, std::future::get() doesn't
	 *       work like that, but this is Qt, so... when in Rome....
	 *
	 * @return The result value of this ExtFuture.
	 */
    T qtget_first();

    /**
     * Waits until the ExtFuture<T> is finished, and returns the resulting QList<T>.
     * Essentially the same semantics as std::future::get(); shared_future::get() always returns a reference instead.
     *
     * @note Directly calls this->future().results().  This blocks any event loop in this thread.
     *
     * @return The results value of this ExtFuture.
     */
    QList<T> get() //const
    {
        /// @todo Not wild about this const_cast<>, but QFuture<> has a QFutureInterface<T>
        /// as a "private" mutable d value member and does this:
        ///     QList<T> results() const { return d.results(); }
        /// ...so hopefully this should be OK.

        return const_cast<ExtFuture<T>*>(this)->results();
//		return this->future().results();
    }

//    QList<T> results() // const
//    {
//        return this->BASE_CLASS::results();
//    }

    /**
     * Like .get(), but only returns the first value in the ExtFuture<>'s QList.
     * Not sure why this doesn't exist in sub-QFuture<> classes, but its doesn't.
     */
    inline T result() //const
    {
//        return const_cast<ExtFuture<T>*>(this)->resultAt(0);
        return this->future().resultAt(0);
    }

    inline T resultAt(int index) //const
    {
//        const_cast<ExtFuture<T>*>(this)->waitForResult(index);
//        return const_cast<ExtFuture<T>*>(this)->resultReference(index);
        return this->future().resultAt(index);
    }

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
     * 		R then_callback(ExtFuture<T>)
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
    template <class F, class R = ct::return_type_t<F>,
            REQUIRES(!IsExtFuture<R> && !std::is_same_v<R, void>)>
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
     * @param tap_callback  Callback with the signature void()(T).
	 *
	 * @return  A reference to *this, i.e. ExtFuture<T>&.
	 */
    template <typename TapCallbackType, typename R = ct::return_type_t<TapCallbackType>,
              REQUIRES(ct::is_invocable_r_v<void, TapCallbackType, T>)>
    ExtFuture<T>& tap(QObject* context, TapCallbackType&& tap_callback)
	{
		static_assert(function_return_type_is_v<decltype(tap_callback), void>, "");

        return TapHelper(context, std::forward<TapCallbackType>(tap_callback)); // *m_tap_function);
	}

	template <typename F, typename R = ct::return_type_t<F>, REQUIRES(argtype_n_is_v<F, 0, T>)>
	ExtFuture<T>& tap(F&& tap_callback)
	{
        qIn() << "ENTER ExtFuture<T>& tap(F&& tap_callback)";
		auto retval = tap(QApplication::instance(), std::forward<F>(tap_callback));
        qIn() << "EXIT ExtFuture<T>& tap(F&& tap_callback)";
		return *this;
	}

    /**
     * tap() overload for "streaming" ExtFutures.
     * Callback takes a reference to this, a begin index, and an end index.
     */
    template<typename TapCallbackType,
             REQUIRES(ct::is_invocable_r_v<void, TapCallbackType, ExtFuture<T>&, int, int>)>
    ExtFuture<T>& tap(TapCallbackType&& tap_callback)
    {
        EnsureFWInstantiated();
        Q_ASSERT(this->resultCount() == 0);
        connect_or_die(m_extfuture_watcher, &ExtFutureWatcher<T>::resultsReadyAt,
                       /*context,*/ [=, tap_cb = std::decay_t<TapCallbackType>(tap_callback)](int begin, int end) {
            qDb() << "IN TAP CALLBACK";
            tap_cb(*this, begin, end);
            ;});

        return *this;
    }

    /**
     * A .tap() variant intended solely for testing.  Allows the callback to set the future's objectName,
     * perhaps register it with a watcher, etc.
     * @note Unlike other .tap()s, the callback is called immediately, not when the ExtFuture has finished.
     */
    template<typename TapCallbackType,
             REQUIRES(ct::is_invocable_r_v<void, TapCallbackType, ExtFuture<T>&>)>
    ExtFuture<T>& test_tap(TapCallbackType&& tap_callback)
    {
        tap_callback(*this);
        return *this;
    }

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
            return thiz.qtget_first();
//            return thiz;
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
    void wait();

	/// @todo
//	void await();

	/**
     * Get this' current state as a ExtFutureState::State.
	 *
     * @return A QFlags<>-derived type describing the current state of the ExtFuture.
	 */
    ExtFutureState::State state() const;

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

    template <class FutureType>
    static ExtFutureState::State state(FutureType future);

protected:

	void EnsureFWInstantiated()
	{
		if(!m_extfuture_watcher)
		{
			m_extfuture_watcher = new ExtFutureWatcher<T>();
            m_extfuture_watcher->setFuture(*this);
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
//		static std::atomic_bool s_was_ever_called {false};

		auto watcher = new QFutureWatcher<T>();
        connect_or_die(watcher, &QFutureWatcherBase::finished, watcher, &QObject::deleteLater);
		QObject::connect(watcher, &QFutureWatcherBase::resultReadyAt, guard_qobject,
				[tap_cb = std::decay_t<F>(tap_callback), watcher](int index) mutable {
//					qDb() << "TAP WRAPPER CALLED, ExtFuture state S/R/F:"
//						  << watcher->isStarted() << watcher->isRunning() << watcher->isFinished();
					// Call the tap callback with the incoming result value.
					tap_cb(watcher->future().resultAt(index));
//					s_was_ever_called = true;
			});
//		QObject::connect(watcher, &QFutureWatcherBase::destroyed, [](){
//            qWr() << "TapHelper ExtFutureWatcher DESTROYED";
////			Q_ASSERT(s_was_ever_called);
//		});
        watcher->setFuture(*this);
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

#endif // New ExtFuture<>

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

/**
 * Creates a completed future containing the value @a value.
 *
 * @param value
 * @return  A ready ExtFuture<deduced_type_t<T>>();
 */
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
template <int = 0, int..., class T, REQUIRES(!IsExtFuture<T>)>
ExtFuture<deduced_type_t<T>> make_ready_future(T&& value)
{
    return ExtAsync::detail::make_ready_future(std::forward<T>(value));
}

template <int = 0, int..., class T = void>
ExtFuture<deduced_type_t<T>> make_exceptional_future(const QException &exception)
{
	return ExtAsync::detail::make_exceptional_future(std::forward<T>(exception));
}


#endif /* UTILS_CONCURRENCY_EXTFUTURE_H_ */

