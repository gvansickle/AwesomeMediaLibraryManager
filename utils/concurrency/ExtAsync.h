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

#include "ExtFuture.h"
#include "ExtFutureWatcher.h"

#include <type_traits>
#include <functional>

#include <QEvent>
#include <QObject>
#include <QtConcurrent>
#include <QtCore/QFutureInterface>
#include <QRunnable>

#include <QCoreApplication>


/**
 * A Qt5 analog to std::async().
 */
template <typename This, typename Function, typename... Args,
		  typename R = QString>
ExtFuture<R>
ExtAsync(This* thiz, Function&& function, Args&&... args)
{
	// ExtFuture<> will default to (STARTED | RUNNING).  This is so that any calls of waitForFinished()
	// against the ExFuture<> (and/or the underlying QFutureInterface<>) will block.
	ExtFuture<R> report_and_control;

	QtConcurrent::run(thiz, function, report_and_control, args...);

	return report_and_control;
}

//template <typename Tin=QString, typename Tout=QString, typename ThenCallbackType = std::function<void(void)>>
inline static void ExtAsyncHelper(std::function<void(void)> then_callback, ExtFuture<QString> predecessor_future, ExtFuture<QString> return_future)
{
	qDb() << "THEN CALLED, WAITING";
	predecessor_future.wait();
	qDb() << "THEN CALLED, WAIT OVER, CALLING CALLBACK";
	then_callback();
	//(*(predecessor_future->m_continuation_function))();
}

//template <//typename Function, // = std::function<int(ExtFuture<QString>&)>, typename... Args,
//		  typename R = QString>
inline static ExtFuture<QString>
ExtAsync(std::function<void(void)> then_callback, ExtFuture<QString> predecessor_future)
{
	ExtFuture<QString> return_future;

//	QtConcurrent::run(std::forward<Function>(function), std::forward<Args>(args)...);
	QtConcurrent::run(ExtAsyncHelper, then_callback, std::forward<ExtFuture<QString>>(predecessor_future), std::forward<ExtFuture<QString>>(return_future));

	return return_future;
}

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

template <typename T, typename Function>
const ExtFuture<T>& onResultReady(ExtFuture<T>& future, QObject *guard, Function f)
{
	auto watcher = new ExtFutureWatcher<T>(); //new QFutureWatcher<T>();
	QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, &QObject::deleteLater);
	QObject::connect(watcher, &QFutureWatcherBase::resultReadyAt, guard, [f, watcher](int index) {
		f(watcher->future().resultAt(index));
	});
	watcher->setFuture(future);
	return future;
}

#endif /* UTILS_CONCURRENCY_EXTASYNC_H_ */
