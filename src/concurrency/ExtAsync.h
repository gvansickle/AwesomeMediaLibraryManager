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

#ifndef UTILS_CONCURRENCY_EXTASYNC_H_
#define UTILS_CONCURRENCY_EXTASYNC_H_

/**
 * @file
 * Qt5 analogs to std::async().
 */

#pragma once

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

template <typename T> class ExtFuture;

#include "impl/ExtAsync_impl.h"

#include "ExtAsyncTask.h"


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
 * A note on QtConcurrent::run():
 * As of Qt5.11.1, QtConcurrent::run() can pass at most 5 params to the callback function, and it appears
 * sometimes even fewer.  This manifests as inscrutable template deduction failures.
 */

/**
 * This namespace and its run() functions are analogous to the run() function templates in the Qt5 header
 * /usr/include/qt5/QtConcurrent/qtconcurrentrun.h which live in the QtConcurrent namespace.
 */
namespace ExtAsync
{

template <class CallbackType>
	struct detail_struct
	{
		/**
		 * QtConcurrent::run() parameter expander.
		 * As of Qt5.11.1, QtConcurrent::run() can pass at most 5 params to the callback function, and it appears
		 * sometimes even fewer.  This manifests as inscrutable template deduction failures.
		 * This is my almost-certainly futile attempt to Fight the Tool(tm).
		 *
		 * @tparam Callback  Callback function with signature "void callback(ExtFuture<T> f [, args...])".
		 */
		template <class ExtFutureT = argtype_t<CallbackType, 0>,
				  class... Args,
				  REQUIRES(is_ExtFuture_v<ExtFutureT>
				  && !is_nested_ExtFuture_v<ExtFutureT>)>
		static ExtFutureT run_param_expander(CallbackType&& callback, Args&&... args)
		{
			ExtFutureT retfuture;

			// Capture the future and the variadic args into a tuple we'll pass to the lambda instead of passing them in the
			// limited QtConcurrent::run() parameter list.
			auto lambda = [&,
					callback_copy=/*std::decay_t*/std::forward<CallbackType>(callback),
					retfuture_copy=std::forward/*decay_t*/<ExtFutureT>(retfuture),
					argtuple = std::make_tuple(std::forward<ExtFutureT>(retfuture), std::forward<Args>(args)...)]
					() mutable {

				try
				{
					// Call the callback with a copy of the retfuture and the args all as elements in argtuple.
					std::apply(callback_copy, argtuple);
				}
				// Send any exceptions downstream to the returned future.
				catch(ExtAsyncCancelException& e)
				{
					qDb() << "CAUGHT CANCEL";
					retfuture_copy.reportException(e);
				}
				catch(QException& e)
				{
					retfuture_copy.reportException(e);
				}
				catch (...)
				{
					retfuture_copy.reportException(QUnhandledException());
				}

				/// @todo If exception should we actually be reporting finished?
				retfuture_copy.reportFinished();
			};

			// Don't need to pass anything other than the lambda.
			QtConcurrent::run(lambda);

			return retfuture;
		}

