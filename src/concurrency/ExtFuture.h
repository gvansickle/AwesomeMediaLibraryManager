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

#ifndef SRC_CONCURRENCY_EXTFUTURE_H
#define SRC_CONCURRENCY_EXTFUTURE_H

/**
 * @file
 * An extended QFuture<T> class.
 */

#pragma once

// Std C++
#include <memory>
#include <type_traits>
#include <functional>

// Future Std C++
#include <future/future_type_traits.hpp>
#include <future/function_traits.hpp>
#include <future/cpp14_concepts.hpp>
//#include <future/deduced_type.hpp>
#include <future/Unit.hpp>

// Qt5
#include <QtConcurrent>
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
#include "ExtFutureProgressInfo.h"

// Forward declare the ExtAsync namespace
namespace ExtAsync { namespace detail {} }

template <class T>
class ExtFuture;

class ExtAsyncCancelException : public QException
{

};

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
template <typename T>
class ExtFuture : public QFuture<T>//, public UniqueIDMixin<ExtFuture<T>>
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
	 * QFuture<T>() defaults to Started | Canceled | Finished.  Not sure we want that, or why that is.

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
	 *        if it isn't both Started and Running.
	 */
#if 0
	ExtFuture() : QFuture<T>() {}
#else
	ExtFuture(QFutureInterfaceBase::State initialState = QFutureInterfaceBase::State(QFutureInterfaceBase::State::Started
																							  | QFutureInterfaceBase::State::Running))
		: QFuture<T>(new QFutureInterface<T>(initialState))	{}
#endif
	/// Copy constructor.
	ExtFuture(const ExtFuture<T>& other) : QFuture<T>(&(other.d)),
			m_progress_unit(other.m_progress_unit)
	{}

	/// Move constructor
	/// @note Qt5's QFuture doesn't have this.
//	ExtFuture(ExtFuture<T>&& other) noexcept = delete;// : QFuture<T>(other) {};

	/// Converting constructor from QFuture<T>.
	ExtFuture(const QFuture<T>& f) : ExtFuture(&(f.d)) {}
	/// Move construct from QFuture.
//	ExtFuture(QFuture<T>&& f) noexcept = delete;// : BASE_CLASS(f) {};// = delete;

	/// Copy construct from QFuture<void>.
	/// @todo Something's broken here, this doesn't actually compile.
//	ExtFuture(const QFuture<void>& f) : ExtFuture<Unit>(f) {};

	explicit ExtFuture(QFutureInterface<T> *p) // ~Internal, see QFuture<>().
		: BASE_CLASS(p) {}

	/**
	 * Unwrapping constructor, ala std::experimental::future::future, boost::future.
	 * @note Honeypot for catching nested ExtFuture<>s and asserting at compile time.
	 * Not sure if we really need this or not.
	 */
#define REAL_ONE_DOESNT_WORK
#ifdef REAL_ONE_DOESNT_WORK
	template <class ExtFutureExtFutureT,
			  REQUIRES(NestedExtFuture<ExtFutureExtFutureT>)>
	inline explicit ExtFuture(ExtFuture<ExtFuture<T>>&&	other)
	{
		Q_UNUSED(other);
		static_assert(NestedExtFuture<ExtFutureExtFutureT>, "Nested ExtFutures not supported");
	}
#else
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
#endif

	/**
	 * Virtual destructor.
	 *
	 * @todo We're now deriving from QFuture<T>, which doesn't have a virtual destructor, so I'm not sure
	 * how correct this is or if it even makes any sense.
	 *
	 * @note We used to derive from QFutureInterface<>:
	 * QFutureInterface<> is derived from QFutureInterfaceBase, which has a virtual destructor.
	 * QFutureInterface<>'s destructor isn't marked either virtual or override.  By the
	 * rules of C++, QFutureInterface<>'s destructor is in fact virtual ("once virtual always virtual"),
	 * so we're good.  Marking this override to avoid further confusion.
	 */
	~ExtFuture() = default;

	/// @name Copy and Move Assignment operators.
	/// @{

	/// Copy assignment.
	ExtFuture<T>& operator=(const ExtFuture<T>& other)
	{
		if(this != &other)
		{
			this->BASE_CLASS::operator=(other);
			this->m_progress_unit = other.m_progress_unit;
		}
		return *this;
	}

	/// Move assignment.
//	ExtFuture<T>& operator=(ExtFuture<T>&& other) noexcept = delete;
//	{
//		this->BASE_CLASS::operator=(other);
//		return *this;
//	}

	/// Copy assignment from QFuture<T>.
	ExtFuture<T>& operator=(const BASE_CLASS& other)
	{
		this->BASE_CLASS::operator=(other);
		return *this;
	}

	/// @}

	/**
	 * Conversion operator to QFuture<T>.
	 */
