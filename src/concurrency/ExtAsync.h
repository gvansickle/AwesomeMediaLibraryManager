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
#ifndef CONCURRENCY_EXTASYNC_H_
#define CONCURRENCY_EXTASYNC_H_

/**
 * @file
 * Some Qt5-based analogs to std::async().
 */


#include <config.h>

// Std C++
#include <type_traits>
#include <functional>
#include <memory>
#include <future>
#include <tuple> // For std::apply().

// Our Std C++ backfill
#include <future/future_type_traits.hpp>
#include <future/cpp14_concepts.hpp>
#include <future/function_traits.hpp>
#include <future/Unit.hpp>

// Qt5
#include <QEvent>
#include <QObject>
#include <QtConcurrent>
#include <QFutureInterface>
#include <QRunnable>
#include <QCoreApplication>

// Ours
#include "ExtFutureState.h"
#include "utils/DebugHelpers.h"
#include "utils/ConnectHelpers.h"

//template <typename T>
//class ExtFuture;
#include "ExtFuture.h"

//template <typename T>
//ExtFuture<T> make_started_only_future();

#include "impl/ExtFuture_make_xxx_future.h"

#include "impl/ExtAsync_impl.h"
#include "impl/ExtAsync_RunInThread.h"


// Generated
//#include "logging_cat_ExtAsync.h"

/**
 * A Qt5 analog to "C++ Extensions for Concurrency, ISO/IEC TS 19571:2016" and a million other similar
 * libraries floating around.  Ideas taken from all over:
 * - C++ TS mentioned above
 * - Facebook's Folly Futures
 * - Boost
 * - mhogomchungu's "tasks": https://github.com/mhogomchungu/tasks
 * - Simon Brunel's QtPromise: https://github.com/simonbrunel/qtpromise
 * - And many many other sources I can't even begin to remember.
 */

/**
 * This namespace and its run() functions are analogous to the run() function templates in the Qt5 header
 * /usr/include/qt5/QtConcurrent/qtconcurrentrun.h which live in the QtConcurrent namespace.
 */
namespace ExtAsync
{

	namespace detail
	{
		/**
		 * Wrapper helper object for run_in_qthread_with_event_loop().
		 */
		class WorkerQObject : public QObject
		{
			Q_OBJECT

		public:
			WorkerQObject() = default;
			~WorkerQObject() override = default;

		public:
			template <class CallbackType, class ExtFutureT, class... Args>
			void process(CallbackType&& callback, ExtFutureT future, Args&&... args)
			{
				std::invoke(callback, future, args...);
			};

		Q_SIGNALS:
			void resultReady();
			void finished();
			void error(QString err);

		private:
			// add your variables here
		};

	}; // END namespace detail