		/**
		 * ExtAsync::run() helper.
		 *
		 * @param callback  Callback function which will be run in the thread pool.
		 * 					Must have this form:
		 * 					@code
		 * 						R callback(ExtFutureT, args...)
		 * 					@endcode
		 * 					Where:
		 * 						ExtFutureT == the extracted first arg of callback, must be a non-nested ExtFuture.
		 * 						R == Unit::LiftT<> of return value of callback.  I.e. void == Unit, all other types the same.
		 * @param args      Optional additional arguments to pass to callback after the ExtFutureT.
		 * @return
		 */
		template<class... Args,
				 class ExtFutureT = argtype_t<CallbackType, 0>,
				 class LiftedR = Unit::LiftT<std::invoke_result_t<CallbackType, ExtFutureT, Args...>>,
				 REQUIRES(is_ExtFuture_v<ExtFutureT>
					&& NonNestedExtFuture<ExtFutureT>
				 && ct::is_invocable_r_v<LiftedR, CallbackType, ExtFutureT, Args...>)
				 >
		static auto run_again(CallbackType callback, Args... args) -> ExtFuture<LiftedR>
		{
			static_assert(!std::is_same_v<LiftedR, void>, "Should never get here with LiftedR == void");

			/**
			 * @note Exception handling.
			 * This is the basic pattern used in multiple ::run()-likes in Qt5.  Note the use of
			 * reportException() on this, which is in these cases the same as the returned future.
			 * We're doing Qt5 one better here, so we'll have to propagate to the control/return future
			 * passed to the callback.
			 * @code
			 *      try {
			 * 		this->m_task->run(*this);
						} catch (QException &e) {
							QFutureInterface<T>::reportException(e);
						} catch (...) {
							QFutureInterface<T>::reportException(QUnhandledException());
						}
			 * @endcode
			 *
			 * Per QException docs: @link http://doc.qt.io/qt-5/qexception.html
			 * "When using QFuture, transferred exceptions will be thrown when calling the following functions:
	    			QFuture::waitForFinished()
	    			QFuture::result()
	    			QFuture::resultAt()
	    			QFuture::results()
			 * "
			 */

			using ExtFutureR = ExtFuture<LiftedR>;

			ExtFutureR retfuture;

			/*
			 * @see SO: https://stackoverflow.com/questions/34815698/c11-passing-function-as-lambda-parameter
			 *      As usual, C++ language issues need to be worked around, this time when trying to capture @a function
			 *      in the inner lambda.
			 *      Bottom line is that the variable "function" can't be captured by a lambda (or apparently stored at all)
			 *      unless we decay off the reference-ness.
			 */


			auto lambda = [=, callback_copy=std::decay_t<CallbackType>(callback),
					check_retfuture=retfuture]
					(ExtFuture<LiftedR> retfuture_copy, auto... copied_args_from_run) mutable -> void
			{
				LiftedR retval;

				//			static_assert(sizeof...(copied_args_from_run) != 0);
				static_assert(!std::is_same_v<LiftedR, void>, "Should never get here with decltype(retval) == void");


				try
				{
					Q_ASSERT(check_retfuture == retfuture_copy);

					// Call the function the user originally passed in.
					retval = std::invoke(callback_copy, retfuture_copy, copied_args_from_run...);

					// Report our single result.
					retfuture_copy.reportResult(retval);
				}
				// Send any exceptions down to the returned future.
				catch(ExtAsyncCancelException& e)
				{
					retfuture_copy.reportException(e);
				}
				catch(QException& e)
				{
					retfuture_copy.reportException(e);
				}
				catch (...)
				{
					retfuture_copy.reportException(QUnhandledException());
				}

				/// @todo If exception should we actually be reporting finished?
				retfuture_copy.reportFinished();

			};

			// Run the callback, wrapped in the lambda above, concurrently.
			// We're ignoring the QFuture<> returned by ::run() here, since retfuture is
			// handling that job for us, better.
			QtConcurrent::run(lambda, retfuture, std::forward<Args>(args)...);

			return retfuture;
		} // END detail_struct::run_again()

#if 0
		/**
		 * ExtAsync::run() helper for ExtFuture<>::then().
		 *
		 * @param callback  Callback function which will be run in the thread pool.
		 * 					Must have this form:
		 * 					@code
		 * 						R callback(ExtFutureThis, ExtFutureReturn, args...)
		 * 					@endcode
		 * 					Where:
		 * 						ExtFutureThis == the extracted first arg of callback, must be a non-nested ExtFuture.
		 * 						ExtFutureReturn == the extracted second arg of callback, must be a non-nested ExtFuture.
		 * 						R == Unit::LiftT<> of return value of callback.  I.e. void == Unit, all other types the same.
		 * @param args      Optional additional arguments to pass to callback after the ExtFutureT.
		 * @return
		 */
		template<class... Args,
				 class ExtFutureT = argtype_t<CallbackType, 0>,
				 class ExtFutureR = argtype_t<CallbackType, 1>,
				 class LiftedR = Unit::LiftT<std::invoke_result_t<CallbackType, ExtFutureT, ExtFutureR, Args...>>,
				 REQUIRES(NonNestedExtFuture<ExtFutureT> && NonNestedExtFuture<ExtFutureR>
				 && std::is_invocable_r_v<LiftedR, CallbackType, ExtFutureT, ExtFutureR, Args...>)
				 >
		static auto run_for_then(CallbackType callback, ExtFutureT&& thisfuture, ExtFutureR&& retfuture, Args... args) -> ExtFutureR
		{
			static_assert(!std::is_same_v<LiftedR, void>, "Should never get here with LiftedR == void");

			/**
			 * @note Exception handling.
			 * This is the basic pattern used in multiple ::run()-likes in Qt5.  Note the use of
			 * reportException() on this, which is in these cases the same as the returned future.
			 * We're doing Qt5 one better here, so we'll have to propagate to the control/return future
			 * passed to the callback.
			 * @code
			 *      try {
			 * 		this->m_task->run(*this);
						} catch (QException &e) {
							QFutureInterface<T>::reportException(e);
						} catch (...) {
							QFutureInterface<T>::reportException(QUnhandledException());
						}
			 * @endcode
			 *
			 * Per QException docs: @link http://doc.qt.io/qt-5/qexception.html
			 * "When using QFuture, transferred exceptions will be thrown when calling the following functions:
					QFuture::waitForFinished()
					QFuture::result()
					QFuture::resultAt()
					QFuture::results()
			 * "
			 */

			/*
			 * @see SO: https://stackoverflow.com/questions/34815698/c11-passing-function-as-lambda-parameter
			 *      As usual, C++ language issues need to be worked around, this time when trying to capture @a function
			 *      in the inner lambda.
			 *      Bottom line is that the variable "function" can't be captured by a lambda (or apparently stored at all)
			 *      unless we decay off the reference-ness.
			 */


			auto lambda = [=, callback_copy=std::decay_t<CallbackType>(callback),
					check_retfuture=retfuture]
					(ExtFuture<LiftedR> retfuture_copy, auto... copied_args_from_run) mutable -> void
			{
				LiftedR retval;

				//			static_assert(sizeof...(copied_args_from_run) != 0);
				static_assert(!std::is_same_v<LiftedR, void>, "Should never get here with decltype(retval) == void");

				Q_ASSERT(check_retfuture == retfuture_copy);

				try
				{
					// Call the function the user originally passed in.
					retval = std::invoke(callback_copy, retfuture_copy, copied_args_from_run...);

					// Report our single result.
					retfuture_copy.reportResult(retval);
				}
				catch(ExtAsyncCancelException& e)
				{
					retfuture_copy.reportException(e);
				}
				catch(QException& e)
				{
					retfuture_copy.reportException(e);
				}
				catch (...)
				{
					retfuture_copy.reportException(QUnhandledException());
				}

				/// @todo If exception should we actually be reporting finished?
				retfuture_copy.reportFinished();

			};

			// Run the callback, wrapped in the lambda above, concurrently.
			// We're ignoring the QFuture<> returned by ::run() here, since retfuture is
			// handling that job for us, better.
			QtConcurrent::run(lambda, thisfuture, retfuture, std::forward<Args>(args)...);

			return retfuture;
		} // END detail_struct::run_with_only_given_params()
#endif
	}; // END struct detail_struct

