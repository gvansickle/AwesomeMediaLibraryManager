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

#include <QPointer>
#include <ThreadWeaver/Job>
//#include <ThreadWeaver/IdDecorator>
#include <ThreadWeaver/DebuggingAids>

#include "utils/DebugHelpers.h"
#include "utils/UniqueIDMixin.h"


AMLMJob::AMLMJob(QObject *parent)
    : KJob(parent), ThreadWeaver::Job()
{
    qDb() << M_NAME_VAL(this);

    /// @todo This is debug, move/remove.
    ThreadWeaver::setDebugLevel(true, 10);
    qDb() << "Set TW::DebugLevel:" << ThreadWeaver::Debug << ThreadWeaver::DebugLevel;
}

AMLMJob::~AMLMJob()
{
//    qDb() << "DESTRUCTOR:" << objectName();
}

void AMLMJob::requestAbort()
{
    // Set atomic abort flag.
    qDb() << "AMLM:TW: SETTING ABORT FLAG";
    m_flag_cancel = 1;
}

void AMLMJob::start()
{
    /// @todo Do we need to do anything here?  The TW::Job starts by the TW::Queue/Weaver it's added to.

    qDb() << "AMLMJob::start(), TWJob status:" << status();
    /// @todo: QTimer::singleShot(0, this, SLOT(doWork()));
}

void AMLMJob::setSuccessFlag(bool success)
{
    qDb() << "SETTING SUCCESS:" << success;
    m_success = success;
}

//void AMLMJob::setProcessedAmount(KJob::Unit unit, qulonglong amount)
//{
//    KJob::setProcessedAmount(unit, amount);
//}

//void AMLMJob::setTotalAmount(KJob::Unit unit, qulonglong amount)
//{
//    KJob::setTotalAmount(unit, amount);
//}

//void AMLMJob::setPercent(unsigned long percentage)
//{
//    this->KJob::setPercent(percentage);
//}

void AMLMJob::defaultBegin(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
    qDb() << "ENTER defaultBegin, self/this:" << self << this;
    qDb() << "Current TW::DebugLevel:" << ThreadWeaver::Debug << ThreadWeaver::DebugLevel;

    // Essentially a duplicate of QObjectDecorator's implementation.
    /// @link https://cgit.kde.org/threadweaver.git/tree/src/qobjectdecorator.cpp?id=a36f37705746561edf10affd77d22852076469b4

    Q_CHECK_PTR(this);
    Q_CHECK_PTR(self);

    // Make connections which we need the "real" self for.
    connections_make_defaultBegin(self, thread);

//    qDb() << "autoDelete()?:" << self->autoDelete();

    Q_EMIT started(self);

    // "job()->defaultBegin(self, thread);"
    this->ThreadWeaver::Job::defaultBegin(self, thread);
}

void AMLMJob::defaultEnd(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
    qDb() << "ENTER defaultEnd, self/this:" << self << this;
    qDb() << "Current TW::DebugLevel:" << ThreadWeaver::Debug << ThreadWeaver::DebugLevel;

    // Essentially a duplicate of QObjectDecorator's implementation.
    /// @link https://cgit.kde.org/threadweaver.git/tree/src/qobjectdecorator.cpp?id=a36f37705746561edf10affd77d22852076469b4

    Q_CHECK_PTR(this);
    Q_CHECK_PTR(self);

    // Call base class implementation.
    // "job()->defaultEnd(self, thread);"
    this->ThreadWeaver::Job::defaultEnd(self, thread);

    if(!self->success())
    {
        qWr() << "FAILED";
        Q_EMIT /*TWJob*/ this->failed(self);
    }
    else
    {
        qDb() << "Succeeded";
        // Only call this on success.
        /*KJob*/ emitResult();
    }
    qDb() << "EMITTING DONE";
    Q_EMIT /*TWJob*/ this->done(self);
}