//	operator QFuture<T>() const
//	{
//		return QFuture<T>(&QFuture<T>::d);
//	}

	/// @name Comparison operators.
	/// @{
	bool operator==(const ExtFuture &other) const { return this->BASE_CLASS::operator==(other); }
	bool operator!=(const ExtFuture &other) const { return this->BASE_CLASS::operator!=(other); }
	/// @}

	/// @name Reporting interface
	/// @{

	/**
	 * Cancel/Pause/Resume helper function for while(1) loops reporting/controlled by this ExtFuture<T>.
	 *
	 * Call this at the bottom of your while(1) loop.  If the call returns true,
	 * you're being canceled and must break out of the loop, report canceled, and return.
	 *
	 * Handles pause/resume completely internally, nothing more needs to be done in the calling loop.
	 *
	 * @return true if loop in runFunctor() should break due to being canceled.
	 * @return
	 */
	bool HandlePauseResumeShouldICancel()
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

	/**
	 * If result is != nullptr, calls to reportResult() and adds a copy of the result.
	 * Unconditionally reports finished.
	 * @param result
	 */
	inline void reportFinished(const T *result = nullptr)
	{
		this->d.reportFinished(result);
	}

	/// From QFutureInterfaceBase

	void reportStarted()
	{
		/**
		 * RunFunctionTaskBase does this on start() (this derived from QFutureInterface<T>):
		 * this->reportStarted();
         * QFuture<T> theFuture = this->future();
         * pool->start(this, //m_priority// 0);
         * return theFuture;
		 */
		this->d.reportStarted();
	}

	/**
	 * Simply calls cancel().
	 * QFutureInterfaceBase::cancel() in turn can only be called once, returns immediately after the first cancel.
	 */
	void reportCanceled()
	{
		this->d.reportCanceled();
	}

	/**
	 * @note Does nothing if this future's state is (Canceled|Finished).
	 * @param e
	 */
	void reportException(const QException &e)
	{
		this->d.reportException(e);
	}

	void reportResultsReady(int beginIndex, int endIndex)
	{
		this->d.reportResultsReady(beginIndex, endIndex);
	}

	void waitForResume()
	{
		this->d.waitForResume();
	}

	/// @name Status reporting interface.
	/// Call these only from your ExtAsync::run() callback.
public:

	void setProgressRange(int minimum, int maximum)
	{
		this->d.setProgressRange(minimum, maximum);
	}
	void setProgressValue(int progressValue)
	{
		this->d.setProgressValue(progressValue);
	}

	void setProgressValueAndText(int progressValue, const QString &progressText)
	{
		this->d.setProgressValueAndText(progressValue, progressText);
	}

	/// @name KJob-inspired status/information reporting interfaces.
	/// Note that these ultimately need to be squeezed through QFutureInterface's progress reporting
	/// mechanism so we can make use of it's progress throttling/anti-flooding mechanism.
	/// The QFutureInterface progress reporting iface is unfortunately pretty limited:  For text,
	/// there's a single entrypoint:
	/// @code
	///     void setProgressValueAndText(int progressValue, const QString &progressText);
	/// @endcode
	/// And complicating factors, this function early-outs on the following conditions:
	/// @code
	///     if (d->m_progressValue >= progressValue)
	///         return;
	///     if (d->state.load() & (Canceled|Finished))
	///         return;
	/// @endcode
	/// @{

	/// @todo
	void setProgressUnit(/*KJob::Unit*/ int prog_unit)
	{
//		ExtFutureProgressInfo pi;

//		pi.fromSetProgressUnit(prog_unit);

//		INTERNAL_reportKJobProgressInfo(pi);
//		static_assert(sizeof(int) == 4);
		m_progress_unit = prog_unit;
	}

	/// @todo
	void setProcessedAmount(/*KJob::Unit*/ int unit, qulonglong amount);
	void setTotalAmount(/*KJob::Unit*/ int unit, qulonglong amount);

	void reportDescription(const QString &title, const QPair< QString, QString > &field1=QPair< QString, QString >(),
						   const QPair< QString, QString > &field2=QPair< QString, QString >())
	{
		ExtFutureProgressInfo pi;

		pi.fromKJobDescription(title, field1, field2);

		INTERNAL_reportKJobProgressInfo(pi);
	}

	void reportInfoMessage(const QString &plain, const QString &rich=QString())
	{
		ExtFutureProgressInfo pi;

		pi.fromKJobInfoMessage(plain, rich);

		INTERNAL_reportKJobProgressInfo(pi);
	}

	void reportWarning(const QString &plain, const QString &rich=QString())
	{
		ExtFutureProgressInfo pi;

		pi.fromKJobWarning(plain, rich);

		INTERNAL_reportKJobProgressInfo(pi);
	}