	template <class CallbackType>
	struct detail_struct
	{
		/**
		 * Root function for all ExtAsync::run() variants.
		 *
		 * @note Note the unusual manner in which we pass parameters to QtConcurrent::run().
		 * As of Qt5.11.1, QtConcurrent::run() can pass at most 5 params to the callback function, and it appears
		 * sometimes even fewer.  This manifests as inscrutable template deduction failures, hence the function's name.
		 * This is my almost-certainly futile attempt to Fight the Tool(tm), but it does seem to be working well.
		 *
		 * This is ultimately where all ExtAsync::run...() calls should end up.  Since we're trying to cleave to C++ standards
		 * as much as possible, the std::async() behavior we're trying to emulate is per @link https://en.cppreference.com/w/cpp/thread/async:
		 *
		 * "[this function] executes the callable object f on a new thread of execution (with all thread-locals initialized) as if
		 * spawned by std::thread(std::forward<F>(f), std::forward<Args>(args)...), except that if the function f returns a value
		 * or throws an exception, it is stored in the shared state accessible through the std::future that async returns
		 * to the caller."
		 *
		 * We're close to that here, except:
		 * - The callback is passed an ExtFuture<T> f as its first parameter.  All communication in or out of the callback
		 *   is through this future.
		 * - A copy of ExtFuture<T> f is returned by this function call.  This returned value should be used by the
		 *   caller for obtaining results and control of the asynchronous task.
		 * - The callback must return void.  Any return values must be passed directly to the ExtFuture<T> f via .reportResult() etc.
		 *
		 * @tparam ExtFutureT  The return type of the run_param_expander() call and of the ExtFuture
		 * 						passed to the callback function as the first parameter.
		 *
		 * @param callback  The function to be run on a thread in the threadpool.  Must be a function with signature:
		 *     @code
		 *     		void callback(ExtFuture<T> f [, args...])
		 *     @endcode
		 * @note The return value of the callback must be void. Any return values must be sent to @a f.
		 *
		 * @param args  Parameter pack of the arguments to pass to the callback.  Note that these will be passed
		 *              in the parameter list after the ExtFuture<T> f parameter, which is passed first.
		 * @returns A copy of the ExtFuture<T> passed to the callback.
		 */
		template <class ExtFutureT = argtype_t<CallbackType, 0>,
				class... Args,
				REQUIRES(is_ExtFuture_v<ExtFutureT> && !is_nested_ExtFuture_v<ExtFutureT>)>
		static ExtFutureT run_param_expander(QThreadPool* executor, CallbackType&& callback, Args&&... args)
		{
			static_assert(!std::is_base_of_v<QObject, decltype(*executor)> , "Executor was deduced incorrectly");

			// Create the ExtFuture<T> we'll be returning.
			ExtFutureT retfuture = make_started_only_future<typename ExtFutureT::inner_t>();

			// Capture the future and the variadic args into a tuple we'll pass to the lambda instead of passing them in the
			// limited QtConcurrent::run() parameter list.
			auto lambda = [
					executor = executor,
					callback_copy = DECAY_COPY(callback),
					retfuture_copy = std::forward<ExtFutureT>(retfuture),
					argtuple = std::make_tuple(std::forward<ExtFutureT>(retfuture), std::forward<Args>(args)...)]
					() mutable
			{

				try
				{
					// Call the callback with a copy of the retfuture and the args all as elements in argtuple.
					std::apply(callback_copy, argtuple);
				}
				// Handle exceptions and cancellation.
				// Exceptions propagate upwards, cancellation propagates downwards.
				/// @note That makes no sense.  At the "root" (i.e. the run() call, here),
				/// there is no upstream, only downstream (attached .then()'s etc).
				/// And any cancel could be coming from outside ("app's closing"), or possibly from
				/// inside the callback, though I can't think of an example of the latter ATM.
				/// The callback could throw for other reasons.
				catch(ExtAsyncCancelException& e)
				{
					qDb() << "CAUGHT CANCEL, CANCELING DOWSTREAM (RETURNED) FUTURE";
					retfuture_copy.cancel();
//					qDb() << "CAUGHT CANCEL, THROWING TO UPSTREAM (THIS) FUTURE";
//					retfuture_copy.reportException(e);
				}
				catch(QException& e)
				{
//					qDb() << "CAUGHT EXCEPTION, CANCELING DOWSTREAM (RETURNED) FUTURE";
//					retfuture_copy.cancel();
					qDb() << "CAUGHT EXCEPTION, THROWING TO DOWNSTREAM (RETURNED) FUTURE";
					retfuture_copy.reportException(e);
				}
				catch (...)
				{
//					qDb() << "CAUGHT UNKNOWN EXCEPTION, CANCELING DOWSTREAM (RETURNED) FUTURE";
//					retfuture_copy.cancel();
					qDb() << "CAUGHT UNKNOWN EXCEPTION, THROWING TO DOWSTREAM (RETURNED) FUTURE";
					retfuture_copy.reportException(QUnhandledException());
				}

				// One last loose end...?  Remember we're at the root of a potential tree of .then()-like
				// attachments.  At this point in the control flow, the returned future may have been canceled by:
				// - A call to this->cancel() from either outside, or
				// - A call to this->cancel() from the catch()'s above.
				// I don't think we need to propagate the cancel to the downstream attachments, since the
				// shared_future-like behavior should already be doing that.
				// - In run() (here) we have nothing to do but return.
				// - In then() we'll have to do the same thing we do for a cancel exception.
				if(retfuture_copy.isCanceled())
				{
					// if constexpr(in_a_then) { <below> };
//					qDb() << "CAUGHT CANCEL, CANCELING DOWSTREAM (RETURNED) FUTURE";
//					retfuture_copy.cancel();
//					qDb() << "CAUGHT CANCEL, THROWING TO UPSTREAM (THIS) FUTURE";
//					retfuture_copy.reportException(e);
					qWr() << "NON-THROWING CANCEL, RETURNING";
//					return;
				}

				// Even in the case of exception or cancelation, we need to reportFinished() or we just hang.
				retfuture_copy.reportFinished();
			};

			// Don't need to pass anything other than the lambda.
			QtConcurrent::run(lambda);

			return retfuture;
		}

