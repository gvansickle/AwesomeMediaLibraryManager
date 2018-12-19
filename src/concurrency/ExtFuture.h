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

#pragma once
#ifndef SRC_CONCURRENCY_EXTFUTURE_H
#define SRC_CONCURRENCY_EXTFUTURE_H

/**
 * @file
 * An extended QFuture<T> class supporting .then() composition etc.
 */

// Std C++
#include <memory>
#include <atomic>
#include <type_traits>
#include <functional>

// Future Std C++
#include <future/future_type_traits.hpp>
#include <future/function_traits.hpp>
#include <future/cpp14_concepts.hpp>
#include <future/Unit.hpp>

// Qt5
#include <QtConcurrent>
#include <QFuture>
#include <QFutureInterface>
#include <QThread>
#include <QPair>

// Ours
#include <utils/QtHelpers.h>
#include <utils/ConnectHelpers.h>
#include <utils/StringHelpers.h>
#include <utils/DebugHelpers.h>
#include <utils/UniqueIDMixin.h>

#include "ExtFutureState.h"
#include "ExtFutureProgressInfo.h"
#include "ExtAsyncExceptions.h"

// Generated
#include "logging_cat_ExtFuture.h"


// Forward declare the ExtAsync namespace
namespace ExtAsync
{
namespace detail {}
}

template <class T>
class ExtFuture;

/// ExtFuture ID counter.
std::atomic_uint64_t get_next_id();

// Stuff that ExtFuture.h needs to have declared/defined prior to the ExtFuture<> declaration.
#include "ExtAsync_traits.h"

#if defined(TEMPL_ONLY_NEED_DECLARATION) || !defined(TEMPL_ONLY_NEED_DEF)

#include "impl/ExtFutureImplHelpers.h"

template <typename T>
ExtFuture<T> make_started_only_future();

/**
 * A C++2x-ish std::shared_future<>-like class implemented on top of Qt5's QFuture<T> and QFutureInterface<T> classes and other facilities.
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
 * - future()
 *
 * Future (consumer/reader) functionality:
 * - get() [std::shared_future::get()]
 * -- result() [QFuture]
 * -- results() [QFuture]
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
static_assert(std::is_class_v<QFuture<int>>);
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
	using inner_t = T;

	/**
	 * Default constructor.
	 *
	 * @param initialState  We default-construct to State(Started|Finished|Canceled) to match QFuture<T>'s default state.
	 * @note .waitForFinished() won't wait on a default-constructed future, thinks it's finished.
	 */
	explicit ExtFuture() : QFuture<T>(), m_extfuture_id_no{get_next_id()}	{ }

	/// Copy constructor.
	ExtFuture(const ExtFuture<T>& other) : QFuture<T>(&(other.d))
//			m_progress_unit(other.m_progress_unit)
	{ m_extfuture_id_no.store(other.m_extfuture_id_no); }

	/// Move constructor
	/// @note Qt5's QFuture doesn't have this.
	/// @note Neither implemented, deleted, or = default'ed.  Due to the vagaries of C++
	/// the latter actually causes the compiler to not fall back on the copy constructor when it needs
	/// to, but if we simply never mention it, the compiler does the right thing.
//	ExtFuture(ExtFuture<T>&& other) noexcept ...;

	/// Converting constructor from QFuture<T>.
	ExtFuture(const QFuture<T>& f, uint64_t id = 0) : ExtFuture(&(f.d)) { m_extfuture_id_no = id; }

	/// Move construct from QFuture.
	/// @note Neither implemented, deleted, or = default'ed.  Due to the vagaries of C++
	/// the latter actually causes the compiler to not fall back on the copy constructor when it needs
	/// to, but if we simply never mention it, the compiler does the right thing.
//	ExtFuture(QFuture<T>&& f) noexcept ...;

	/// Copy construct from QFuture<void>.
	/// @todo Needs to be static I think.

	/**
	 * Construct from a QFutureInterface<T> pointer.
	 *
	 * @note Quasi-internal, see QFuture<>()'s use.
	 *
	 * @param p   Pointer to a QFutureInterface<T> instance.
	 * @param id  Optional identifier for debugging.
	 */
	explicit ExtFuture(QFutureInterface<T> *p, uint64_t id = 0)
		: BASE_CLASS(p) { m_extfuture_id_no = id; }

	/**
	 * Unwrapping constructor, ala std::experimental::future::future, boost::future.
	 * @note Unimplemented, honeypot for catching nested ExtFuture<>s and asserting at compile time.
	 * Not sure if we really need this or not.
	 */
	template <class ExtFutureExtFutureT,
			  REQUIRES(NestedExtFuture<ExtFutureExtFutureT>)>
	explicit ExtFuture(ExtFuture<ExtFuture<T>>&& other)
	{
		Q_UNUSED(other);
		static_assert(NestedExtFuture<ExtFutureExtFutureT>, "Nested ExtFutures not supported");
	}

	/**
	 * Destructor.
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
	ExtFuture<T>& operator=(const ExtFuture<T>& other);

	/// Move assignment.
	/// @note Neither implemented, deleted, or = default'ed.  Due to the vagaries of C++
	/// the latter actually causes the compiler to not fall back on the copy constructor when it needs
	/// to, but if we simply never mention it, the compiler does the right thing.
//	ExtFuture<T>& operator=(ExtFuture<T>&& other) noexcept ...;

	/// Copy assignment from QFuture<T>.
	ExtFuture<T>& operator=(const BASE_CLASS& other);

	/// @}

	/**
	 * Conversion operator to QFuture<void>.
	 */
	explicit operator QFuture<void>() const
	{
		return QFuture<void>(&(this->d));
	}

	/**
	 * Conversion operator to ExtFuture<Unit>.
	 * @todo Can't get this to compile, can't figure out the template-laden error message.
	 */
