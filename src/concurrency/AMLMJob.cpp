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

#include "utils/DebugHelpers.h"

/**
 * @todo Static factory function.
 */


AMLMJob::AMLMJob(QObject *parent) : KJob(parent), ThreadWeaver::Job(),
    m_tw_job_qobj_decorator(ThreadWeaver::QJobPointer::create(this, false, this))
{
    make_connections();
}

AMLMJob::~AMLMJob()
{
    qDb() << "DESTRUCTOR";
}

void AMLMJob::requestAbort()
{
    // Set atomic abort flag.
    qDb() << "AMLM:TW: SETTING ABORT FLAG";
    m_flag_cancel = 1;
}

void AMLMJob::start()
{
    /// @todo Do we need to do anything here?  Has the TW Job started already?

    qDb() << "AMLMJob::start(), TWJob status:" << status();
    /// @todo: QTimer::singleShot(0, this, SLOT(doWork()));
}

QPointer<KJob> AMLMJob::asKJobSP()
{
    Q_CHECK_PTR(this);

//    auto shthis = sharedFromThis();
//    auto retval = (QPointer<KJob>)(qobject_cast<KJob>(this));
    auto retval = this;
    Q_CHECK_PTR(retval);
    return retval;
}

ThreadWeaver::JobPointer AMLMJob::asTWJobPointer()
{
    Q_CHECK_PTR(this);

    return sharedFromThis();
//    ThreadWeaver::JobPointer == QPointer<ThreadWeaver::JobInterface>
//    Q_ASSERT(0);
//    auto retval = QPointerDynamicCast<ThreadWeaver::JobInterface>(shthis);
//    Q_CHECK_PTR(retval);

//    return retval;
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
    qDb() << "BEGIN";

    // Essentially a duplicate of QObjectDecorator's implementation.
    Q_CHECK_PTR(this);
    Q_CHECK_PTR(self);

    // Make connections which we need the "real" self for.
    connections_make_defaultEnter(self, thread);

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
        // Only emitted on success.
        /*KJob*/ emitResult();
    }
    qDb() << "EMITTING DONE";
    Q_EMIT /*TWJob*/ done(self);
}

bool AMLMJob::doKill()
{
    qDb() << "DOKILL";

    // Kill the TW::Job.
    requestAbort();

    onKJobDoKill();

    /// @todo Need to wait for the final kill here?

    return true;
}

bool AMLMJob::doSuspend()
{
    /// @todo
    qDb() << "TODO: DOSUSPEND";
    return false;
}

bool AMLMJob::doResume()
{
    /// @todo
    qDb() << "TODO: DORESUME";
    return false;
}

void AMLMJob::make_connections()
{
    /// Qt::DirectConnection here to make this ~a function call.
//    connect(this, &AMLMJob::signalKJobDoKill, this, &AMLMJob::onKJobDoKill, Qt::DirectConnection);
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

/**
 * Make connections we can only make while in defaultEnter() and have the real JobPointer.
 */
void AMLMJob::connections_make_defaultEnter(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
    // Connections from self to m_tw_job_qobj_decorator.

}

/**
 * Make connections we can only make while in defaultExit() and have the real JobPointer.
 */
void AMLMJob::connections_make_defaultExit(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{

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

/////////////////////////////////

TWJobWrapper::TWJobWrapper(ThreadWeaver::JobPointer twjob, bool enable_auto_delete, QObject* parent) : KJob(parent),
    m_the_tw_job(twjob), m_is_autodelete_enabled(enable_auto_delete)
{
    // Now we create the QobjextDecorator and hook it up to the twjob.
    m_the_tw_job_qobj_decorator = ThreadWeaver::QJobPointer::create(m_the_tw_job.data(), enable_auto_delete, this);

    // Connect signals to other signals/slots.
//    connect(m_the_tw_job_qobj_decorator, &ThreadWeaver::QObjectDecorator::done, this, &TWJobWrapper::done);
    /// Qt::DirectConnection here to make this ~a function call.
//    connect(this, &AMLMJob::signalKJobDoKill, this, &AMLMJob::onKJobDoKill, Qt::DirectConnection);
//    connect(this, &AMLMJob::signalKJobDoKill, this, &AMLMJob::onKJobDoKill, Qt::DirectConnection);

    // void started(ThreadWeaver::JobPointer);
    // This signal is emitted when this job is being processed by a thread.
    // internal QObjectDecorator->external QObjectDecorator interface.
    /// @todo Could we get rid of the internal QObjectDecorator?
    /// @answ No, because then AMLMJob would be multiply-derived from QObject twice, through KJob and TW::QObjectDecorator.
//    connect(m_the_tw_job_qobj_decorator, &ThreadWeaver::QObjectDecorator::started, this, &AMLMJob::started);

    //  void done(ThreadWeaver::JobPointer);
    // This signal is emitted when the job has been finished (no matter if it succeeded or not).
//    connect(m_tw_job_qobj_decorator.data(), &ThreadWeaver::QObjectDecorator::done, this, &AMLMJob::done);

    //  void failed(ThreadWeaver::JobPointer);
    // This signal is emitted when success() returns false after the job is executed.
//    connect(m_tw_job_qobj_decorator.data(), &ThreadWeaver::QObjectDecorator::failed, this, &AMLMJob::failed);

    /// @todo Figure out how we're going to trigger KJob::result (emitResult()).
//    connect(m_the_tw_job_qobj_decorator, &ThreadWeaver::QJobPointer::result, this, &AMLMJob::onKJobResult);
//    connect(m_the_tw_job_qobj_decorator, &ThreadWeaver::QJobPointer::finished, this, &AMLMJob::onKJobFinished);
}

TWJobWrapper::~TWJobWrapper()
{
    /// @todo Unclear if we have to delete anything in here.
}

void TWJobWrapper::start()
{

}

void TWJobWrapper::setAutoDelete(bool enable_autodelete)
{
    m_the_tw_job_qobj_decorator->setAutoDelete(enable_autodelete);
}

const ThreadWeaver::JobPointer TWJobWrapper::job() const
{
    return m_the_tw_job;
}

ThreadWeaver::JobPointer TWJobWrapper::job()
{
    return m_the_tw_job;
}

void TWJobWrapper::requestAbort()
{
    // Set atomic abort flag.
    qDb() << "AMLM:TW: SETTING ABORT FLAG";
    m_flag_cancel = 1;
}

//void TWJobWrapper::onKJobResult()
//{
//    /// Called when the KJob is finished.
//    qDb() << "KJOB RESULT" << this;

//    if(this->error())
//    {
//        // There was an error.
//    }
//}

//void TWJobWrapper::onKJobFinished()
//{
//    qDb() << "KJOB FINISHED" << this;
//}
