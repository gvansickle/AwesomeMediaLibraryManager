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

#ifndef WORKERTHREADBASE_H
#define WORKERTHREADBASE_H

#include <QObject>
#include <QThread>

/**
 * Class which tries to encapsulate some of the rather complex Qt5 threading issues into a single
 * base class, whcih can then be more conveniently subclassed.
 * See here: https://mayaposch.wordpress.com/2011/11/01/how-to-really-truly-use-qthreads-the-full-explanation/
 */
class WorkerThreadBase : public QObject
{
    Q_OBJECT

public:
    explicit WorkerThreadBase(QObject *parent = 0);

	void moveToThread(QThread* targetThread);

signals:
	/// Signal we will emit when all work is complete and we should be destroyed.
	void finished();

public slots:
	/// Signaled when the QThread is started.  Override.
	virtual void WorkerStarted() = 0;
	virtual void WorkerFinished() = 0;

	/// By default, emits finished signal.
	virtual void quit();

private:
	Q_DISABLE_COPY(WorkerThreadBase)

	void connectDefaultSignals(QThread* targetThread);
};

#endif // WORKERTHREADBASE_H