//	explicit operator ExtFuture<Unit>() const
//	{
//		return ExtFuture<Unit>(&(this->d));
////		return qToUnitExtFuture(*this);
//	}

	/// @name Comparison operators.
	/// @{

	template <class U>
	bool operator==(const ExtFuture<U> &other) const
	{
		/// @todo Additional fields.
		return this->d == other.d;
	}

	template <class U>
	bool operator!=(const ExtFuture<U> &other) const
	{
		/// @todo Additional fields.
		return this->d != other.d;
	}

	template <class U>
	bool operator==(const QFuture<U> &other) const { return this->BASE_CLASS::operator==(other); }

	template <class U>
	bool operator!=(const QFuture<U> &other) const { return this->BASE_CLASS::operator!=(other); }

	/// @}

	/**
	 * @name Static members for the global state needed by ExtFuture.
	 */
	/// @{
	static void InitStaticExtFutureState();
	/// @}

	/// @name Extra informational accessors.
	/// @{

	/**
	 * C++2x is_ready().
	 * "Checks if the associated shared state is ready.  The behavior is undefined if valid() is false."
	 * (from @link https://en.cppreference.com/w/cpp/experimental/shared_future/is_ready).
	 * Same semantics as std::experimental::shared_future::is_ready().
	 *
	 * @return  true if the associated shared state is ready.
	 */
	bool is_ready() const
	{
		// I'm defining the undefined behavior if this isn't valid here as "assert".  You're welcome ISO.
		Q_ASSERT(this->valid() == true);
		// We're only C++2x-ready if we're Finished or Canceled (including Exceptions).
		return this->isFinished() || this->isCanceled();
	}

	/**
	 * C++2x valid().
	 * This sort of gets lost in translation.  Per @link https://en.cppreference.com/w/cpp/thread/shared_future/valid,
	 * we should be valid()==false if we've been:
	 * 1. Default constructed (and presumably never given a state via another method, e.g. assignment).
	 * 2. Moved from.
	 * 3. (std::experimental::future only) Invalidated by a call of .get().
	 * Our Qt 5 underpinnings don't support move semantics (anywhere AFAICT), which eliminates #2.  #3 doesn't apply
	 * since QFuture<T> etc. don't become invalid due to .get() or other results-access calls (again shared_future semantics).
	 * #1 is the only one I'm not 100% on.  We have a QFutureInterface<T> constructed beneath us in all cases, so per
	 * the definitions above, I don't think we're ever in an "invalid" state.  But default-constructed might be what should
	 * be considered invalid here.
	 *
	 * @returns  true if *this refers to a shared state, otherwise false.
	 */
	bool valid() const { return true; }

	/**
	 * Returns true if this ExtFuture<T> is sitting on an exception.  Does not cause any potential exception
	 * to be thrown.
	 *
	 * @note Does not acquire any locks.  Unclear if it should or not:
	 * - both ExceptionStore::throwPossibleException() and ExceptionStore::setException() calls its own hasException()
	 *   without any synchronization.
	 * - void QFutureInterfaceBase::waitForFinished() calls throwPossibleException() with QFutureInterfaceBasePrivate's
	 *   mutex locked.
	 * - QFutureInterfaceBase::waitForResult() calls throwPossibleException() without locking that same mutex.
	 */
	bool hasException() const
	{
		return this->d.exceptionStore().hasException();
	}

	/// @} // END  Extra informational accessors.

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
			try
			{
				/// @todo This could throw.
				this->waitForResume();
			}
			catch(...)
			{
				/// @todo
				Q_ASSERT_X(0, "catch", ".waitForResume() in pause threw");
			}
		}
		if (this->isCanceled())
		{
			// The job should be canceled.
			// The calling Functor should break out of its while() loop.
			return true;
		}
		else
		{
			return false;
		}
	}


	/// @name Results reporting interface.	From QFutureInterface<T>.
	/// @{

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

	/// @} // END Results reporting interface.

	/**
	 * If result is != nullptr, calls reportResult() and adds a copy of the result.
	 * Unconditionally reports finished.
	 * @param result
	 */
	inline void reportFinished(const T *result = nullptr)
	{
		this->d.reportFinished(result);
	}

	/// From QFutureInterfaceBase

	/**
	 * Acquires mutex.
	 * Returns immediately having taken no action if (state & (Started|Canceled|Finished)) == true.
	 * Else sets state = Started | Running, sends Started callout.
	 */
	void reportStarted()
	{
		/**
		 * RunFunctionTaskBase does this on start() (this derived from QFutureInterface<T>):
		 * this->reportStarted();
         * QFuture<T> theFuture = this->future();
         * pool->start(this, //m_priority// 0);
         * return theFuture;
		 */
		if(this->d.queryState(QFutureInterfaceBase::State(QFutureInterfaceBase::Started|QFutureInterfaceBase::Canceled|QFutureInterfaceBase::Finished)))
		{
			qWr() << "reportStarted() will be ignored, state:" << this->state();
		}
		this->d.reportStarted();
	}

	/**
	 * Calls this->d.cancel() (QFutureInterfaceBase::cancel()) which in turn:
	 * - Locks m_mutex
	 * - if state is Canceled already, return, having done nothing.
	 * - else switch state out of Paused and into Canceled.
	 * - Send QFutureCallOutEvent::Canceled.
	 * @note This is shadowing the same non-virtual function in QFuture<T>.
	 */
	void cancel()
	{
		// Same as what QFuture<>::cancel does.
		this->d.cancel();
	}

	/**
	 * Blocks until this future is finished or canceled.
	 * @note This is shadowing the same non-virtual function in QFuture<T>.
	 */
	void waitForFinished()
	{
		if(this->hasException())
		{
			qWr() << "waitForFinished() about to throw";
		}
		this->d.waitForFinished();
		if(this->hasException())
		{
			Q_ASSERT_X(0, "waitForFinished()", "ExtFuture held an exception but didn't throw");
		}
	}

	/**
	 * Simply calls QFutureInterfaceBase::reportCanceled(), which just calls cancel().
	 * QFutureInterfaceBase::cancel() in turn:
	 * - Locks m_mutex
	 * - if state is Canceled already, return, having done nothing.
	 * - else switch state out of Paused and into Canceled.
	 * - Send QFutureCallOutEvent::Canceled.
	 */
	void reportCanceled()
	{
		this->d.reportCanceled();
	}

	/**
	 * QFutureInterfaceBase::reportException():
	 * - Locks mutex.
	 * - Does nothing and returns if this future's state is (Canceled|Finished).
	 * - Stores exception in the shared state,
	 * - Switches state to Canceled.
	 * - Sends Canceled callout.
	 */
	void reportException(const QException &e)
	{
		if(this->d.queryState(QFutureInterfaceBase::State(QFutureInterfaceBase::Canceled|QFutureInterfaceBase::Finished)))
		{
			qWr() << "FUTURE ALREADY FINISHED OR CANCELED, EXCEPTION WILL BE IGNORED:" << state();
		}
		this->d.reportException(e);
	}

	/**
	 * Returns immediately if indices are equal or state is Canceled|Finished.
	 */
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
//		m_progress_unit = prog_unit;
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
	 * Will throw if this holds an exception.
	 *
	 * Essentially the same semantics as std::future::get(); shared_future::get() always returns a reference instead.
	 *
	 * @note Directly calls this->results().  This blocks any event loop in this thread.
	 *
	 * @return The results value of this ExtFuture.  As with std::shared_future::get() and QFuture's various .results(),
	 *         any stored exception referenced by this future will be thrown.
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
	 * @return A new future for containing the return value of @a then_callback.
	 */
	/**
	 * The root then() implementation.
	 * Takes a context QObject and a then_callback where then_callback's signature is:
	 * 	@code
	 * 		then_callback(ExtFuture<T>) -> R
	 * 	@endcode
	 * where R is a non-ExtFuture<> return value.
	 * If context == nullptr, then_callback will be run in an arbitrary thread.
	 * If context points to a QObject, then_callback will be run in its event loop.
	 *
	 * @note Canceling
	 *
	 * The returned future can be canceled, and the cancellation will propagate upstream (i.e. to this).
	 *
	 * @note Exceptions from callback
	 *
	 * Per std::experimental::shared_future::then() at @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
	 * "Any value returned from the continuation is stored as the result in the shared state of the returned future object.
	 *  Any exception propagated from the execution of the continuation is stored as the exceptional result in the shared
	 *  state of the returned future object."
	 *
	 * So we need to wrap the call to then_callback with a try/catch, and send any exceptions we catch
	 * to the returned future.
	 *
	 * @param call_on_cancel  If true, call the callback if the returned future is canceled.
	 *                        If false, don't call the callback, but handle cancellation internally.
	 */
	template <typename ThenCallbackType,
			  typename LiftedR = Unit::LiftT<std::invoke_result_t<ThenCallbackType, ExtFuture<T>>>,
			  REQUIRES(!is_ExtFuture_v<LiftedR>
			  && std::is_invocable_r_v<Unit::DropT<LiftedR>, ThenCallbackType, ExtFuture<T>>)>
	ExtFuture<LiftedR> then(QThreadPool* context, bool call_on_cancel, ThenCallbackType&& then_callback) const /** std .then() is const */
	{
		static_assert (!std::is_same_v<LiftedR, void>, "Callback return value should never be void");

		Q_ASSERT(context == nullptr); // Not yet implemented.
		if(context != nullptr)
		{
			// If non-null, make sure context has an event loop.
			QThread* ctx_thread = context->thread();
			Q_ASSERT(ctx_thread != nullptr);
			Q_ASSERT(ctx_thread->eventDispatcher() != nullptr);
		}

		// The future we'll immediately return.  We copy this into the then_callback ::run() context below.
		ExtFuture<LiftedR> returned_future = make_started_only_future<LiftedR>();
		/// @todo Use context.
		QtConcurrent::run(
//		returned_future = ExtAsync::run_for_then(
			[=, then_callback_copy = DECAY_COPY(then_callback)]
					(ExtFuture<T> this_future_copy, ExtFuture<LiftedR> returned_future_copy) -> void {

			// Ok, we're now running in the thread which will call then_callback_copy(this_future_copy).
			// At this point:
			// - The outer then() call may have already returned.
			// -- Hence returned_future, context may be gone off the stack.
			// - this may be have been destructed and deleted already.
			// - this_future_copy may or may not be running, finished, canceled, or canceled with exception,
			//   but should be in one of those states.
			// - returned_future_copy state may be:
			//    - Normal: Started, Canceled, or canceled with exception.
			//    - Abnormal: Finished, but you'd have to be trying.
			//   Would be something like this:
			//     f = ef.then(...haven't got inside here yet...);
			//     f.cancel().

			Q_ASSERT(returned_future_copy != this_future_copy);
			Q_ASSERT(this_future_copy.isRunning()
					 || this_future_copy.isCanceled()
					 || this_future_copy.isFinished());

			try
			{
				// We should never end up calling then_callback_copy with a non-finished future; this is the code
				// which will guarantee that.
				// This could throw a propagated exception from upstream (this_future_copy).
				// Per @link https://medium.com/@nihil84/qt-thread-delegation-esc-e06b44034698, we can't just use
				// this_future_copy.waitForFinished() here because it will return immediately if the thread hasn't
				// "really" started (i.e. if isRunning() == false).
				/// @todo Is that correct though?
//				if(!this_future_copy.isRunning())
				{
					qCWarning(EXTFUTURE) << "START SPINWAIT";
					// Blocks (busy-wait with yield) until one of the futures is canceled or finished.
					::spinWaitForFinishedOrCanceled(QThreadPool::globalInstance(), this_future_copy, returned_future_copy);
//					spinWaitForFinishedOrCanceled(this_future_copy);
					qWr() << "END SPINWAIT";
				}

				/// Spinwait is over, now we have four combinations to dispatch on.

				// Was downstream canceled?
				if(returned_future_copy.isCanceled())
				{
					// Downstream canceled, this is why we're here.
					qDb() << "returned_future_copy CANCELED:" << returned_future_copy;
					// Propagate cancel to this_future_copy.
					this_future_copy.reportCanceled();
				}
				else if(returned_future_copy.isFinished())
				{
					// Downstream is already Finished, but not canceled.
					// Not clear that this is a valid, or even possible, state here.
					qWr() << "returned_future_copy FINISHED?:" << returned_future_copy;
					Q_ASSERT_X(0, __func__, "Future returned by then() is finished first, shouldn't be possible.");
				}

				// Was this_future canceled?
				if(this_future_copy.isCanceled())
				{
					/// @todo Upstream canceled.
					qWr() << "this_future_copy CANCELED:" << this_future_copy.state();
//					Q_ASSERT(0); // SHOULD CANCEL DOWNSTREAM.
				}

				// Did this_future_copy Finish first?
				if(this_future_copy.isFinished())
				{
					// Normal finish of this_future_copy, no cancel or exception to propagate.
					qDb() << "THIS_FUTURE FINISHED NORMALLY";
				}

				// Now we're either Finished or Canceled, so we can call waitForFinished().  We now know
				// the state of isRunning() does reflect if we're done or not.
				// This call will block, or throw if an exception is reported to this_future_copy.
				this_future_copy.waitForFinished();

				// Ok, so now we're definitely finished and/or canceled.

				Q_ASSERT(this_future_copy.isFinished() || this_future_copy.isCanceled());

				// Got here, so we didn't throw.  We might be canceled.
			}
			// Handle exceptions and cancellation.
			// Exceptions propagate upwards, cancellation propagates downwards.
			catch(ExtAsyncCancelException& e)
			{
				qDb() << "CAUGHT CANCEL, CANCELING DOWSTREAM (RETURNED) FUTURE";
				returned_future_copy.cancel();
//				returned_future_copy.reportException(e);
				qDb() << "CAUGHT CANCEL, THROWING TO UPSTREAM (THIS) FUTURE";
				this_future_copy.reportException(e);
			}
			catch(QException& e)
			{
				qDb() << "CAUGHT EXCEPTION, CANCELING DOWSTREAM (RETURNED) FUTURE";
				returned_future_copy.cancel();
				qDb() << "CAUGHT EXCEPTION, THROWING TO UPSTREAM (THIS) FUTURE";
				this_future_copy.reportException(e);
			}
			catch (...)
			{
				qDb() << "CAUGHT UNKNOWN EXCEPTION, CANCELING DOWSTREAM (RETURNED) FUTURE";
				returned_future_copy.cancel();
				qDb() << "CAUGHT UNKNOWN EXCEPTION, THROWING TO UPSTREAM (THIS) FUTURE";
				this_future_copy.reportException(QUnhandledException());
			}

			// One last loose end.  If we get here, one or both of the futures may have been canceled by .cancel(), which
			// doesn't throw.  So:
			// - In run() we have nothing to do but return.
			// - In then() (here) we'll have to do the same thing we do for a cancel exception.
			if(returned_future_copy.isCanceled() || this_future_copy.isCanceled())
			{
				// if constexpr(in_a_then) { <below> };
				qDb() << "CANCELED, CANCELING DOWSTREAM (RETURNED) FUTURE" << returned_future_copy;
				returned_future_copy.cancel();
				qDb() << "CANCELED, THROWING TO UPSTREAM (THIS) FUTURE";
				this_future_copy.reportException(ExtAsyncCancelException());
			}

			//
			// The this_future_copy.waitForFinished() above either returned and the futures weren't canceled,
			// or may have thrown above and been caught and reported.
			//

			Q_ASSERT_X(this_future_copy.isFinished(), "then outer callback", "Should be finished here.");

			// Could have been a normal finish or a cancel.

			// Should we call the then_callback?
			// Always on Finished, maybe not if canceled.
			if(this_future_copy.isFinished() || (call_on_cancel && this_future_copy.isCanceled()))
			{
				///
				/// Ok, this_future_copy is finally finished and we can call the callback.
				///
				qDb() << "THEN: CALLING CALLBACK, this_future_copy:" << this_future_copy;

				try
				{
					// Call the callback with the results- or canceled/exception-laden this_future_copy.
					// Could throw, hence we're in a try.
					qDb() << "THENCB: Calling then_callback_copy(this_future_copy).";
					LiftedR retval;

//					if(context != nullptr)
//					{
//						// Call the callback in the context's event loop.
//						retval = run_in_event_loop(context, then_callback_copy);
//					}
//					else
					{
						if constexpr(std::is_same_v<LiftedR, Unit>)
						{
							// then_callback_copy returns void, return a Unit separately.
							qDb() << "INVOKING ret type == Unit";
							std::invoke(then_callback_copy, this_future_copy);
							retval = unit;
						}
						else
						{
							// then_callback_copy returns non-void, return the callback's return value.
							qDb() << "INVOKING ret type != Unit";
							retval = std::invoke(then_callback_copy, this_future_copy);
						}
						qDb() << "INVOKED";
					}
					// Didn't throw, report the result.
					returned_future_copy.reportResult(retval);
				}
				// One more time, Handle exceptions and cancellation, this time of the callback itself.
				// Exceptions propagate upwards, cancellation propagates downwards.
				catch(ExtAsyncCancelException& e)
				{
					qDb() << "CAUGHT CANCEL, CANCELING DOWSTREAM (RETURNED) FUTURE";
					returned_future_copy.cancel();
					qDb() << "CAUGHT CANCEL, THROWING TO UPSTREAM (THIS) FUTURE";
					this_future_copy.reportException(e);
				}
				catch(QException& e)
				{
					qDb() << "CAUGHT EXCEPTION, CANCELING DOWSTREAM (RETURNED) FUTURE";
					returned_future_copy.cancel();
					qDb() << "CAUGHT EXCEPTION, THROWING TO UPSTREAM (THIS) FUTURE";
					this_future_copy.reportException(e);
				}
				catch (...)
				{
					qDb() << "CAUGHT UNKNOWN EXCEPTION, CANCELING DOWSTREAM (RETURNED) FUTURE";
					returned_future_copy.cancel();
					qDb() << "CAUGHT UNKNOWN EXCEPTION, THROWING TO UPSTREAM (THIS) FUTURE";
					this_future_copy.reportException(QUnhandledException());
				}

				// One last loose end.  If we get here, one or both of the futures may have been canceled by .cancel(), which
				// doesn't throw.  So:
				// - In run() we have nothing to do but return.
				// - In then() (here) we'll have to do the same thing we do for a cancel exception.
				if(returned_future_copy.isCanceled() || this_future_copy.isCanceled())
				{
					// if constexpr(in_a_then) { <below> };
					qDb() << "CANCELED, CANCELING DOWSTREAM (RETURNED) FUTURE" << returned_future_copy;
					returned_future_copy.cancel();
					qDb() << "CANCELED, THROWING TO UPSTREAM (THIS) FUTURE";
					this_future_copy.reportException(ExtAsyncCancelException());
					/// @todo Should we throw out of the function here?  Above?
//					return;
				}
			}
			else if (call_on_cancel || !(this_future_copy.isFinished() || this_future_copy.isCanceled()))
			{
				// Something went wrong, we got here after .waitForFinished() returned or threw, but
				// the this_future_status isn't Finished or Canceled.
				Q_ASSERT_X(0, __PRETTY_FUNCTION__, ".waitForFinished() returned or threw, but this_future_status isn't Finished or Canceled");
			}
			else
			{
				// Not Finished, have to be Canceled.
				Q_ASSERT(this_future_copy.isCanceled());
			}

			/// See ExtAsync, again not sure if we should finish here if canceled.
			returned_future_copy.reportFinished();
			},
			*this,
			returned_future); //< Note copy by value of the two futures into the ::run().

		return returned_future;
	}

	template <typename ThenCallbackType,
				  typename R = Unit::LiftT<std::invoke_result_t<ThenCallbackType, ExtFuture<T>>>,
				  REQUIRES(!is_ExtFuture_v<R>
				  && ct::is_invocable_r_v<Unit::DropT<R>, ThenCallbackType, ExtFuture<T>>)>
	ExtFuture<R> then(QThreadPool* context, ThenCallbackType&& then_callback) const
	{
		// Forward to the master callback, don't call the then_callback on a cancel.
		return this->then(context, /*call_on_cancel==*/ false, std::forward<ThenCallbackType>(then_callback));
	}

	/**
	 * std::experimental::future-like .then() which takes a continuation function @a then_callback
	 * with the signature:
	 * 	@code
	 * 		R then_callback(ExtFuture<T> f)
	 * 	@endcode
	 * where neither R or T is an ExtFuture<>.
	 *
	 * When the shared state of *this future is ready, the then_callback continuation will be called with the
	 * finished ExtFuture f  *this.
	 *
	 * If then_callback() throws an exception, it will be stored in the ExtFuture<> returned by .then().
	 *
	 * @tparam R a non-ExtFuture<> type.
	 * @tparam T a non-ExtFuture<> type.
	 *
	 * @param then_callback
	 * @returns ExtFuture<R>  A future which will be made ready with the return value of then_callback.
	 */
	template <class ThenCallbackType, class R = Unit::LiftT<ct::return_type_t<ThenCallbackType>>,
			REQUIRES(is_non_void_non_ExtFuture_v<R>
			  && ct::is_invocable_r_v<Unit::DropT<R>, ThenCallbackType, ExtFuture<T>>)>
	ExtFuture<R> then( ThenCallbackType&& then_callback ) const
	{
		// then_callback is always an lvalue.  Pass it to the next function as an lvalue or rvalue depending on the type of ThenCallbackType.
		return this->then(nullptr /*QApplication::instance()*/, /*call_on_cancel==*/ false,
				std::forward<ThenCallbackType>(then_callback));
	}

	///
	/// @} // END .then() overloads.
	///

	/// @name .tap() overloads.
	/// @{

	/**
	 * Attaches a .tap() callback to this ExtFuture.
	 *
	 * The callback passed to tap() is invoked with individual results from this, of type T, as they become available.
	 *
	 * @param tap_callback  Callback with the signature void tap_callback(T).
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
	 * Attaches a non-streaming tap callback to this ExtFuture.
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
	 * Root .tap() overload for streaming taps.
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
		return this->StreamingTapHelper(context, std::forward<StreamingTapCallbackType>(tap_callback));
	}

	/**
	 * .tap() overload for streaming taps.
	 * Tap will run in the QApplication instance. @todo Not functional yet.
	 */
	template<typename StreamingTapCallbackType,
			 REQUIRES(ct::is_invocable_r_v<void, StreamingTapCallbackType, ExtFuture<T>, int, int>)>
	ExtFuture<T> tap(StreamingTapCallbackType&& tap_callback)
	{
		return this->tap(QApplication::instance(), std::forward<StreamingTapCallbackType>(tap_callback));
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
		ExtFuture<Unit> retval = this->then([=, finally_callback_copy = DECAY_COPY(finally_callback)](ExtFuture<T> this_future) -> Unit {
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

	/**
	 * Static function to get any future's current state as a ExtFutureState::State.
	 *
	 * @return A QFlags<>-derived type describing the current state of the ExtFuture.
	 */
	template <class FutureType>
	static ExtFutureState::State state(const FutureType& future);

protected:

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
		return StreamingTapHelper(guard_qobject, [=, tap_cb = DECAY_COPY(tap_callback)](ExtFuture<T> f, int begin, int end) {
			Q_ASSERT(f.isStarted());
//			Q_ASSERT(!f.isFinished());
			for(auto i = begin; i < end; ++i)
			{
				std::invoke(tap_cb, f.resultAt(i));
			}
		});
	}

	/**
	 * Helper for streaming taps.  Calls streaming_tap_callback with (ExtFuture<T>, begin_index, end_index) whenever
	 * the future has new results ready.
	 * @param guard_qobject  @todo Currently unused.
	 * @param streaming_tap_callback   callable with signature:
	 * @code
	 *  void streaming_tap_callback(ExtFuture<T>, int, int)
	 * @endcode
	 * @return
	 */
	template <typename StreamingTapCallbackType,
		REQUIRES(ct::is_invocable_r_v<void, StreamingTapCallbackType, ExtFuture<T>, int, int>)
		>
	ExtFuture<T> StreamingTapHelper(QObject *guard_qobject, StreamingTapCallbackType&& streaming_tap_callback)
	{
		/// @todo Use guard_qobject, should be QThreadPool* I think.
//		Q_ASSERT(guard_qobject == nullptr);

		// This is fundamentally similar to the .then() case in that the callback has to be called
		// with this_future in a non-blocking state.

		// The future we'll immediately return.  We copy this into the streaming_tap_callback's ::run() context.
		ExtFuture<T> returned_future = make_started_only_future<T>();

		// The concurrent run().
		QtConcurrent::run([=, streaming_tap_callback_copy = DECAY_COPY(streaming_tap_callback)]
						  (ExtFuture<T> this_future_copy, ExtFuture<T> returned_future_copy) {
				qDb() << "STREAMINGTAP: START ::RUN(), this_future_copy:" << this_future_copy
						<< "ret_future_copy:" << returned_future_copy;

				Q_ASSERT(returned_future_copy != this_future_copy);

				int i = 0;
				try
				{

					while(true)
					{
						qDb() << "STREAMINGTAP: Waiting for next result";

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
						/// @todo This needs to wait on both this_ and returned_ futures.
						this_future_copy.waitForResult(i);

						// Check if the wait failed to result in any results.
						int result_count = this_future_copy.resultCount();
						if(result_count <= i)
						{
							// No new results, must have finshed etc.
							qWr() << "STREAMINGTAP: NO NEW RESULTS, BREAKING, this_future:" << this_future_copy.state();
							break;
						}

						// Call the tap callback.
						//				streaming_tap_callback_copy(ef, i, result_count);
//						qDb() << "STREAMINGTAP: CALLING TAP CALLBACK, this_future:" << this_future_copy;
						std::invoke(streaming_tap_callback_copy, this_future_copy, i, result_count);

						// Copy the new results to the returned future.
						for(; i < result_count; ++i)
						{
							qDb() << "STREAMINGTAP: Next result available at i = " << i;

							T the_next_val = this_future_copy.resultAt(i);
							returned_future_copy.reportResult(the_next_val);
						}
					} // END while(true).

					qDb() << "STREAMINGTAP: LEFT WHILE(!Finished) LOOP, f0 state:" << this_future_copy;

					// Check final state.  We know it's at least Finished.
					/// @todo Could we be Finished here with pending results?
					/// Don't care as much on non-Finished cases.
					if(this_future_copy.isCanceled())
					{
						qDb() << "TAP: this_future cancelled:" << this_future_copy.state();
						returned_future_copy.reportCanceled();
					}
					else if(this_future_copy.isFinished())
					{
						qDb() << "TAP: ef finished:" << this_future_copy.state();
						returned_future_copy.reportFinished();
					}
					else
					{
						/// @todo Exceptions.
						qDb() << "NOT FINISHED OR CANCELED:" << this_future_copy.state();
						Q_ASSERT(0);
					}
				}
				catch(ExtAsyncCancelException& e)
				{
					/**
					 * Per std::experimental::shared_future::then() at @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
					 * "Any value returned from the continuation is stored as the result in the shared state of the returned future object.
					 *  Any exception propagated from the execution of the continuation is stored as the exceptional result in the shared
					 *  state of the returned future object."
					 */
					returned_future_copy.reportException(e);
				}
				catch(QException& e)
				{
					returned_future_copy.reportException(e);
				}
				catch (...)
				{
					returned_future_copy.reportException(QUnhandledException());
				}

				return returned_future_copy;
			}, // END lambda
			*this,
			returned_future); // END ::run() call.

		return returned_future;
	}

	/// @name Additional member variables on top of what QFuture<T> has.
	/// These will cause us to need to worry about slicing, additional copy construction/assignment work
	/// which needs to be synchronized somehow, etc etc.
	/// @{

public:
	std::atomic_uint64_t m_extfuture_id_no {0};

//	int m_progress_unit { 0 /* == KJob::Unit::Bytes*/};

	/// @}
};


/// @name Free functions
/// @{

namespace ExtAsync
{

template < class Sequence >
struct when_any_result
{
	std::size_t index;
	Sequence futures;
};

/**
 * C++2x when_all().
 * @tparam InputIterator
 * @param first
 * @param last
 * @return
 */
template <class InputIterator>
ExtFuture<std::vector<typename std::iterator_traits<InputIterator>::value_type>>
when_all(InputIterator first, InputIterator last);

template <class... Futures>
ExtFuture<std::tuple<std::decay_t<Futures>...>> when_all(Futures&&... futures);


} /// END namespace ExtAsync

/// @}


#endif // #if defined(TEMPL_ONLY_NEED_DECLARATION) || !defined(TEMPL_ONLY_NEED_DEF)

/// Definition

#if !defined(TEMPL_ONLY_NEED_DECLARATION) || defined(TEMPL_ONLY_NEED_DEF)

/// @name START ExtFuture member implementation
/// @{

#include "impl/ExtFuture_impl.hpp"

//template <typename T>
//explicit operator ExtFuture::ExtFuture<Unit>() const
//{
//	return ExtFuture<Unit>(&(this->d));
////		return qToUnitExtFuture(*this);
//}

template <typename T>
static ExtFutureState::State state(const QFuture<T>& qfuture_derived)
{
	/// @note .d is in fact private for QFuture<void>s.
	return ExtFutureState::state(qfuture_derived.d);
}

template<typename T>
static ExtFutureState::State state(const ExtFuture<T>& ef)
{
	return ef.state();
}

/// @} END ExtFuture member implementation

/**
 * Convert any ExtFuture<T> to a QFuture<void>.
 */
template <typename T>
QFuture<void> qToVoidFuture(const ExtFuture<T> &future)
{
	return QFuture<void>(&(future.d));
}

/**
 * Convert any ExtFuture<T> to an ExtFuture<Unit>.
 */
template <typename T>
ExtFuture<Unit> qToUnitExtFuture(const ExtFuture<T> &future)
{
	return ExtFuture<Unit>(future.d);
}

/**
 * Convert a QFuture<void> to an ExtFuture<Unit>.
 */
//template <typename T>
//ExtFuture<Unit> qToUnitExtFuture(const QFuture<void> &future)
//{
//	QFuture<Unit> temp;
//	temp = future;
//	return ExtFuture<Unit>(future.d);
//}

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
	QFutureInterface<T> qfi;

	qfi.reportStarted();
	qfi.reportResult(std::forward<T>(value));
	qfi.reportFinished();

	return 	ExtFuture<T>(&qfi, get_next_id());
}

