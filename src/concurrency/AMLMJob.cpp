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
#include <ThreadWeaver/IdDecorator>

#include "utils/DebugHelpers.h"
#include "utils/UniqueIDMixin.h"


AMLMJob::AMLMJob(QObject *parent)
    : KJob(parent), ThreadWeaver::Job()
{
    qDb() << M_NAME_VAL(this);
}

AMLMJob::~AMLMJob()
{
    qDb() << "DESTRUCTOR:" << objectName();
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

//QPointer<KJob> AMLMJob::asKJobSP()
//{
//    Q_CHECK_PTR(this);

////    auto shthis = sharedFromThis();
////    auto retval = (QPointer<KJob>)(qobject_cast<KJob>(this));
//    QPointer<KJob> retval = this;
//    Q_CHECK_PTR(retval);

//    return retval;
//}

//ThreadWeaver::JobPointer AMLMJob::asTWJobPointer()
//{
//M_WARNING("TODO: SHould this be returning this or the QObjectDecorator?");
//    Q_CHECK_PTR(this);

//    // ThreadWeaver::JobPointer is a QSharedPointer<TW::JobInterface>, so
//    // we need to make sure we return a sp which doesn't duplicate the ref count.

////    auto retval = this->sharedFromThis();
////    ThreadWeaver::JobPointer retval = m_tw_job_qobj_decorator;

////    Q_ASSERT(retval);

//    return this;
//}

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

    // Essentially a duplicate of QObjectDecorator's implementation.
    /// @link https://cgit.kde.org/threadweaver.git/tree/src/qobjectdecorator.cpp?id=a36f37705746561edf10affd77d22852076469b4


    Q_CHECK_PTR(this);
    Q_CHECK_PTR(self);

    // Make connections which we need the "real" self for.
    connections_make_defaultBegin(self, thread);

//    qDb() << "autoDelete()?:" << self->autoDelete();

    Q_EMIT started(self);

    ThreadWeaver::Job::defaultBegin(self, thread);
}

void AMLMJob::defaultEnd(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
    qDb() << "ENTER defaultEnd, self/this:" << self << this;

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
        // Only emitted on success.
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
    /// @todo Could we get rid of the internal QObjectDecorator?
    /// @answ No, because then AMLMJob would be multiply-derived from QObject twice, through KJob and TW::QObjectDecorator.
    /// @note The .data() deref is necessary, connect can't otherwise connect through a QSharedPointer.
    connect(this, &AMLMJob::started, this, &AMLMJob::onTWStarted);

    //  void done(ThreadWeaver::JobPointer);
    // This signal is emitted when the job has been finished (no matter if it succeeded or not).
    connect(this, &AMLMJob::done, this, &AMLMJob::onTWDone);

    //  void failed(ThreadWeaver::JobPointer);
    // This signal is emitted when success() returns false after the job is executed.
    connect(this, &AMLMJob::failed, this, &AMLMJob::onTWFailed);

    /// @}

    // Connect up KJob signals/slots.
    connect(this, &KJob::result, this, &AMLMJob::onKJobResult);
    /// @todo This event fires and gets to AMLMJob::onKJobFinished() after this has been destructed.
    connect(this, &KJob::finished, this, &AMLMJob::onKJobFinished);

    qDb() << "MADE CONNECTIONS, this:" << this;
}

/**
 * Make connections we can only make while in defaultEnter() and have the real JobPointer.
 */
void AMLMJob::connections_make_defaultBegin(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
    qDb() << "ENTER";

}

/**
 * Break connections we can only break while in defaultExit() and have the real JobPointer.
 */
void AMLMJob::connections_make_defaultExit(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{

}

void AMLMJob::onTWStarted(ThreadWeaver::JobPointer twjob)
{
    qDb() << "ENTER";
}

void AMLMJob::onTWDone(ThreadWeaver::JobPointer twjob)
{
    qDb() << "ENTER";
}

void AMLMJob::onTWFailed(ThreadWeaver::JobPointer twjob)
{
    qDb() << "ENTER";
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