bool AMLMJob::doKill()
{
    // KJob::doKill().
    qDb() << "DOKILL";

    // Kill the TW::Job.
    requestAbort();

    onKJobDoKill();

    /// @todo Need to wait for the final kill here?

    return true;
}

bool AMLMJob::doSuspend()
{
    /// @todo // KJob::doSuspend().
    qDb() << "TODO: DOSUSPEND";
    return false;
}

bool AMLMJob::doResume()
{
    /// @todo // KJob::doResume().
    qDb() << "TODO: DORESUME";
    return false;
}

void AMLMJob::make_connections()
{
    qDb() << "MAKING CONNECTIONS, this:" << this;

//    Q_ASSERT(!m_tw_job_qobj_decorator.isNull());

    /// @name TW::IdDecorator connections.
    /// @{

    // void started(ThreadWeaver::JobPointer);
    // This signal is emitted when this job is being processed by a thread.
    // internal QObjectDecorator->external QObjectDecorator interface.
    connect(this, &AMLMJob::started, this, &AMLMJob::onTWStarted);

    //  void done(ThreadWeaver::JobPointer);
    // This signal is emitted when the job has been finished (no matter if it succeeded or not).
    connect(this, &AMLMJob::done, this, &AMLMJob::onTWDone);

    //  void failed(ThreadWeaver::JobPointer);
    // This signal is emitted when success() returns false after the job is executed.
    connect(this, &AMLMJob::failed, this, &AMLMJob::onTWFailed);

    /// @}

    // Connect up KJob signals/slots.
    // Emitted as a byproduct of calling emitResult(), which simply calls KJob::finishJob(true),
    // which:
    // - quits any private event loop
    // - emits finished(this)
    // - Because of the true param, emits result(this)
    // - If KJob isAutoDelete(), calls deleteLater().
    // KJob::kill() will also cause the emission of ::result, via the same finsishJob() path, if KillVerbosity is
    // not Quietly.
    connect(this, &KJob::result, this, &AMLMJob::onKJobResult);

    // Emitted by calling emitResult() and kill().
    // Intended to notify UIs that should detach from the job.
    /// @todo This event fires and gets to AMLMJob::onKJobFinished() after this has been destructed.
    connect(this, &KJob::finished, this, &AMLMJob::onKJobFinished);

    qDb() << "MADE CONNECTIONS, this:" << this;
}

/**
 * Make connections we can only make while in defaultEnter() and have the real JobPointer.
 */
void AMLMJob::connections_make_defaultBegin(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
    qDb() << "ENTER connections_make_defaultBegin";
    Q_CHECK_PTR(self);
}

/**
 * Break connections we can only break while in defaultExit() and have the real JobPointer.
 */
void AMLMJob::connections_make_defaultExit(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
    qDb() << "ENTER connections_make_defaultExit";
    Q_CHECK_PTR(self);
}

void AMLMJob::onTWStarted(ThreadWeaver::JobPointer twjob)
{
    qDb() << "ENTER onTWStarted";
    Q_CHECK_PTR(twjob);
}

void AMLMJob::onTWDone(ThreadWeaver::JobPointer twjob)
{
    qDb() << "ENTER onTWDone";
    Q_CHECK_PTR(twjob);
}

void AMLMJob::onTWFailed(ThreadWeaver::JobPointer twjob)
{
    qDb() << "ENTER onTWFailed";
    Q_CHECK_PTR(twjob);
}

void AMLMJob::onKJobDoKill()
{
    qDb() << "ENTER onKJobDoKill";

    qDb() << "EXIT onKJobDoKill";
}

void AMLMJob::onKJobResult(KJob *job)
{
    Q_CHECK_PTR(job);

    /// Called when the KJob is finished.
    qDb() << "KJOB RESULT" << job;

    if(job->error())
    {
        // There was an error.
    }
}

void AMLMJob::onKJobFinished(KJob *job)
{
    Q_CHECK_PTR(job);

    qDb() << "KJOB FINISHED" << job;
}