template <class T, class E,
		  REQUIRES(!is_ExtFuture_v<T>
		  && std::is_base_of_v<QException, E>)>
ExtFuture<typename std::decay_t<T>> make_exceptional_future(const E & exception)
{
	QFutureInterface<T> qfi;

	qfi.reportStarted();
	qfi.reportException(exception);
	qfi.reportFinished();

	return ExtFuture<T>(&qfi, get_next_id());
}

/**
 * Helper which returns a (Started) ExtFuture<T>.
 */
template <typename T>
ExtFuture<T> make_started_only_future()
{
	// QFutureInterface<T> starts out with a state of NoState.
	QFutureInterface<T> fi;
	fi.reportStarted();
//	Q_ASSERT(ExtFutureState::state(fi) == ExtFutureState::Started) << state(fi);
	return ExtFuture<T>(&fi, get_next_id());
}

/**
 * QDebug stream operator.
 */
template <typename T>
QDebug operator<<(QDebug dbg, const ExtFuture<T> &extfuture)
{
	QDebugStateSaver saver(dbg);

	// .resultCount() does not cause a stored exception to be thrown.  It does acquire the mutex.
	dbg << "ExtFuture<T>( id=" << extfuture.m_extfuture_id_no << "state:" << extfuture.state() << ", resultCount():" << extfuture.resultCount() << ")";

	return dbg;
}

/**
 * std::ostream stream operator.
 */
template <typename T>
std::ostream& operator<<(std::ostream& outstream, const ExtFuture<T> &extfuture)
{
	// .resultCount() does not appear to cause a stored exception to be thrown.  It does acquire the mutex.
	outstream << "ExtFuture<T>( id=" << extfuture.m_extfuture_id_no << " state: " << extfuture.state() << ", resultCount(): " << extfuture.resultCount() << ")";

	return outstream;
}

#endif //#if !defined(TEMPL_ONLY_NEED_DECLARATION) || defined(TEMPL_ONLY_NEED_DEF)

/// @name Explicit instantiations to try to get compile times down.
/// @{
extern template class ExtFuture<Unit>;
extern template class ExtFuture<bool>;
extern template class ExtFuture<int>;
extern template class ExtFuture<long>;
extern template class ExtFuture<long long>;
extern template class ExtFuture<unsigned long>;
extern template class ExtFuture<unsigned long long>;
extern template class ExtFuture<std::string>;
extern template class ExtFuture<double>;
extern template class ExtFuture<QString>;
extern template class ExtFuture<QByteArray>;
/// @}

#endif /* SRC_CONCURRENCY_EXTFUTURE_H_ */