	/**
	 * Modified run forwarder for use by ExtFuture<>.then().
	 */
	template<class CallbackType, class... Args,
			 class ExtFutureT = argtype_t<CallbackType, 0>,
			 class ExtFutureR = argtype_t<CallbackType, 1>,
			 class LiftedR = Unit::LiftT<std::invoke_result_t<CallbackType, ExtFutureT, ExtFutureR, Args...>>,
			 REQUIRES(NonNestedExtFuture<ExtFutureT> && NonNestedExtFuture<ExtFutureR>
			 && std::is_invocable_r_v<LiftedR, CallbackType, ExtFutureT, ExtFutureR, Args...>)
			 >
	ExtFutureR run_for_then(CallbackType&& callback, ExtFutureT&& thisfuture, ExtFutureR&& retfuture, Args&&... args)
	{
		return ExtAsync::detail_struct<CallbackType>::run_for_then(
				std::forward<CallbackType>(callback),
				std::forward<ExtFutureT>(thisfuture),
				std::forward<ExtFutureR>(retfuture),
				std::forward<Args>(args)...);
	}

#if 0
	namespace detail
	{
		template <class T, class = void>
		struct is_void_takes_param : std::false_type {};
		template <class T>
		struct is_void_takes_param<T, std::void_t<ct::has_void_return<T>>>
			: std::true_type {};

