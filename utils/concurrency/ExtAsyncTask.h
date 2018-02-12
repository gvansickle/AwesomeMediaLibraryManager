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

#ifndef UTILS_CONCURRENCY_EXTASYNCTASK_H_
#define UTILS_CONCURRENCY_EXTASYNCTASK_H_

/// Based on this Stack Overflow reply: https://stackoverflow.com/a/16729619

/**
 * Subclass this to get a controllable, reporting-enabled task which can be passed to one of the ExtAsync::run() overloads.
 * Note that this class does not derive from QObject (or any other class for that matter).
 * @tparam T  The type returned in the ExtFuture<T>.
 */
template <typename T, typename FutureType = ExtFuture<T>>
class ExtAsyncTask
{
public:
	virtual ~ExtAsyncTask();

	/**
	 * Override this in your derived class to do the long-running work.
	 * Periodically check @a control->isCanceled() and return if it returns false.
	 * Send results out via one of the control->reportResult() overloads.
	 *
	 * @note Fun Fact about progress reporting: It's all too easy to flood the GUI thread
	 * with progress updates unless you know one non-obvious thing about QFutureWatcher<>'s
	 * progressValueChanged() implementation.  From the Qt5 docs (http://doc.qt.io/qt-5/qfuturewatcher.html):
	 *
	 * "In order to avoid overloading the GUI event loop, QFutureWatcher limits the progress signal emission rate.
	 *  This means that listeners connected to this slot might not get all progress reports the future makes.
	 *  The last progress update (where ***progressValue equals the maximum value***) will always be delivered."
	 *
	 * Emphasis mine.  That means don't do what I did and set the progress range max equal to the progressValue when
	 * you don't know a priori what the range max is (e.g. during a directory traversal).
	 * If you do so, the GUI slows to a crawl.
	 */
	virtual void run(FutureType& control) = 0;
};


/**
 * Used as an intermediary helper class between the ExtAsyc::run() functions and an
 * instance of ExtAsyncTask<T>.  Handles the setup and teardown.  Setup handled in the run() function
 * includes reporting started and not starting if already cancelled.  Teardown includes reporting finished,
 * if finishing was due to cancellation, and propagating and reporting exceptions.
 * QRunnable handles autoDelete() and ref counting.
 *
 * @note This is a sort of combination of the RunFunctionTaskBase<> and RunFunctionTask<> class templates
 * from Qt5.
 *
 * @note It appears that QRunnable will not be copyable in Qt6.  Not sure if that will affect us or not.
 *
 * @see /usr/include/qt5/QtConcurrent/qtconcurrentrunbase.h
 */
template <typename T>
class ExtAsyncTaskRunner : public ExtFuture<T>, public QRunnable
{
public:
    explicit ExtAsyncTaskRunner(ExtAsyncTask<T>* tsk) : m_task(tsk) { }
    virtual ~ExtAsyncTaskRunner() { delete m_task; }

	/**
	 * start() function analogous to those in Qt5's RunFunctionTaskBase<>.
	 */
    ExtFuture<T> start(QThreadPool *pool = QThreadPool::globalInstance());

	/**
	 * The run() function is analogous to that in Qt5's RunFunctionTask<> class template.
	 * It's an overridden virtual function inherited from QRunnable.
	 */
	void run() override;

	ExtAsyncTask<T> *m_task;
};


//// ExtAsyncTask<T> Implementation below.

template<class T, typename FutureType>
ExtAsyncTask<T, FutureType>::~ExtAsyncTask()
{
	// Nothing.
}

//// ExtAsyncTaskRunner<T> Implementation below.

template<typename T>
ExtFuture<T> ExtAsyncTaskRunner<T>::start(QThreadPool* pool)
{
	this->setThreadPool(pool);
	this->setRunnable(this);
	// Report that we've started via the QFutureInterface.
	this->reportStarted();
	/// @note The original has: "this->future();" here.
	/// Since ExtFuture<-QFutureInterface and the latter is a
	/// QFuture "factory" of sorts, returning a copy of ourselves seems
	/// correct here.
	ExtFuture<T> extfuture = *this;
	pool->start(this, /*m_priority*/ 0);
	return extfuture;
}

template<typename T>
void ExtAsyncTaskRunner<T>::run()
{
	if (this->isCanceled())
	{
		this->reportFinished();
		return;
	}
#ifndef QT_NO_EXCEPTIONS
	try {
#endif
		// Run the actual worker function.
		/// In Qt5 QtConcurrent's RunFunctionTask*<>, this is done somewhat differently.
		/// A "StoredFunctorCall0" struct (or one of dozens of variants) from
		/// the header /usr/include/qt5/QtConcurrent/qtconcurrentstoredfunctioncall.h
		/// come into play here, and effectively does something like this:
		///     void runFunctor() override { this->result = function(); }
		/// Since we want to be able to send up interim results and progress through a
		/// QFutureInterface<> of some sort, we need to do something else.
		this->m_task->run(*this);
#ifndef QT_NO_EXCEPTIONS
	} catch (QException &e) {
		ExtFuture<T>::reportException(e);
	} catch (...) {
		ExtFuture<T>::reportException(QUnhandledException());
	}
#endif
	if (this->isCanceled())
	{
		// Report that we were canceled.
		this->reportCanceled();
	}
	else
	{
		this->reportFinished();
	}
}


#endif /* UTILS_CONCURRENCY_EXTASYNCTASK_H_ */
