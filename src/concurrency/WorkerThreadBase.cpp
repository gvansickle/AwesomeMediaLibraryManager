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

#include <concurrency/WorkerThreadBase.h>
#include <QDebug>
#include "WorkerThreadBase.h"


WorkerThreadBase::WorkerThreadBase(QObject *parent) : QObject(parent)
{
	/// @warning We're still in the creating thread in the constructor here.
	/// Don't allocate anything from the heap, and be aware of this if you allocate anything else.
}

WorkerThreadBase::~WorkerThreadBase()
{

}


void WorkerThreadBase::moveToThread(QThread *targetThread)
{
	qDebug() << "Current thread is:" << this->thread();
	qDebug() << "Moving to thread:" << targetThread;
    this->QObject::moveToThread(targetThread);
	qDebug() << "Current thread is:" << this->thread();
    connectDefaultSignals(targetThread);
}

void WorkerThreadBase::WorkerStarted()
{
    qWarning() << "WorkerStarted() slot was not overridden in derived class";
}

void WorkerThreadBase::quit()
{
    qDebug() << "Received signal to quit(), emitting finished.";
	Q_EMIT finished();
}

void WorkerThreadBase::connectDefaultSignals(QThread *targetThread)
{
    connect(targetThread, &QThread::started, this, &WorkerThreadBase::WorkerStarted);
    // When work is done, tell the QThread to quit.
    connect(this, &WorkerThreadBase::finished, targetThread, &QThread::quit);
    // ...and also tell the infrastructure to delete us later.
    connect(this, &WorkerThreadBase::finished, this, &WorkerThreadBase::deleteLater);
    // Tell the thread to delete itself when it is finally shut down.
    connect(targetThread, &QThread::finished, targetThread, &QThread::deleteLater);
}

