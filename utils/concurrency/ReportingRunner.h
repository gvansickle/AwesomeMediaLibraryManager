/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

template <typename T>
class RunControllableTask : public QFutureInterface<T> , public QRunnable
{
public:
    explicit RunControllableTask(ControllableTask<T>* tsk) : m_task(tsk) { }
    virtual ~RunControllableTask() { delete m_task; }

    QFuture<T> start()
    {
		this->setRunnable(this);
		// Report that we've started via the QFutureInterface.
		this->reportStarted();
		QFuture<T> future = this->future();
		QThreadPool::globalInstance()->start(this, /*m_priority*/ 0);
		return future;
    }

    void run()
    {
		if (this->isCanceled())
		{
			this->reportFinished();
			return;
		}
		this->m_task->run(*this);
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