		/**
		 * Takes a QObject as the executor parameter, runs in the event loop of the thread the QObject lives in.
		 * This is mainly for use in .then() for returning results to the GUI.
		 * @note Yeah, this could probably be merged with the above QThreadPool one, but I'm sick of fighting templates at the moment.
		 */
		template <class ExtFutureT = argtype_t<CallbackType, 0>,
				class... Args,
				REQUIRES(is_ExtFuture_v<ExtFutureT> && !is_nested_ExtFuture_v<ExtFutureT>)>
		static ExtFutureT run_param_expander(QObject* executor, CallbackType&& callback, Args&&... args)
		{
			static_assert(!std::is_convertible_v<QThreadPool*, decltype(executor)*>, "Executor was deduced incorrectly");

			// Create the ExtFuture<T> we'll be returning.
			ExtFutureT retfuture = make_started_only_future<typename ExtFutureT::inner_t>();

			// Capture the future and the variadic args into a tuple we'll pass to the lambda instead of passing them in the
			// limited QtConcurrent::run() parameter list.
			auto lambda = [
					callback_copy = DECAY_COPY(callback),
					retfuture_copy=std::forward<ExtFutureT>(retfuture),
					argtuple = std::make_tuple(std::forward<ExtFutureT>(retfuture), std::forward<Args>(args)...)]
					() mutable
			{

				try
				{
					// Call the callback with a copy of the retfuture and the args all as elements in argtuple.
					std::apply(callback_copy, argtuple);
				}
					// Handle exceptions and cancellation.
					// Exceptions propagate upwards, cancellation propagates downwards.
				catch(ExtAsyncCancelException& e)
				{
					qDb() << "CAUGHT CANCEL, CANCELING DOWSTREAM (RETURNED) FUTURE";
					retfuture_copy.cancel();
//					qDb() << "CAUGHT CANCEL, THROWING TO UPSTREAM (THIS) FUTURE";
//					retfuture_copy.reportException(e);
				}
				catch(QException& e)
				{
					qDb() << "CAUGHT EXCEPTION, CANCELING DOWSTREAM (RETURNED) FUTURE";
					retfuture_copy.cancel();
//					qDb() << "CAUGHT EXCEPTION, THROWING TO UPSTREAM (THIS) FUTURE";
//					retfuture_copy.reportException(e);
				}
				catch (...)
				{
					qDb() << "CAUGHT UNKNOWN EXCEPTION, CANCELING DOWSTREAM (RETURNED) FUTURE";
					retfuture_copy.cancel();
//					qDb() << "CAUGHT UNKNOWN EXCEPTION, THROWING TO UPSTREAM (THIS) FUTURE";
//					retfuture_copy.reportException(QUnhandledException());
				}

				// One last loose end.  If we get here, the returned future may have been canceled by .cancel(), which
				// doesn't throw.  So:
				// - In run() (here) we have nothing to do but return.
				// - In then() we'll have to do the same thing we do for a cancel exception.
				if(retfuture_copy.isCanceled())
				{
					// if constexpr(in_a_then) { <below> };
//					qDb() << "CAUGHT CANCEL, CANCELING DOWSTREAM (RETURNED) FUTURE";
//					retfuture_copy.cancel();
//					qDb() << "CAUGHT CANCEL, THROWING TO UPSTREAM (THIS) FUTURE";
//					retfuture_copy.reportException(e);
					qWr() << "NON-THROWING CANCEL, RETURNING";
					return;
				}

				/// @todo If exception should we actually be reporting finished?
				retfuture_copy.reportFinished();
			};

			// Don't need to pass anything other than the lambda.
//			QtConcurrent::run(lambda);
//M_WARNING("TODO: This won't block, seems like it should.  But maybe we're OK, neither does ::run()");
//M_WARNING("TODO: We do need to finish the returned future though.");
			ExtAsync::detail::run_in_event_loop(executor, lambda);

			return retfuture;
		}

