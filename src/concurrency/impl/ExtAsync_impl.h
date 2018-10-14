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
 * @file ExtAsync_impl.h
 */
#ifndef SRC_CONCURRENCY_IMPL_EXTASYNC_IMPL_H_
#define SRC_CONCURRENCY_IMPL_EXTASYNC_IMPL_H_

#include <config.h>

// Std C++
#include <functional>

// Future Std C++
#include <future/future_type_traits.hpp>
#include <future/function_traits.hpp>
#include <future/cpp14_concepts.hpp>
#include <future/Unit.hpp>

// Qt5
#include <QException>
#include <QtConcurrentRun>

// Ours
#include <utils/DebugHelpers.h>
#include "../ExtFuture.h"
#include "../ExtAsync_traits.h"
#include "../ExtAsyncExceptions.h"

//template <class T>
//class ExtFuture;

namespace ExtAsync
{
	template <class CallbackType>
	struct detail_struct
	{

	template<class... Args,
			 class ExtFutureT = argtype_t<CallbackType, 0>,
			 class R = Unit::LiftT<std::invoke_result_t<CallbackType, ExtFutureT, Args...>>,
			 REQUIRES(is_ExtFuture_v<ExtFutureT>
			 && ct::is_invocable_r_v<R, CallbackType, ExtFutureT, Args...>)
			 >
	static auto run_again(CallbackType callback, Args... args) -> ExtFuture<R>
	{
		static_assert(!std::is_same_v<R, void>, "Should never get here with retval == void");

		using ExtFutureR = ExtFuture<R>;

		ExtFutureR retfuture;

		// This exists solely so we can assert that the ExtFuture passed into the callback is correct.
		ExtFutureR check_retfuture = retfuture;

//		static_assert(sizeof...(args) != 0);


		/*
		 * @see SO: https://stackoverflow.com/questions/34815698/c11-passing-function-as-lambda-parameter
		 *      As usual, C++ language issues need to be worked around, this time when trying to capture @a function
		 *      in the inner lambda.
		 *      Bottom line is that the variable "function" can't be captured by a lambda (or apparently stored at all)
		 *      unless we decay off the reference-ness.
		 */


		auto lambda = [=, callback_copy=std::decay_t<CallbackType>(callback),
					check_retfuture=retfuture]
					(ExtFuture<R> retfuture_copy, auto... copied_args_from_run) mutable -> void
		{
			R retval;

//			static_assert(sizeof...(copied_args_from_run) != 0);
			static_assert(!std::is_same_v<R, void>, "Should never get here with retval == void");

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
				/**
				 * Per std::experimental::shared_future::then() at @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
				 * "Any value returned from the continuation is stored as the result in the shared state of the returned future object.
				 *  Any exception propagated from the execution of the continuation is stored as the exceptional result in the shared
				 *  state of the returned future object."
				 */
	//			qDb() << "Rethrowing exception";
				retfuture_copy.reportException(e);
			}
			catch(QException& e)
			{
	//			qDb() << "Rethrowing exception";
				retfuture_copy.reportException(e);
			}
			catch (...)
			{
	//			qDb() << "Rethrowing exception";
				retfuture_copy.reportException(QUnhandledException());
			}

	//		qDb() << "REPORTING FINISHED";
			retfuture_copy.reportFinished();

			};

		QtConcurrent::run(lambda, retfuture, std::forward<Args>(args)...);

		return retfuture;
	}

	}; // END struct detail
} // END namespace ExtAsync

//template <class CallbackType,
//		class T, class U,
//		  class R = Unit::LiftT<ct::return_type_t<CallbackType>>
////		  REQUIRES(ct::is_invocable_r_v<R, CallbackType>)
//		  >
//ExtFuture<R> run_again(CallbackType&& callback, T t, U u)
//{
//	return ExtAsync::detail_struct<CallbackType>::run_again(callback, t, u);
//}

template <class CallbackType,
//		  class R = Unit::LiftT<ct::return_type_t<CallbackType>>
		class R = Unit::LiftT<ct::return_type_t<CallbackType>>
//		class R = Unit::LiftT<std::invoke_result_t<CallbackType>>
>
ExtFuture<R> run_again(CallbackType&& callback)
{
	return ExtAsync::detail_struct<CallbackType>::run_again(callback);
}

#endif /* SRC_CONCURRENCY_IMPL_EXTASYNC_IMPL_H_ */
