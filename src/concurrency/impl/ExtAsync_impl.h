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


#include <functional>

//#include "../ExtFuture.h"

template<class CallbackType,
		 class ExtFutureT = argtype_t<CallbackType, 0>, //std::tuple_element_t<0, ct::args_t<CallbackType>>,
		 class T = typename isExtFuture<ExtFutureT>::inner_t,
		 class... Args,
		 class R = std::invoke_result_t<CallbackType, ExtFutureT, Args...>,
		 REQUIRES(is_ExtFuture_v<ExtFutureT>
		 && ct::is_invocable_r_v<R, CallbackType, ExtFutureT, Args&&...>)
		 >
ExtFuture<R> run_again(CallbackType&& callback, Args&&... args)
{
	using ExtFutureR = ExtFuture<R>;

	ExtFutureR retfuture;

	// This exists solely so we can assert that the ExtFuture passed into the callback is correct.
	ExtFutureR check_retfuture = retfuture;

	/*
	 * @see SO: https://stackoverflow.com/questions/34815698/c11-passing-function-as-lambda-parameter
	 *      As usual, C++ language issues need to be worked around, this time when trying to capture @a function
	 *      in the inner lambda.
	 *      Bottom line is that the variable "function" can't be captured by a lambda (or apparently stored at all)
	 *      unless we decay off the reference-ness.
	 */

	QtConcurrent::run([fn=std::decay_t<CallbackType>(callback), check_retfuture](ExtFutureR retfuture) -> void {
		R retval;

		Q_ASSERT(check_retfuture == retfuture);

		try
		{
			// Call the function the user originally passed in.
//			qDb() << "RUNNING";
			retval = std::invoke(fn, retfuture);
//			qDb() << "RUNNING END";
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
			qDb() << "Rethrowing exception";
			retfuture.reportException(e);
			// I think we don't want to rethrow like this here.  This will throw to the wrong future
			// (the QtConcurrent::run() retval, see above.
			//throw;
		}
		catch(QException& e)
		{
			qDb() << "Rethrowing exception";
			retfuture.reportException(e);
		}
		catch (...)
		{
			qDb() << "Rethrowing exception";
			retfuture.reportException(QUnhandledException());
		}

		qDb() << "REPORTING FINISHED";
		retfuture.reportFinished();

		}, retfuture);

	return retfuture;
}



#endif /* SRC_CONCURRENCY_IMPL_EXTASYNC_IMPL_H_ */