		/**
		 * Overload which doesn't take an explicit executor parameter.  Will be run in the global thread pool.
		 *
		 * @tparam ExtFutureT
		 * @tparam Args
		 * @param callback
		 * @param args
		 * @return
		 */
		template <class ExtFutureT = argtype_t<CallbackType, 0>,
				    class... Args,
				    REQUIRES(is_ExtFuture_v<ExtFutureT> && !is_nested_ExtFuture_v<ExtFutureT>)>
		static ExtFutureT run_param_expander(CallbackType&& callback, Args&&... args)
		{
			return run_param_expander(QThreadPool::globalInstance(), std::forward<CallbackType>(callback), std::forward<Args>(args)...);
		};



		/**
		 * Run the callback in a new QThread with its own event loop.
		 * @tparam CallbackType  Callback of type:
		 *     @code
		 *     void callback(ExtFutureT [, ...])
		 *     @endcode
		 * @param callback  The callback to run in the new thread.
		 */
		template <class ExtFutureT = argtype_t<CallbackType, 0>,
				class... Args,
				REQUIRES(is_ExtFuture_v<ExtFutureT> && !is_nested_ExtFuture_v<ExtFutureT>)>
		static ExtFutureT run_in_qthread_with_event_loop(CallbackType&& callback, Args&&... args)
		{
			using ExtAsync::detail::WorkerQObject;

			ExtFutureT retfuture = make_started_only_future<typename ExtFutureT::inner_t>();

			QThread* thread = new QThread;
			WorkerQObject* worker = new WorkerQObject();
			// Move this worker QObject to the new thread.
			worker->moveToThread(thread);

			// Connect to the worker's error signal.
			connect_or_die(worker, &WorkerQObject::error, thread, [=](QString errorstr){
				qCr() << "WorkerQObject reported error:" << errorstr;
			});
			// Connection from thread start to actual WorkerQObject process() function start
			connect_or_die(thread, &QThread::started, worker, [=,
						   callback_copy = DECAY_COPY(std::forward<CallbackType>(callback)),
						   retfuture_copy = retfuture,
						   argtuple = std::make_tuple(worker, std::forward<decltype(callback)>(callback), std::forward<ExtFutureT>(retfuture), std::forward<Args>(args)...)
							]() mutable {
				/// @todo Exceptions/cancellation.
				worker->process(callback_copy, retfuture_copy, args...);
//				std::apply(&WorkerQObject::process<CallbackType, ExtFutureT, decltype(args)...>, argtuple);
				/// @note Unconditional finish here.
				retfuture_copy.reportFinished();
			});
			// When the worker QObject is finished, tell the thread to quit, and register the worker to be deleted.
			connect_or_die(worker, &WorkerQObject::finished, thread, &QThread::quit);
			connect_or_die(worker, &WorkerQObject::finished, worker, &QObject::deleteLater);
			// Connect the QThread's finished signal to the deleteLater() slot so that it gets scheduled for deletion.
			/// @note I think this connection allows us to ignore the thread once we've started it, and it won't leak.
			connect_or_die(thread, &QThread::finished, thread, &QThread::deleteLater);
			// Start the new thread.
			thread->start();

			return retfuture;
		};
	}; // END struct detail_struct

