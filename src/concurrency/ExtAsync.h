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

//#include "ExtFuture.h"
//#include "ExtFutureWatcher.h"


#include <type_traits>
#include "future_type_traits.hpp"
#include "function_traits.hpp"
#include "cpp14_concepts.hpp"
#include <functional>
#include <memory>

#include <QEvent>
#include <QObject>
#include <QtConcurrent>
#include <QtCore/QFutureInterface>
#include <QRunnable>

#include <QCoreApplication>

#include "utils/DebugHelpers.h"

//#include "impl/ExtFuture_fwddecl_p.h"

template <typename T> class ExtFuture;
//template <typename T> class ExtAsyncTask;
//template <typename T> class ExtAsyncTaskRunner;

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
 * This namespace and its run() functions are analogous to the run() function templates in the Qt5 header
 * /usr/include/qt5/QtConcurrent/qtconcurrentrun.h which live in the QtConcurrent namespace.
 */
namespace ExtAsync
{
	namespace detail
	{
		template <class T, class = void>
		struct is_void_takes_param : std::false_type {};
		template <class T>
		struct is_void_takes_param<T, void_t<ct::has_void_return<T>>>
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

				// ExtFuture<> will default to (STARTED | RUNNING).  This is so that any calls of waitForFinished()
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

	template <typename T>
	static ExtFuture<T> run(ExtAsyncTask<T>* task)
	{
		qWr() << "EXTASYNC::RUN: IN ExtFuture<T> run(ExtAsyncTask<T>* task):" << __PRETTY_FUNCTION__;

		return (new ExtAsyncTaskRunner<T>(task))->start();
	}

	/**
	 * ExtAsync::run() overload for member functions taking an ExtFuture<T>& as the first non-this param.
	 * E.g.:
	 * 		void Class::Function(ExtFuture<T>& future, Type1 arg1, Type2 arg2, [etc..]);
	 *
	 * @returns The ExtFuture<T> passed to @a function.
	 */
	template <typename This, typename F, typename... Args,
		std::enable_if_t<ct::has_void_return_v<F>, int> = 0>
	auto
	run(This* thiz, F&& function, Args&&... args)
	{
		// Extract the type of the first arg of function, which should be an ExtFuture<?>&.
		using argst = ct::args_t<F>;
		using arg1t = std::tuple_element_t<1, argst>;
		using ExtFutureR = std::remove_reference_t<arg1t>;

		qWr() << "EXTASYNC::RUN: IN ExtFuture<R> run(This* thiz, F&& function, Args&&... args):" << __PRETTY_FUNCTION__;

		static_assert(sizeof...(Args) <= 1, "Too many args");
		static_assert(function_traits<F>::arity_v > 1, "Callback must take at least one argument");

		// ExtFuture<> will default to (STARTED | RUNNING).  This is so that any calls of waitForFinished()
		// against the ExFuture<> (and/or the underlying QFutureInterface<>) will block.
		ExtFutureR report_and_control;

		QtConcurrent::run(thiz, std::forward<F>(function), report_and_control, std::forward<Args>(args)...);

		return report_and_control;
	}

	/**
	 * For free functions of the form:
	 * 	void Function(ExtFuture<T>& future, Type1 arg1, Type2 arg2, [etc..]);
	 *
	 * Note use of C++14 auto return type deduction.
	 */
	template <class F, /*class R = ExtFuture<int>,*/ class... Args, std::enable_if_t<ct::has_void_return_v<F>, int> = 0>
	auto
	run(F&& function, Args&&... args)
	{
		/// @note Used.
		qWr() << "EXTASYNC::RUN: IN auto run(F&& function, Args&&... args):" << __PRETTY_FUNCTION__;
		// Extract the type of the first arg of function, which should be an ExtFuture<?>&.
		using argst = ct::args_t<F>;
		using arg0t = std::tuple_element_t<0, argst>;
		using ExtFutureR = std::remove_reference_t<arg0t>;
		detail::run_helper_struct<ExtFutureR> helper;

		return helper.run(std::forward<F>(function), std::forward<Args>(args)...);
	}

	/**
	 * Asynchronously run a free function taking no params and returning non-void/non-ExtFuture<>.
	 *
	 * @param function
	 * @return
	 */
	template <typename F, typename R = ct::return_type_t<F>>
		std::enable_if_t<!std::is_member_function_pointer_v<F>
			&& !ct::has_void_return_v<F>
		, ExtFuture<R>>
	run(F&& function)
	{
		/// @note Used.
		qWr() << "EXTASYNC::RUN: IN ExtFuture<R> run(F&& function):" << __PRETTY_FUNCTION__;

		ExtFuture<R> retfuture;

		/*
		 * @see SO: https://stackoverflow.com/questions/34815698/c11-passing-function-as-lambda-parameter
		 *      As usual, C++ language issues need to be worked around, this time when trying to capture @a function
		 *      in the inner lambda.
		 *      Bottom line is that the variable "function" can't be captured by a lambda (or apparently stored at all)
		 *      unless we decay off the reference-ness.
		 */

	    QtConcurrent::run([fn=std::decay_t<F>(function)](ExtFuture<R> retfuture) mutable -> void {
	    	R retval;
	    	// Call the function the user orginally passed in.
	    	retval = fn();
	    	// Report our single result.
	    	retfuture.reportResult(retval);
	    	retfuture.reportFinished();
	    	;}, std::forward<ExtFuture<R>>(retfuture));

	    return retfuture;
	}
};


/// @todo Move this include.
#include "ExtAsyncTask.h"


#include "ExtFuture.h"


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
      ~Event() { fun(); }
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
      ~Event() { (obj->*method)(); }
   };
   QCoreApplication::postEvent(obj, new Event(obj, method));
}

#include "ExtFutureWatcher.h"

void ExtAsyncTest(QObject *context);

#endif /* UTILS_CONCURRENCY_EXTASYNC_H_ */