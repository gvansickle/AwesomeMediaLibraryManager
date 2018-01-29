/*
 * Copyright 2017, 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef AWESOMEMEDIALIBRARYMANAGER_REPORTINGRUNNER_H
#define AWESOMEMEDIALIBRARYMANAGER_REPORTINGRUNNER_H


#include <QtCore/QFutureInterface>
#include <QThreadPool>

/// Based on this Stack Overflow reply: https://stackoverflow.com/a/16729619

/**
 * Subclass this to get a controllable task which can be passed to ReportingRunner::run().
 * @tparam T  The type returned in the QFuture<T>.
 */
template <class T>
class ControllableTask
{
public:
    virtual ~ControllableTask() {}

    /// Override this in your derived class to do the long-running work.
    /// Periodically check @a control->isCanceled() and return if it returns false.
    /// Send results out via one of the control->reportResult() overloads.
    virtual void run(QFutureInterface<T>& control) = 0;
};

/**
 * This is a sort of combination of the RunFunctionTaskBase<> and RunFunctionTask<> class templates
 * from Qt5.
 * @see /usr/include/qt5/QtConcurrent/qtconcurrentrunbase.h
 */
template <typename T>
class RunControllableTask : public QFutureInterface<T> , public QRunnable
{
public:
    explicit RunControllableTask(ControllableTask<T>* tsk) : m_task(tsk) { }
    virtual ~RunControllableTask() { delete m_task; }

	/**
	 * The start() functions are analogous to those in Qt5's RunFunctionTaskBase<>.
	 * @return
	 */
    QFuture<T> start()
    {
		return start(QThreadPool::globalInstance());
    }

	QFuture<T> start(QThreadPool *pool)
	{
		this->setThreadPool(pool);
		this->setRunnable(this);
		// Report that we've started via the QFutureInterface.
		this->reportStarted();
		QFuture<T> future = this->future();
		pool->start(this, /*m_priority*/ 0);
		return future;
	}

	/**
	 * The run() function is analogous to that in Qt5's RunFunctionTask<> class template.
	 * It's an overridden virtual function inherited from QRunnable.
	 */
	void run() override
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
		/// In Qt5 QtConcurrent, this is done somewhat differently.
		/// A "StoredFunctorCall0" struct (or one of dozens of variants) from
		/// the header /usr/include/qt5/QtConcurrent/qtconcurrentstoredfunctioncall.h
		/// come into play here, and effectively does something like this:
		///     void runFunctor() override { this->result = function(); }
		/// Since we want to be able to send up interim results and progress, we
		/// need to do something else.
		this->m_task->run(*this);
#ifndef QT_NO_EXCEPTIONS
		} catch (QException &e) {
			QFutureInterface<T>::reportException(e);
		} catch (...) {
			QFutureInterface<T>::reportException(QUnhandledException());
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

    ControllableTask<T> *m_task;
};

/**
 * This class is analogous to the run() function templates in the Qt5 header
 * /usr/include/qt5/QtConcurrent/qtconcurrentrun.h.  There, they live in the QtConcurrent namespace,
 * here we use a class.
 */
class ReportingRunner
{
public:
    template <class T>
    static QFuture<T> run(ControllableTask<T>* task)
    {
		return (new RunControllableTask<T>(task))->start();
    }
};

#endif //AWESOMEMEDIALIBRARYMANAGER_REPORTINGRUNNER_H