	static std::atomic_int64_t s_qthread_id {0};

	static inline void name_qthread()
	{
		int64_t id = ++s_qthread_id;
		QThread::currentThread()->setObjectName(QString("%1_").arg(id) + QThread::currentThread()->objectName() );
	};

	/**
	 * ExtAsync::run() overload for member functions taking an ExtFuture<T> as the first non-this param.
	 * E.g.:
	 * 		void Class::Function(ExtFuture<T> future, Type1 arg1, Type2 arg2, [etc..]);
	 *
	 * @returns The ExtFuture<T> passed to @a function.
	 */
	template <typename This, typename F,
			  class ExtFutureR, // = typename function_traits<F&&>::template arg_t<1>,//std::tuple_element_t<1, ct::args_t<F>>,
			  typename... Args,
//			  class R = std::remove_reference_t<std::tuple_element_t<1, ct::args_t<F>>>,
		REQUIRES(std::is_class_v<This>
			  && ct::has_void_return_v<F>
			  && ct::is_invocable_r_v<void, F, This*, Args...>) //(arity_v<F> == 2))
			  >
	ExtFutureR run(This* thiz, F&& function, Args&&... args)
#if 0
	{
		return ExtAsync::qthread_async_with_cnr_future(function, args...);
	}
#else
	{
        /// @todo TEMP this currently limits the callback to look like C::F(ExtFuture<>&).
        static_assert(sizeof...(Args) <= 1, "Too many extra args given to run() call");
        static_assert(sizeof...(Args) == 0, "TODO: More than 0 args passed to run() call");

        constexpr auto calback_arg_num = arity_v<F>;
        STATIC_PRINT_CONSTEXPR_VAL(calback_arg_num);
        static_assert(calback_arg_num == 1, "Callback function takes more or less than 1 parameter");

        // Get a std::tuple<> containing the types of all args of F.
		using argst = ct::args_t<F>;
        /// @todo TEMP debug restriction
		static_assert(std::tuple_size_v<argst> != 1, "Callback function takes more or less than 1 parameter");
        // Extract the type of the first non-this arg of function, which should be an ExtFuture<?>&.
        using arg1t = std::tuple_element_t<1, argst>;
//		using ExtFutureR = std::remove_reference_t<arg1t>;

		qWr() << "EXTASYNC::RUN: IN ExtFuture<R> run(This* thiz, F&& function, Args&&... args):" << __PRETTY_FUNCTION__;

		static_assert(function_traits<F>::arity_v > 1, "Callback must take at least one argument");

		// ExtFuture<> will default to (STARTED | RUNNING).  This is so that any calls of waitForFinished()
		// against the ExFuture<> (and/or the underlying QFutureInterface<>) will block.
		ExtFutureR report_and_control = make_started_only_future<ExtFutureR::value_type_t>();

		QtConcurrent::run(std::forward<This*>(thiz), std::forward<F>(std::decay_t<F>(function)),
						  report_and_control, std::forward<Args>(args)...);

		return report_and_control;
	}
#endif

