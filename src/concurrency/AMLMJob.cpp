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

#include "AMLMJob.h"

#include <ThreadWeaver/Job>

#include "utils/DebugHelpers.h"

/**
 * Static factory function.
 */
//AMLMJob *AMLMJob::make_amlmjob(ThreadWeaver::Job* tw_job)
//{
//    return new AMLMJob(tw_job, true, this);
//}


AMLMJob::AMLMJob(JobInterface *decoratee, bool autoDelete, QObject *parent)
    : KJob(parent), ThreadWeaver::IdDecorator(decoratee, autoDelete),
      m_tw_job_qobj_decorator(new ThreadWeaver::QObjectDecorator(decoratee, autoDelete, this))
{
    // Connect the signals that need connecting.

    // void started(ThreadWeaver::JobPointer);
    // This signal is emitted when this job is being processed by a thread.
    // internal QObjectDecorator->external QObjectDecorator interface.
    /// @todo Could we get rid of the internal QObjectDecorator?
    /// @answ No, because then AMLMJob would be multiply-derived from QObject twice, through KJob and TW::QObjectDecorator.
    connect(m_tw_job_qobj_decorator, &ThreadWeaver::QObjectDecorator::started, this, &AMLMJob::started);

    //  void done(ThreadWeaver::JobPointer);
    // This signal is emitted when the job has been finished (no matter if it succeeded or not).
    connect(m_tw_job_qobj_decorator, &ThreadWeaver::QObjectDecorator::done, this, &AMLMJob::done);

    //  void failed(ThreadWeaver::JobPointer);
    // This signal is emitted when success() returns false after the job is executed.
    connect(m_tw_job_qobj_decorator, &ThreadWeaver::QObjectDecorator::failed, this, &AMLMJob::failed);
}

AMLMJob::~AMLMJob()
{
    qDb() << "DELETE";
}

void AMLMJob::start()
{
    /// @todo Do we need to do anything here?  Has the TW Job started already?

    qDb() << "AMLMJob::start(), TWJob status:" << status();
}

void AMLMJob::defaultBegin(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
    // Essentially a duplicate of QObjectDecorator's implementation.
    Q_ASSERT(job());

    Q_EMIT started(self);

    job()->defaultBegin(self, thread);
}

void AMLMJob::defaultEnd(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
    // Essentially a duplicate of QObjectDecorator's implementation.
    Q_ASSERT(job());

    job()->defaultEnd(self, thread);
    if(!self->success())
    {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
}

