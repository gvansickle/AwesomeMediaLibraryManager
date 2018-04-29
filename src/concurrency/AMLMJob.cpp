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
    : KJob(parent), ThreadWeaver::Job(),
      m_tw_job_qobj_decorator(new ThreadWeaver::QObjectDecorator(decoratee, autoDelete, this))
{
    // Connect the signals that need connecting.
    make_connections();
}

AMLMJob::AMLMJob() : KJob(), ThreadWeaver::Job(),
    m_tw_job_qobj_decorator(new ThreadWeaver::QObjectDecorator(this, false, this))
{
    make_connections();
}

AMLMJob::~AMLMJob()
{
    qDb() << "DESTRUCTOR";
}

void AMLMJob::start()
{
    /// @todo Do we need to do anything here?  Has the TW Job started already?

    qDb() << "AMLMJob::start(), TWJob status:" << status();
    /// @todo: QTimer::singleShot(0, this, SLOT(doWork()));
}

KJob* AMLMJob::asKJob()
{
    auto retval = dynamic_cast<KJob*>(this);
    Q_CHECK_PTR(retval);
    return retval;
}

ThreadWeaver::IdDecorator* AMLMJob::asIdDecorator()
{
    auto retval = dynamic_cast<ThreadWeaver::IdDecorator*>(this);
    Q_CHECK_PTR(retval);
    return retval;
}

void AMLMJob::setProcessedAmount(KJob::Unit unit, qulonglong amount)
{
    KJob::setProcessedAmount(unit, amount);
}

void AMLMJob::setTotalAmount(KJob::Unit unit, qulonglong amount)
{
    KJob::setTotalAmount(unit, amount);
}

void AMLMJob::setPercent(unsigned long percentage)
{
    this->KJob::setPercent(percentage);
}

void AMLMJob::defaultBegin(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
    qDb() << "BEGIN";

    // Essentially a duplicate of QObjectDecorator's implementation.
    Q_CHECK_PTR(this);
    Q_CHECK_PTR(self);

//    qDb() << "autoDelete()?:" << autoDelete();

    Q_EMIT started(self);

    ThreadWeaver::Job::defaultBegin(self, thread);
}

void AMLMJob::defaultEnd(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
    qDb() << "END";

    // Essentially a duplicate of QObjectDecorator's implementation.
    Q_CHECK_PTR(this);
    Q_CHECK_PTR(self);

    ThreadWeaver::Job::defaultEnd(self, thread);
    if(!self->success())
    {
        qWr() << "FAILED";
        Q_EMIT /*TWJob*/ failed(self);
    }
    else
    {
        qDb() << "Succeeded";
        /*KJob*/ emitResult();
    }
    qDb() << "EMITTING DONE";
    Q_EMIT /*TWJob*/ done(self);
}

bool AMLMJob::doKill()
{
    Q_EMIT signalKJobDoKill();
}

bool AMLMJob::doSuspend()
{
    /// @todo
    return false;
}

bool AMLMJob::doResume()
{
    /// @todo
    return false;
}

void AMLMJob::make_connections()
{
    /// Qt::DirectConnection here to make this ~a function call.
    connect(this, &AMLMJob::signalKJobDoKill, this, &AMLMJob::onKJobDoKill, Qt::DirectConnection);

    // void started(ThreadWeaver::JobPointer);
    // This signal is emitted when this job is being processed by a thread.
    // internal QObjectDecorator->external QObjectDecorator interface.
    /// @todo Could we get rid of the internal QObjectDecorator?
    /// @answ No, because then AMLMJob would be multiply-derived from QObject twice, through KJob and TW::QObjectDecorator.
    connect(m_tw_job_qobj_decorator.data(), &ThreadWeaver::QObjectDecorator::started, this, &AMLMJob::started);

    //  void done(ThreadWeaver::JobPointer);
    // This signal is emitted when the job has been finished (no matter if it succeeded or not).
    connect(m_tw_job_qobj_decorator.data(), &ThreadWeaver::QObjectDecorator::done, this, &AMLMJob::done);

    //  void failed(ThreadWeaver::JobPointer);
    // This signal is emitted when success() returns false after the job is executed.
    connect(m_tw_job_qobj_decorator.data(), &ThreadWeaver::QObjectDecorator::failed, this, &AMLMJob::failed);

    /// @todo Figure out how we're going to trigger KJob::result (emitResult()).
    connect(this, &KJob::result, this, &AMLMJob::onKJobResult);
    connect(this, &KJob::finished, this, &AMLMJob::onKJobFinished);
}

void AMLMJob::onKJobDoKill()
{
    qDb() << "ENTER";



    qDb() << "EXIT";
}

void AMLMJob::onKJobResult(KJob *job)
{
    /// Called when the KJob is finished.
    qDb() << "KJOB RESULT" << job;

    if(job->error())
    {
        // There was an error.
    }
}

void AMLMJob::onKJobFinished(KJob *job)
{
    qDb() << "KJOB FINISHED" << job;
}