	template <class CallbackType,
	        class ExtFutureT = argtype_t<CallbackType, 0>,
			class... Args,
			REQUIRES(is_ExtFuture_v<ExtFutureT> && !is_nested_ExtFuture_v<ExtFutureT>)>
	static ExtFutureT run_in_qthread_with_event_loop(CallbackType&& callback, Args&&... args)
	{
		return ExtAsync::detail_struct<CallbackType>::run_in_qthread_with_event_loop(
				std::forward<CallbackType>(callback), std::forward<Args>(args)...
				);
	}

#if 1 /// @todo obsolete this?  Used by AMLMJobT::start().
    /**
     * ExtAsync::run() overload for member functions of classes derived from AMLMJob taking zero params.
     * E.g.:
	 * 		void AMLMJobTDerivedClass::Function();
     *
	 * @returns An ExtFuture<>.
     */
	template <typename This, typename F = void (This::*)(),
		REQUIRES(std::is_class_v<This>
//			  && std::is_member_function_pointer_v<&This::F>
			  && ct::is_invocable_r_v<void, F, This*>)>
	typename This::ExtFutureType run_class_noarg(This* thiz, F function)
    {
        constexpr auto calback_arg_num = arity_v<F>;
//		STATIC_PRINT_CONSTEXPR_VAL(calback_arg_num);
        static_assert(calback_arg_num == 1, "Callback function takes more or less than 1 parameter");

        qIn() << "EXTASYNC::RUN: IN :" << __PRETTY_FUNCTION__;

		QtConcurrent::run(thiz, function); //std::forward<F>(std::decay_t<F>(function)));
        qIn() << "AFTER QtConcurrent::run(), thiz:" << thiz << "ExtFuture state:" << ExtFutureState::state(thiz->get_extfuture());

        return thiz->get_extfuture();
    }
#endif

	/**
     * Asynchronously run a free function of the form:
	 * @code
	 * 	void Function(ExtFuture<T> control_and_reporting_future, [Type1 arg1, Type2 arg2, etc..]);
	 * @endcode
	 * Creates and returns a copy of control_and_reporting_future.
	 */
	template<class CallbackType,
			 class ExtFutureT = argtype_t<CallbackType, 0>, //std::tuple_element_t<0, ct::args_t<CallbackType>>,
			 class T = typename isExtFuture<ExtFutureT>::inner_t,
			 class... Args,
			 REQUIRES(is_ExtFuture_v<ExtFutureT>
			 && std::is_invocable_r_v<void, CallbackType, ExtFutureT, Args&&...>)
             >
	ExtFuture<T> run(CallbackType&& callback, Args&&... args)
    {
#if 1
		return ExtAsync::qthread_async_with_cnr_future(FWD_DECAY_COPY(CallbackType, callback), std::forward<Args>(args)...);
#else
		using argst = ct::args_t<CallbackType>;
		using arg0t = std::tuple_element_t<0, argst>;
		using ExtFutureR = std::remove_reference_t<arg0t>;
		static_assert(std::is_same_v<ExtFutureT, ExtFutureR>);

		return ExtAsync::detail_struct<CallbackType>::run_param_expander(std::forward<CallbackType>(callback), std::forward<Args>(args)...);
#endif
    }

	/**
	 * Asynchronously run @p callback, which must be of the form:
	 * @code
	 * 	R callback(ExtFuture<T>, [args...])
	 * @endcode
	 *	Where R may be void.
	 *
	 * @p callback will be run via QtConcurrent::run() like so:
	 * @code
	 *    QtConcurrent::run(lambda, retfuture, std::forward<Args>(args)...);
	 * @endcode
	 *
	 * Where lambda holds callback and handles cancellation and exception propagation.
	 *
	 * @param callback
	 * @return ExtFuture<LiftedR> where LiftedR is the return type of callback if non-void, or Unit if it is void.
	 */
	template <class CallbackType, class... Args,
			  class ExtFutureT = argtype_t<CallbackType, 0>,
			  class LiftedR = Unit::LiftT<std::invoke_result_t<CallbackType, ExtFutureT, Args&&...>>,
			  REQUIRES(is_ExtFuture_v<ExtFutureT>
			  && NonNestedExtFuture<ExtFutureT>
			  && std::is_invocable_r_v<Unit::DropT<LiftedR>, CallbackType, ExtFutureT, Args&&...>)
	>
	ExtFuture<LiftedR> run_again(CallbackType&& callback, Args&&... args)
	{
		return ExtAsync::detail_struct<CallbackType>::run_param_expander(std::forward<CallbackType>(callback), std::forward<Args>(args)...);
	}