		template <class ExtFutureR>
		struct run_helper_struct
		{
			/// For F == void(ExtFutureR&, args...)
			template <class F, class... Args, typename std::enable_if_t<ct::has_void_return_v<F>, int> = 0>
			ExtFutureR run(F&& function, Args&&... args)
			{
				/// @note Used
				qWr() << "EXTASYNC::RUN: IN ExtFutureR run_helper_struct::run(F&& function, Args&&... args):" << __PRETTY_FUNCTION__;

                // ExtFuture<> will default to (STARTED).  This is so that any calls of waitForFinished()
				// against the ExFuture<> (and/or the underlying QFutureInterface<>) will block.
				using RetType = std::remove_reference_t<ExtFutureR>;
				RetType report_and_control;
				static_assert(ct::has_void_return_v<F>, "Callback must return void");
				static_assert(!std::is_reference<RetType>::value, "RetType shouldn't be a reference in here");

				QtConcurrent::run([fn=std::decay_t<F>(function)](RetType extfuture, Args... args) mutable {
					return fn(extfuture, std::move(args)...);
				}, std::forward<RetType>(report_and_control), std::forward<Args>(args)...);

				qWr() << "Returning ExtFuture:" << &report_and_control << report_and_control;
				Q_ASSERT(report_and_control.isStarted());
				Q_ASSERT(!report_and_control.isFinished());
				return report_and_control;
			}
		};

	} // END namespace detail
#endif

	static std::atomic_int64_t s_qthread_id {0};

	static inline void name_qthread()
	{
		int64_t id = ++s_qthread_id;
		QThread::currentThread()->setObjectName(QString("%1_").arg(id) + QThread::currentThread()->objectName() );
	};

    /**
     * Helper struct for creating SFINAE-friendly function overloads-of-last-resort.
     *
     * @link https://gracicot.github.io/tricks/2017/07/01/deleted-function-diagnostic.html
     *
     * ...and this trick actually doesn't work.  It semi-works on gcc with C++11, but C++14 just gives the "use of deleted function" error.
     */
    struct TemplateOverloadResolutionFailed
    {
        template <typename T>
        TemplateOverloadResolutionFailed(T&&)
        {
            static_assert(!std::is_same_v<T,T>, "Unable to find an appropriate template.");
        };
    };

    /**
     * ExtAsync::run() overload-of-last-resort to flag that none of the other templates matched.
     */
//    void run(TemplateOverloadResolutionFailed, ...) = delete;

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
		ExtFutureR report_and_control;

		QtConcurrent::run(std::forward<This*>(thiz), std::forward<F>(std::decay_t<F>(function)),
						  report_and_control, std::forward<Args>(args)...);