private:

	void INTERNAL_reportKJobProgressInfo(const ExtFutureProgressInfo& pi)
	{
		/// @todo There's a race here, both progressValue() and setProgressValueAndText()
		/// individulaly acquire/release the QFIB mutex.
		auto current_progress_val = this->d.progressValue();

		this->d.setProgressValueAndText(current_progress_val+1, pi);

		this->d.setProgressValue(current_progress_val);
	}

	/**
	 * An attempt to squeeze KJob progress info through QFuture{Watcher}'s interface.
	 * This is a bit of a mess.  We have two QFuture<T>/QFutureInterfaceBase channels for ints:
	 * - QFutureInterfaceBase::setProgressRange(int minimum, int maximum), which does this:
	 * 	QMutexLocker locker(&d->m_mutex);
	 * 	d->m_progressMinimum = minimum;
	 * 	d->m_progressMaximum = maximum;
	 * 	d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::ProgressRange, minimum, maximum));
	 * - void QFutureInterfaceBase::setProgressValue(int progressValue), which does more:
	 * @code
	 * @endcode
	 *
	 */
	void INTERNAL_reportKJobProgressNumericalInfo(const ExtFutureProgressInfo& pi)
	{

	}

public:

	/// @}

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
	 * @note Directly calls this->results().  This blocks any event loop in this thread.
	 *
	 * @return The results value of this ExtFuture.
	 */
	QList<T> get() const
	{
		auto retval = this->results();
		return retval;
	}

	/**
	 * QFuture<T> has result(), results(), resultAt(), and isResultReadyAt().
	 */

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
//	template <typename F, typename R = ct::return_type_t<F>,
//			  REQUIRES(is_non_void_non_ExtFuture_v<R>
//			  && ct::is_invocable_r_v<R, F, ExtFuture<T>>)
//			  >
//	ExtFuture<R> then(QObject* context, F&& then_callback)
//	{
//		return this->ThenHelper(context, std::forward<F>(then_callback));
//	}

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
	template <class F, class R = Unit::LiftT<ct::return_type_t<F>>,
			REQUIRES(is_non_void_non_ExtFuture_v<R>
			  && ct::is_invocable_r_v<R, F, ExtFuture<T>>)>
	ExtFuture<R> then( F&& then_callback )
	{
		// then_callback is always an lvalue.  Pass it to the next function as an lvalue or rvalue depending on the type of F.
		return this->then(/*QApplication::instance()*/nullptr, std::forward<F>(then_callback));
	}

	/// @} // END .then() overloads.

	/// @name .tap() overloads.
	/// @{

	/**
	 * Attaches a "tap" callback to this ExtFuture.
	 *
	 * The callback passed to tap() is invoked with individual results from this, of type T, as they become available.
	 *
	 * @param tap_callback  Callback with the signature void()(T).
	 *
	 * @return ExtFuture<T>
	 */
	template <typename TapCallbackType,
			  REQUIRES(ct::is_invocable_r_v<void, TapCallbackType, T>)>
	ExtFuture<T> tap(QObject* context, TapCallbackType&& tap_callback)
	{
		return this->TapHelper(context, std::forward<TapCallbackType>(tap_callback));
	}

	/**
	 * Attaches a "tap" callback to this ExtFuture.
	 *
	 * The callback passed to tap() is invoked with individual results from this, of type T, as they become available.
	 *
	 * @param tap_callback  Callback with the signature void()(T).
	 *
	 * @return ExtFuture<T>
	 */
	template <typename TapCallbackType,
			  REQUIRES(ct::is_invocable_r_v<void, TapCallbackType, T>)>
	ExtFuture<T> tap(TapCallbackType&& tap_callback)
	{
		auto retval = this->tap(QApplication::instance(), std::forward<TapCallbackType>(tap_callback));

		return retval;
	}

	/**
	 * tap() overload for "streaming" taps.
	 * Callback takes a reference to this, a begin index, and an end index:
	 * @code
	 *      void TapCallback(ExtFuture<T> ef, int begin, int end)
	 * @endcode
	 *
	 * @returns An ExtFuture<T> which is made ready when this is completed.
	 */
	template<typename StreamingTapCallbackType,
			 REQUIRES(ct::is_invocable_r_v<void, StreamingTapCallbackType, ExtFuture<T>, int, int>)>
	ExtFuture<T> tap(QObject* context, StreamingTapCallbackType&& tap_callback)
	{
#if 0
//        EnsureFWInstantiated();

//        ExtFuture<T>* ef_copy = new ExtFuture<T>;
		ExtFuture<T> ef_copy;

		// Create a new FutureWatcher<T>
		auto* watcher = new_self_destruct_futurewatcher(context);

		connect_or_die(watcher, &QFutureWatcher<T>::resultsReadyAt,
					   context, [=, tap_cb = std::decay_t<TapCallbackType>(tap_callback)](int begin, int end) mutable {
			qDb() << "IN TAP CALLBACK, begin:" << begin << ", end:" << end;
			tap_cb(*this, begin, end);
			;});
		connect_or_die(watcher, &QFutureWatcher<T>::finished, context, [=]() mutable {
			qDb() << "FUTURE FINISHED:" << *this;
			// The idea here is to make sure any .then() after this .tap() gets called second.
			ef_copy = *this;
			ef_copy.reportFinished();
			qDb() << "FUTURE FINISHED COPY:" << ef_copy;
			Q_ASSERT(ef_copy.isFinished());
		});
		connect_or_die(watcher, &QFutureWatcher<T>::canceled, context, [=]() mutable {
			qDb() << "FUTURE CANCELED:" << *this;
			// The idea here is to make sure any .then() after this .tap() gets called second.
			ef_copy = *this;
			ef_copy.reportCanceled();
			qDb() << "FUTURE CANCELED COPY:" << ef_copy;
			Q_ASSERT(ef_copy.isCanceled());
		});

		watcher->setFuture(*this);

		return ef_copy;
#endif
		return this->StreamingTapHelper(context, std::forward<StreamingTapCallbackType>(tap_callback));
	}

	template<typename StreamingTapCallbackType,
			 REQUIRES(ct::is_invocable_r_v<void, StreamingTapCallbackType, ExtFuture<T>, int, int>)>
	ExtFuture<T> tap(StreamingTapCallbackType&& tap_callback)
	{
		return this->tap(qApp, std::forward<StreamingTapCallbackType>(tap_callback));
	}

	/**
	 * A .tap() variant intended solely for testing.  Allows the callback to set the future's objectName,
	 * perhaps register it with a watcher, etc.
	 * @note Unlike other .tap()s, the callback is called immediately, not when the ExtFuture has finished.
	 */
	template<typename TapCallbackType,
			 REQUIRES(ct::is_invocable_r_v<void, TapCallbackType, ExtFuture<T>>)>
	ExtFuture<T> test_tap(TapCallbackType&& tap_callback)
	{
		std::invoke(tap_callback, *this);
		return *this;
	}

	/**
	 * Degenerate .tap() case where no callback is specified.
	 * Basically a no-op, simply returns a copy of *this.
	 *
	 * @return Reference to this.
	 */
	ExtFuture<T> tap()
	{
		return *this;
	}

	/// @} // END .tap() overloads.

	/**
	 * Registers a callback of type void(void) which is always called when this is finished, regardless
	 * of success or failure.
	 * Useful for RAII-like cleanup, etc.
	 *
	 * @note This is sort of a cross between .then() and .tap().
	 *
	 * @param finally_callback
	 * @return
	 */
	template <class FinallyCallbackType,
			  REQUIRES(std::is_invocable_v<FinallyCallbackType>)>
	ExtFuture<Unit> finally(FinallyCallbackType&& finally_callback)
	{
		ExtFuture<Unit> retval = this->then([=, finally_callback_copy = std::decay_t<FinallyCallbackType>(finally_callback)](ExtFuture<T> this_future) -> Unit {
			this_future.waitForFinished();

			// Call the finally_callback.
			finally_callback_copy();

			return unit;
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

	/**
	 * Get this' current state as a ExtFutureState::State.
	 *
	 * @return A QFlags<>-derived type describing the current state of the ExtFuture.
	 */
	ExtFutureState::State state() const;

	template <class FutureType>
	static ExtFutureState::State state(const FutureType& future);

protected:

//	void EnsureFWInstantiated()
//	{
//		if(!m_extfuture_watcher)
//		{
//			m_extfuture_watcher = new ExtFutureWatcher<T>();
//            m_extfuture_watcher->setFuture(*this);
//		}
//	}

	/**
	 * Simple wrapper allowing us to call this->d.waitForResult(i) without
	 * directly touching the not-really-public QFutureInterfaceBase instance d.
	 *
	 * Used by StreamingTapHelper.
	 *
	 * @param resultIndex
	 */
	void waitForResult(int resultIndex)
	{
		this->d.waitForResult(resultIndex);
	}


	/**
	 * The then() implementation.
	 * Takes a context object and a then_callback which returns a non-void-non-ExtFuture<> result.
	 * If context == nullptr, then_callback will be run in an arbitrary thread.
	 * If context points to a QObject, then_callback will be run in its event loop.
	 */
	template <typename F,
			  typename R = Unit::LiftT<std::invoke_result_t<F, ExtFuture<T>>>,
			  REQUIRES(is_non_void_non_ExtFuture_v<R>
			  && ct::is_invocable_r_v<R, F, ExtFuture<T>>)>
	ExtFuture<R> then(QObject* context, F&& then_callback)
	{
		if(context != nullptr)
		{
			// If non-null, make sure context has an event loop.
			QThread* ctx_thread = context->thread();
			Q_ASSERT(ctx_thread != nullptr);
			Q_ASSERT(ctx_thread->eventDispatcher() != nullptr);
		}

		/**
		 * @todo Exception handling.
		 * This is the basic pattern used in the RunFunctionTaskBase<> and RunFunctionTask<> class templates from Qt5's internals.
		 *  Note the use of reportException():
		 * run()
		 * {
		 * 	if (this->isCanceled())
    		{
        		// We've been cancelled before we started, just report finished.
        		/// @note It seems like we also should be calling reportCancelled(),
        		/// but neither of Qt5's RunFunctionTask<T> nor Qt Creator's AsyncJob<> (derived only from QRunnable)
        		/// do so.
        		this->reportFinished();
        		return;
    		}

    		try
    		{
		 * 		this->m_task->run(*this);
			} catch (QException &e) {
				QFutureInterface<T>::reportException(e);
			} catch (...) {
				QFutureInterface<T>::reportException(QUnhandledException());
			}

			this->reportResult(result);
    		this->reportFinished();
    		}
		 */

		// The future we'll return.
		ExtFuture<R> retfuture;

		QtConcurrent::run([=, then_callback_copy = std::decay_t<F>(then_callback)](ExtFuture<T> this_future, ExtFuture<R> ret_future) {

			qDb() << "PRE-THEN: START THEN RUN(), thisfuture:" << this_future.state() << "ret_future:" << ret_future.state();

			qDb() << "THREAD ID:" << QThread::currentThreadId();
			PropagateDownstreamCancelsUpstream(ret_future, this_future);

//			try
//			{
				// Wait for this future to finish.
				// This could throw a propagated exceptions from upstream.
				qDb() << "THEN: Waiting for upstream" << this_future <<  "to finish";
M_WARNING("NEVER FINISHING");
				this_future.waitForFinished();
				qDb() << "THEN: upstream finished";
//			}
//			catch(ExtAsyncCancelException& e)
//			{
//				/**
//				 * Per std::experimental::shared_future::then() at @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
//				 * "Any value returned from the continuation is stored as the result in the shared state of the returned future object.
//				 *  Any exception propagated from the execution of the continuation is stored as the exceptional result in the shared
//				 *  state of the returned future object."
//				 */
//				qDb() << "PRE-THEN: upstream threw Cancel" << ExtFutureState::state(this_future);
//				ret_future.reportException(e);
//				// I think we don't want to rethrow like this here.  This will throw to the wrong future
//				// (the QtConcurrent::run() retval, see above.
//				//throw;
//			}
//			catch(QException& e)
//			{
//				qDb() << "PRE-THEN: upstream threw QException" << ExtFutureState::state(this_future);
//				ret_future.reportException(e);
//			}
//			catch (...)
//			{
//				qDb() << "PRE-THEN: upstream threw unknown" << ExtFutureState::state(this_future);
//				ret_future.reportException(QUnhandledException());
//			}

			// Ok, thisfuture is finished.
			// Is it really canceled?
//			if(this_future.isCanceled())
//			{
//				// Propagate the cancel to the returned future.
//				/// @todo Or should this throw a cancellation exception?
//				ret_future.cancel();
//			}

			// Call the then callback.
			// We should never end up calling then_callback_copy with a non-finished future.
			// We call it either directly (no context), or via an event sent to the context object's event loop.
			R retval;
			if constexpr (true)//context == nullptr)
			{
				retval = std::invoke(then_callback_copy, this_future);
			}
			else
			{
				qWr() << "RUNNING IN EVENT LOOP";
				retval = run_in_event_loop(context, then_callback_copy);
			}

			qDb() << "THEN: then_callback CALLED and RETURNED, reporting FINISHED on ret_future with retval."; //, retval:" << retval;

			ret_future.reportFinished(&retval);

			Q_ASSERT(ret_future.isFinished());

			// Check final state.  We know it's at least Finished.
			/// @todo Could we be Finished here with pending results?
			/// Don't care as much on non-Finished cases.
//			if(thisfuture.isCanceled())
//			{
//				qDb() << "TAP: ef cancelled:" << thisfuture.state();
//				ret_future.reportCanceled();
//			}
//			else if(thisfuture.isFinished())
//			{
//				qDb() << "TAP: ef finished:" << thisfuture.state();
//				ret_future.reportFinished();
//			}
//			else
//			{
//				/// @todo Exceptions.
//				qDb() << "NOT FINISHED OR CANCELED:" << thisfuture.state();
//				Q_ASSERT(0);
//			}
		},
		*this,
		retfuture);

		return retfuture;
#if 0
//		qDb() << "ENTER";

		static_assert(sizeof...(Args) <= 1, "Too many args");

		auto watcher = new QFutureWatcher<T>();

		auto retval = new ExtFuture<R>();

		/*QObject::*/connect_or_die(watcher, &QFutureWatcherBase::finished, watcher,
						 [then_cb = std::decay_t<F>(then_callback), retval, args..., watcher]() mutable -> void {
			// Call the then() callback function.
//			qDb() << "THEN WRAPPER CALLED";
			// f() takes void, val, or ExtFuture<T>.
			// f() returns void, a type R, or an ExtFuture<R>
			retval->reportResult(then_cb(std::move(args)...));
			retval->reportFinished();
//			qDb() << "RETVAL STATUS:" << *retval;
			watcher->deleteLater();
		});
		// Start watching this ExtFuture.
		watcher->setFuture(*this);
//		qDb() << "RETURNING:" << *retval;
		return *retval;
#endif
	}
#if 0
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
#endif

	/**
	 * TapHelper which calls tap_callback whenever there's a new result ready.
	 * @param guard_qobject
	 * @param tap_callback   callable with signature void(*)(T)
	 * @return
	 */
	template <typename F,
		REQUIRES(ct::is_invocable_r_v<void, F, T>)
		>
	ExtFuture<T> TapHelper(QObject *guard_qobject, F&& tap_callback)
	{
		return StreamingTapHelper(guard_qobject, [=, tap_cb = std::decay_t<F>(tap_callback)](ExtFuture<T> f, int begin, int end) {
			Q_ASSERT(f.isStarted());
//			Q_ASSERT(!f.isFinished());
			for(auto i = begin; i < end; ++i)
			{
				std::invoke(tap_cb, f.resultAt(i));
			}
		});
#if 0
		auto watcher = new QFutureWatcher<T>();
		connect_or_die(watcher, &QFutureWatcherBase::finished, watcher, &QObject::deleteLater);
		connect_or_die(watcher, &QFutureWatcherBase::resultReadyAt, guard_qobject,
				[tap_cb = std::decay_t<F>(tap_callback), watcher](int index) mutable {
//					qDb() << "TAP WRAPPER CALLED, ExtFuture state S/R/F:"
//						  << watcher->isStarted() << watcher->isRunning() << watcher->isFinished();
					// Call the tap callback with the incoming result value.
					tap_cb(watcher->future().resultAt(index));
			});
		watcher->setFuture(*this);
		return *this;
#endif
	}

	/**
	 * Helper for streaming taps.  Calls streaming_tap_callback with (ExtFuture<T>, begin_index, end_index) whenever
	 * the future has new results ready.
	 * @param guard_qobject  @todo Currently unused.
	 * @param streaming_tap_callback   callable with signature void(*)(ExtFuture<T>, int, int)
	 * @return
	 */
	template <typename F,
		REQUIRES(ct::is_invocable_r_v<void, F, ExtFuture<T>, int, int>)
		>
	ExtFuture<T> StreamingTapHelper(QObject *guard_qobject, F&& streaming_tap_callback)
	{
		ExtFuture<T> ret_future;

		try
		{
			QtConcurrent::run([=, streaming_tap_callback_copy = std::decay_t<F>(streaming_tap_callback)](ExtFuture<T> this_future, ExtFuture<T> f2) {
				qDb() << "TAP: START TAP RUN(), this_future:" << this_future.state() << "f2:" << f2.state();

				int i = 0;

				while(true)
				{
					qDb() << "TAP: Waiting for next result";
					/**
					  * QFutureInterfaceBase::waitForResult(int resultIndex)
					  * - if exception, rethrow.
					  * - if !running, return.
					  * - stealAndRunRunnable()
					  * - lock mutex.
					  * - const int waitIndex = (resultIndex == -1) ? INT_MAX : resultIndex;
					  *   while (isRunning() && !d->internal_isResultReadyAt(waitIndex))
					  *     d->waitCondition.wait(&d->m_mutex);
					  *   d->m_exceptionStore.throwPossibleException();
					  */
					this_future.waitForResult(i);

					// Check if the wait failed to result in any results.
					int result_count = this_future.resultCount();
					if(result_count <= i)
					{
						// No new results, must have finshed etc.
						qDb() << "NO NEW RESULTS, BREAKING, this_future:" << this_future.state();
						break;
					}

					// Call the tap callback.
					//				streaming_tap_callback_copy(ef, i, result_count);
					qDb() << "CALLING TAP CALLBACK, this_future:" << this_future.state();
					std::invoke(streaming_tap_callback_copy, this_future, i, result_count);

					// Copy the new results to the returned future.
					for(; i < result_count; ++i)
					{
						qDb() << "TAP: Next result available at i = " << i;

						T the_next_val = this_future.resultAt(i);
						f2.reportResult(the_next_val);
					}
				}

				qDb() << "LEFT WHILE(!Finished) LOOP, ef state:" << this_future.state();

				// Check final state.  We know it's at least Finished.
				/// @todo Could we be Finished here with pending results?
				/// Don't care as much on non-Finished cases.
				if(this_future.isCanceled())
				{
					qDb() << "TAP: this_future cancelled:" << this_future.state();
					f2.reportCanceled();
				}
				else if(this_future.isFinished())
				{
					qDb() << "TAP: ef finished:" << this_future.state();
					f2.reportFinished();
				}
				else
				{
					/// @todo Exceptions.
					qDb() << "NOT FINISHED OR CANCELED:" << this_future.state();
					Q_ASSERT(0);
				}
			},
			*this,
			ret_future);
		}
		catch(ExtAsyncCancelException& e)
		{
			/**
			 * Per std::experimental::shared_future::then() at @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
			 * "Any value returned from the continuation is stored as the result in the shared state of the returned future object.
			 *  Any exception propagated from the execution of the continuation is stored as the exceptional result in the shared
			 *  state of the returned future object."
			 */
			ret_future.reportException(e);
			// I think we don't want to rethrow like this here.  This will throw to the wrong future
			// (the QtConcurrent::run() retval, see above.
			//throw;
		}
		catch(QException& e)
		{
			ret_future.reportException(e);
		}
		catch (...)
		{
			ret_future.reportException(QUnhandledException());
		}

		return ret_future;
	}

#if 0
	template <typename Function>
	ExtFuture<T>& TapProgressHelper(QObject *guard_qobject, Function f)
	{
		qDb() << "ENTER";
		auto watcher = new ExtFutureWatcher<T>();
		connect_or_die(watcher, &QFutureWatcherBase::finished, watcher, &QObject::deleteLater);
		watcher->onProgressChange([f, watcher](int min, int val, int max, QString text){
			f({min, val, max, text});
			;});
		QObject::connect(watcher, &QFutureWatcherBase::destroyed, [](){ qWr() << "TAPPROGRESS ExtFutureWatcher DESTROYED";});
		watcher->setFuture(*this);
		qDb() << "EXIT";
		return *this;
	}
#endif

	/**
	 * Wait for either of the in or out futures to finish or throw.
	 */
//	template <class ExtFutureR>
//	void wait_for_either(ExtFutureT in_fut, ExtFutureT out_fut)
//	{
//		// There's probably a much better way to do this, but....

//		for(const auto& f : futures)
//		{
//			if(f.isFinished())
//			{
//				// Finished, add it to the finshed bin.
//				finished_futures.push_back(f);
//			}
//		}

//		// Did we get any?
//		if(!finished_futures.empty())
//		{
//			return;
//		}
//	}

	template <class ExtFutureR>
	static void PropagateDownstreamCancelsUpstream(ExtFutureR ret_future, ExtFuture<T> this_future)
	{
		using R = ExtFuture_inner_t<ExtFutureR>;

		qDb() << "START ThrowDownstreamCancelsUpstream with downstream:" << ret_future << "upstream:" << this_future;

		// Ok, our job is to catch any cancel exceptions from the downstream ret_future
		// and rethrow them upstream to this_future.

		QtConcurrent::run([=]() mutable {
//			try
//			{
				// Wait for cancel or finish on downstream.
				qDb() << "IN TRY: WAITING FOR DOWNSTREAM FINISH/CANCEL, THREAD ID:" << QThread::currentThreadId();
				ret_future.waitForFinished();
				qDb() << "IN TRY: DOWNSTREAM FINISHED";
//			}
//			catch(ExtAsyncCancelException& e)
//			{
//				qDb() << "CAUGHT CANCEL EXCEPTION:";// << state(ret_future) << state(this_future);
//				throw;
//			}
//			catch(...)
//			{
//				qDb() << "CAUGHT NON-CANCEL EXCEPTION:";// << state(ret_future) << state(this_future);
//				throw;
//			}

				if(ret_future.isCanceled())
				{
					qDb() << "CAUGHT ISCANCELED:" << ret_future.state() << this_future.state();
					this_future.reportCanceled();
				}
				else
				{
					// Was a regular finish, ignore it.
					qDb() << "IGNORING REGULAR FINISH:";// << state(ret_future) << state(this_future);
				}

			;});
#if 0
		try
		{
			if(ret_future.isCanceled())
			{
				// Note that it appears we must use a watcher here since we have no other obvious means
				// by which to detect the cancellation of the downstream future, other than maybe calling
				// another ExtAsync::run() with a specialized callback.  Maybe for another day.

				// Per Simon Brunel, @link https://www.qpm.io/packages/com.github.simonbrunel.qtpromise/index.html
				// "A QFuture is canceled if cancel() has been explicitly called OR if an
				// exception has been thrown from the associated thread. Trying to call
				// result() in the first case causes a "read access violation", [GRVS: is this still true?] so let's
				// rethrown potential exceptions using waitForFinished() and thus detect
				// if the future has been canceled by the user or an exception."
qDb() << "Trying wait...";
				this->waitForFinished();
qDb() << "Finished waiting...";
				// If we fall through here, we didn't automatically rethrow, so downstream must have
				// had .cancel() called on it.  Throw a cancel exception upstream.
				throw ExtAsyncCancelException();
			}
			else
			{
qDb() << "Not canceled...";
				// Finished, not canceled.
				/// @todo Do we even need to do anything in this case?
			}

		}
		catch(...)
		{
			// reportException() here?
			qCr() << "Rethrowing exception from" << ret_future << "to" << this_future;
			throw;
		}
#endif

qDb() << "END ThrowDownstreamCancelsUpstream";
	}

	/// @name Additional member variables on top of what QFuture<T> has.
	/// These will cause us to need to worry about slicing, additional copy construction/assignment work
	/// which needs to be synchronized somehow, etc etc.
	/// @{

	int m_progress_unit { 0 /* == KJob::Unit::Bytes*/};

	/// @}
};

//template <class T>
//ExtFuture<T>::ExtFuture(const QFuture<void>& f) : ExtFuture<T>::BASE_CLASS(f) {}

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

/**
 * Convert any ExtFuture<T> to a QFuture<void>.
 */
template <typename T>
QFuture<void> qToVoidFuture(const ExtFuture<T> &future)
{
	return QFuture<void>(future.d);
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
template<typename T,
		 REQUIRES(!is_ExtFuture_v<T>)>
ExtFuture<typename std::decay_t<T>> make_ready_future(T&& value)
{
	ExtFuture<T> extfuture;

	extfuture.reportStarted();
	extfuture.reportResult(std::forward<T>(value));
	extfuture.reportFinished();

	return extfuture;
}

template <class T, class E,
		  REQUIRES(!is_ExtFuture_v<T>
		  && std::is_base_of_v<QException, E>)>
ExtFuture<typename std::decay_t<T>> make_exceptional_future(const E & exception)
{
	ExtFuture<T> extfuture;

	extfuture.reportStarted();
	extfuture.reportException(exception);
	extfuture.reportFinished();

	return extfuture;
}

/**
 * QDebug stream operator.
 */
template <typename T>
QDebug operator<<(QDebug dbg, const ExtFuture<T> &extfuture)
{
	QDebugStateSaver saver(dbg);

	dbg << "ExtFuture<T>( state=" << extfuture.state() << ", resultCount():" << extfuture.resultCount() << ")";

	return dbg;
}

/**
 * std::ostream stream operator (for gtest).
 */
template <typename T>
std::ostream& operator<<(std::ostream& outstream, const ExtFuture<T> &extfuture)
{
	outstream << "ExtFuture<T>( state=" << extfuture.state() << ", resultCount():" << extfuture.resultCount() << qUtf8Printable(toString(extfuture.state())) << ")";

	return outstream;
}

/// @name Explicit instantiations to try to get compile times down.
/// @{
extern template class ExtFuture<Unit>;
extern template class ExtFuture<bool>;
extern template class ExtFuture<int>;
extern template class ExtFuture<long>;
extern template class ExtFuture<std::string>;
extern template class ExtFuture<double>;
extern template class ExtFuture<QString>;
extern template class ExtFuture<QByteArray>;
/// @}

#endif /* SRC_CONCURRENCY_EXTFUTURE_H_ */