	/**
	 * Asynchronously run a free function taking no params and returning non-void/non-ExtFuture<>.
	 *
	 * @param function  An invocable with the signature "R function(void)".
	 * @return
	 */
	template <typename CallableType, typename R = std::invoke_result_t<CallableType, void>,
		REQUIRES(is_non_void_non_ExtFuture_v<R> // Return type is not void or ExtFuture<>
			  && std::is_invocable_r_v<R, CallableType, void> // F has signature R F(void).
			  )>
	auto run_zero_params(CallableType&& function) -> ExtFuture<R>
	{
		ExtFuture<R> retfuture = ExtAsync::make_started_only_future<R>();

		/*
		 * @see SO: https://stackoverflow.com/questions/34815698/c11-passing-function-as-lambda-parameter
		 *      As usual, C++ language issues need to be worked around, this time when trying to capture @a function
		 *      in the inner lambda.
		 *      Bottom line is that the variable "function" can't be captured by a lambda (or apparently stored at all)
		 *      unless we decay off the reference-ness.
		 */

		QtConcurrent::run([fn=std::decay_t<CallableType>(function)](ExtFuture<R> retfuture) -> void {
	    	R retval;

			try
			{
				// Call the function the user originally passed in.
				retval = std::invoke(fn);
				// Report our single result.
				retfuture.reportResult(retval);
			}
			catch(ExtAsyncCancelException& e)
			{
				/**
				 * Per std::experimental::shared_future::then() at @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
				 * "Any value returned from the continuation is stored as the result in the shared state of the returned future object.
				 *  Any exception propagated from the execution of the continuation is stored as the exceptional result in the shared
				 *  state of the returned future object."
				 */
				retfuture.reportException(e);
				// I think we don't want to rethrow like this here.  This will throw to the wrong future
				// (the QtConcurrent::run() retval, see above.
				//throw;
			}
			catch(QException& e)
			{
				retfuture.reportException(e);
			}
			catch (...)
			{
				retfuture.reportException(QUnhandledException());
			}
			retfuture.reportFinished();

		}, std::forward<ExtFuture<R>>(retfuture));

	    return retfuture;
	}

#if 1 // OBSOLETE
    /**
     * Asynchronously run a free function taking no params we care about here, arbitrary params otherwise,
     * and returning non-void/non-ExtFuture<>.
     *
	 * @warning This is really no better than using QtConcurrent::run() in that the returned future
	 *          isn't cancelable.  Provided mainly for completeness.
	 *
	 * @param function  Callable, R function(Args...)
     * @return
     */
	template <typename CallbackType, class... Args, typename R = std::invoke_result_t<CallbackType, Args...>,
        REQUIRES(!std::is_member_function_pointer_v<CallbackType>
			  && (sizeof...(Args) > 0)
			  && is_non_void_non_ExtFuture_v<R>
			&& (arity_v<CallbackType> > 0)
			&& std::is_invocable_r_v<R, CallbackType, Args&&...>)
        >
	ExtFuture<R> run(CallbackType&& function, Args&&... args)
    {
M_WARNING("OBSOLETE");
		ExtFuture<R> retfuture = ExtAsync::make_started_only_future<R>();

		/**
		 * @todo Exception handling.
		 * This is the basic pattern.  Note the use of reportException():
		 * 		this->m_task->run(*this);
					} catch (QException &e) {
						QFutureInterface<T>::reportException(e);
					} catch (...) {
						QFutureInterface<T>::reportException(QUnhandledException());
					}
		 *
		 * Per QException docs: @link http://doc.qt.io/qt-5/qexception.html
		 * "When using QFuture, transferred exceptions will be thrown when calling the following functions:
    			QFuture::waitForFinished()
    			QFuture::result()
    			QFuture::resultAt()
    			QFuture::results()
		 * "
		 */

		try
		{
			/*
			 * @see SO: https://stackoverflow.com/questions/34815698/c11-passing-function-as-lambda-parameter
			 *      As usual, C++ language issues need to be worked around, this time when trying to capture @a function
			 *      in the inner lambda.
			 *      Bottom line is that the variable "function" can't be captured by a lambda (or apparently stored at all)
			 *      unless we decay off the reference-ness.
			 */
			 retfuture = QtConcurrent::run([fn=std::decay_t<CallbackType>(function)](auto... copied_args_from_run) {
				R retval;
				// Call the function the user originally passed in.
				retval = std::invoke(fn, copied_args_from_run...);//std::forward<Args&&>(args)...);
				// Report our single result.
	//            retfuture.reportResult(retval);
	//            retfuture.reportFinished();
				return retval;
				}, std::forward<Args>(args)...);
		}
		catch(ExtAsyncCancelException& e)
		{
			/**
			 * Per std::experimental::shared_future::then() at @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
			 * "Any value returned from the continuation is stored as the result in the shared state of the returned future object.
			 *  Any exception propagated from the execution of the continuation is stored as the exceptional result in the shared
			 *  state of the returned future object."
			 */
			qDb() << "RETHROWING CANCEL EXCEPTION";
			retfuture.reportException(e);
			// I think we don't want to rethrow like this here.  This will throw to the wrong future
			// (the QtConcurrent::run() retval, see above.
			//throw;
		}
		catch(QException& e)
		{
			retfuture.reportException(e);
		}
		catch (...)
		{
			retfuture.reportException(QUnhandledException());
		}
		retfuture.reportFinished();

		return retfuture;
    }
};
#endif