		return report_and_control;
	}

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

	/**
     * Asynchronously run a free function of the form:
	 * @code
	 * 	void Function(ExtFuture<T> control_and_reporting_future, Type1 arg1, Type2 arg2, [etc..]);
	 * @endcode
	 * Creates and returns a copy of control_and_reporting_future.
	 *
	 * @todo More params.
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
		using argst = ct::args_t<CallbackType>;
		using arg0t = std::tuple_element_t<0, argst>;
		using ExtFutureR = std::remove_reference_t<arg0t>;
		static_assert(std::is_same_v<ExtFutureT, ExtFutureR>);
#if 0
		ExtFutureT retval;
		qDb() << "FUTURE:" << retval;

        // retval is passed by copy here.
//		QtConcurrent::run([callback_fn=std::decay_t<CallbackType>(callback)](ExtFutureT ef, auto... args) {
//			std::invoke(callback_fn, ef, args...);
//		}, std::forward<ExtFutureT>(retval), std::forward<Args>(args)...);
		QtConcurrent::run(std::forward<CallbackType>(callback), std::forward<ExtFutureT>(retval), std::forward<Args>(args)...);
		return retval;
#else
		return ExtAsync::detail_struct<CallbackType>::run_param_expander(std::forward<CallbackType>(callback), std::forward<Args>(args)...);
#endif
    }

	/**
	 * Asynchronously run @p callback, which must be of the form:
	 * @code
	 * 	R callback(ExtFuture<T>, args...)
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
#if 1
		return ExtAsync::detail_struct<CallbackType>::run_param_expander(std::forward<CallbackType>(callback), std::forward<Args>(args)...);
#else
		return ExtAsync::detail_struct<CallbackType>::run_again(std::forward<CallbackType>(callback), std::forward<Args>(args)...);
#endif
	}


#if 0
	/**
	 * Asynchronously run a "dumb" callback, only passing it the params which are explicitly given
	 * in the run() call.
	 */
	template <class CallbackType, class... Args,
			class LiftedR = Unit::LiftT<ct::return_type_t<CallbackType>>,
			REQUIRES(std::is_invocable_r_v<Unit::DropT<LiftedR>, CallbackType, Args&&...>)
			>
	ExtFuture<LiftedR> run_again(CallbackType&& callback, Args&&... args)
	{
		ExtFuture<LiftedR> ret_future = ExtAsync::detail_struct<CallbackType>::run_again(
					[=, callback_copy=std::decay_t<CallbackType>(callback)](ExtFuture<LiftedR> return_future) {
					return_future.reportFinished(Unit::LiftT<LiftedR>(std::invoke(callback_copy, std::forward<Args>(args)...)));
					return return_future;
					});
		return ret_future;
	}