/**
 * Run a functor on another thread.
 * Works by posting a message to @a obj's thread's event loop, and running the functor in the event's destructor.
 *
 * Functionally equivalent to something like:
 * QMetaObject::invokeMethod(this->m_current_libmodel, "onIncomingFilename", Q_ARG(QString, s));
 *
 *
 * Adapted from https://stackoverflow.com/questions/21646467/how-to-execute-a-functor-or-a-lambda-in-a-given-thread-in-qt-gcd-style/21653558#21653558
 */
template <typename F>
static void runInObjectEventLoop(F && fun, QObject * obj = qApp)
{
   struct Event : public QEvent
   {
      using Fun = typename std::decay<F>::type;
      Fun fun;
      Event(Fun && fun) : QEvent(QEvent::None), fun(std::move(fun)) {}
      Event(const Fun & fun) : QEvent(QEvent::None), fun(fun) {}
      ~Event() override { fun(); }
   };
   QCoreApplication::postEvent(obj, new Event(std::forward<F>(fun)));
}

/**
 * Run a member function or slot in another thread.
 * Works by posting a message to @a obj's thread's event loop, and running the functor in the event's destructor.
 *
 * Functionally equivalent to something like:
 * QMetaObject::invokeMethod(this->m_current_libmodel, "onIncomingFilename", Q_ARG(QString, s));
 *
 * @note Adapted from https://stackoverflow.com/questions/21646467/how-to-execute-a-functor-or-a-lambda-in-a-given-thread-in-qt-gcd-style/21653558#21653558
 *
 * @param obj     The QObject in whose thread's event loop to run @a method.
 * @param method  The member function or slot of @a obj to run.
 */
template <typename T, typename R>
static void runInObjectEventLoop(T * obj, R(T::* method)()) {
   struct Event : public QEvent {
      T * obj;
      R(T::* method)();
      Event(T * obj, R(T::*method)()):
         QEvent(QEvent::None), obj(obj), method(method) {}
      ~Event() override { (obj->*method)(); }
   };
   QCoreApplication::postEvent(obj, new Event(obj, method));
}

/// Above is pre-Qt5.10.  ExtAsync::detail::run_in_event_loop() should be used from Qt5.10+.



#endif /* CONCURRENCY_EXTASYNC_H_ */