#endif

	/**
	 * Asynchronously run a free function taking no params and returning non-void/non-ExtFuture<>.
	 *
	 * @param function  An invocable with the signature "R function(void)".
	 * @return
	 */
	template <typename CallableType, typename R = std::invoke_result_t<CallableType, void>,
		REQUIRES(is_non_void_non_ExtFuture_v<R> // Return type is not void or ExtFuture<>
			  && ct::is_invocable_r_v<R, CallableType, void> // F has signature R F(void).
			  )>
	ExtFuture<R> run(CallableType&& function)
	{
		ExtFuture<R> retfuture;

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
				retfuture.reportFinished();
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
	    	;}, std::forward<ExtFuture<R>>(retfuture));

	    return retfuture;
	}

    /**
     * Asynchronously run a free function taking no params we care about here, arbitrary params otherwise,
     * and returning non-void/non-ExtFuture<>.
     *
	 * @warning This is really no better than using QtConcurrent::run() in that the returned future
	 *          isn't cancelable.  Provided mainly for completeness.
	 *
	 * @param function  Callable, R function(Arg)
     * @return
     */
	template <typename CallbackType, typename R = ct::return_type_t<CallbackType>, typename... Args,
        REQUIRES(!std::is_member_function_pointer_v<CallbackType>
			  && (sizeof...(Args) > 0)
			  && is_non_void_non_ExtFuture_v<R>
			&& (arity_v<CallbackType> > 0)
			&& ct::is_invocable_r_v<R, CallbackType, Args&&...>)
        >
	ExtFuture<R> run(CallbackType&& function, Args&&... args)
    {
		ExtFuture<R> retfuture;

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

		return retfuture;
    }

	////// START EXPERIMENTAL

	/**
	 * Returns a callable object which captures f.
	 * Different from async_adapter() in that f is to be called with normal values,
	 * not futures.
	 */
	template <typename F>
	static auto asynchronize(F f)
	{
		return [f](auto ... xs) {
			return [=] () {
				return std::async(std::launch::async, f, xs...);
			};
		};
	}

	/**
	 * Returned object can be called with any number of future objects as parameters.
	 * It then calls .get() on all futures, applies function f to them, and returns the result.
	 */
	template <typename F>
	static auto fut_unwrap(F f)
	{
		return [f](auto ... xs) {
			return f(xs.get()...);
		};
	}

	/**
	 * Wraps a synchronous function, makes it wait for future arguments and returns a future result.
	 */
	template <typename F>
	static auto async_adapter(F f)
	{
		return [f](auto ... xs) {
			return [=] () {
				// What's going on here:
				// - Everything in parameter pack xs is assumed to be a callable object.  They will be called without args.
				// - fut_unwrap(f) transforms f into a function object which accepts an arbitrary number of args.
				// - When this async finally runs f, f calls .get() on all the xs()'s.
				return std::async(std::launch::async, fut_unwrap(f), xs()...);
			};
		};
	}

	////// END EXPERIMENTAL

};

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
static void runInObjectEventLoop(F && fun, QObject * obj = qApp) {
   struct Event : public QEvent {
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

/// Above is pre-Qt5.10.  The below should be used from Qt5.10+.

/**
 * For callables with the signature "void Callback(void)".
 */
template <class CallableType,
		  REQUIRES(std::is_invocable_r_v<void, CallableType>)>
static void run_in_event_loop(QObject* context, CallableType&& callable)
{
	bool retval = QMetaObject::invokeMethod(context, std::forward<CallableType>(callable));
	// Die if the function couldn't be invoked.
	Q_ASSERT(retval == true);
}

/**
 * For callables with the signature "ReturnType Callback(void)", where ReturnType != ExtFuture.
 */
//template <class CallableType, class ReturnType = Unit::LiftT<std::invoke_result_t<CallableType>>,
//		  REQUIRES(is_non_void_non_ExtFuture_v<ReturnType>
//		  && std::is_invocable_r_v<Unit::DropT<ReturnType>, CallableType>)>
//static ReturnType run_in_event_loop(QObject* context, CallableType&& callable)
//{
//	ReturnType return_value;
//	bool retval;
//	if constexpr(std::is_same_v<ReturnType, void>)
//	{
//		static_assert(std::is_same_v<ReturnType, void>, "Bad return type");
//		// callable returns void.
//		retval = QMetaObject::invokeMethod(context, std::forward<CallableType>(callable));
//		return_value = unit;
//	}
//	else
//	{
//		// callable returns a non-void.
//		retval = QMetaObject::invokeMethod(context, std::forward<CallableType>(callable), &return_value);
//		/// @todo We're getting "QMetaObject::invokeMethod: Unable to invoke methods with return values in queued connections" here.
//		Q_ASSERT_X(retval == true, __PRETTY_FUNCTION__, "invokeMethod() failed");
//	}
//	// Die if the function couldn't be invoked.
//	/// @todo We're getting "QMetaObject::invokeMethod: Unable to invoke methods with return values in queued connections" here.
//	Q_ASSERT(retval == true);
//	return return_value;
//}

namespace ExtAsync
{

//.....

} // Namespace ExtAsync.

#endif /* UTILS_CONCURRENCY_EXTASYNC_H_ */
