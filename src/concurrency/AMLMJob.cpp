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

/// Qt5
#include <QPointer>

/// KF5
#include <ThreadWeaver/DebuggingAids>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/Queue>
#include <KJob>

#include <KJobWidgets>
#include <KDialogJobUiDelegate>

/// Ours
#include "utils/DebugHelpers.h"
#include "utils/UniqueIDMixin.h"
#include "utils/QtCastHelpers.h"
#include "utils/TheSimplestThings.h"
#include <gui/MainWindow.h>

AMLMJob::AMLMJob(QObject *parent)
    : KJob(parent), ThreadWeaver::Job()
{
    /// @note Buried in TW::Private::Job_Private there's a mutex "mutex".
    /// It is used in TW::Job in a number of places (e.g. aboutToBeQueued()/removeQueuePolicy()/etc.), but not by default in
    /// anything we're overriding in here as far as I can see.

    // Let's try this...
//    auto sh = new SignalHook(this);
//    sh->hook_all_signals(this);

    setObjectName(uniqueQObjectName());
    qDb() << M_NAME_VAL(this);

    /// @todo This is sort of horrible, we should find a just-in-time way to do the uiDelegate.
    /// ...and also, while this prevents crashes, we don't get any dialog etc. output on fail.
    /// So not at all clear what's happening here.
    KJobWidgets::setWindow(this, MainWindow::instance());
    setUiDelegate(new KDialogJobUiDelegate());

    /// @todo This is debug, move/remove.
    ThreadWeaver::setDebugLevel(true, 10);
    qDb() << "Set TW::DebugLevel:" << ThreadWeaver::Debug << ThreadWeaver::DebugLevel;
}

AMLMJob::~AMLMJob()
{
    // KJob destructor checks if KJob->isFinished and emits finished(this) if so.
    // Doesn't cancel the job.
    qDb() << "AMLMJob DELETED" << this;
}

void AMLMJob::requestAbort()
{
    // Set atomic abort flag.
    qDb() << "AMLM:TW: SETTING ABORT FLAG ON AMLMJOB:" << this;
    m_flag_cancel = 1;
}

void AMLMJob::start()
{
    /// @note The TW::Job starts as soon as it's added to a TW::Queue/Weaver.

    qDb() << "AMLMJob::start() called on:" << this << "TWJob status:" << status();

    /// By default for now, we'll do the simplest thing and queue the TW::job up on the default TW::Queue.
    ThreadWeaver::Queue* queue = ThreadWeaver::Queue::instance();

    /// @todo: QTimer::singleShot(0, this, SLOT(doWork()));
    auto stream = queue->stream();
    start(stream);
}

void AMLMJob::start(ThreadWeaver::QueueStream &qstream)
{
    // Simply queue this TW::Job onto the given QueueStream.  Job should start immediately.
    qstream << this;
}

void AMLMJob::setSuccessFlag(bool success)
{
    qDb() << "SETTING SUCCESS/FAIL:" << success;
    m_success = success;
}

KJob::Unit AMLMJob::progressUnit() const
{
    // Shhh... you didn't see this.
//    return d_ptr->progressUnit;
    return m_progress_unit;
}

qulonglong AMLMJob::processedSize() const
{
    return processedAmount(progressUnit());
}

qulonglong AMLMJob::totalSize() const
{
    return totalAmount(progressUnit());
}

void AMLMJob::dump_job_info(KJob* kjob, const QString& header)
{
    if(!header.isEmpty())
    {
        qIn() << header;
    }
    qIn() << "INFO FOR AMLMJob:" << kjob;
    qIn() << "Progress info:";
    qIn() << "  Caps:" << kjob->capabilities();
    qIn() << "  percent:" << kjob->percent();
    for(auto unit : {KJob::Unit::Bytes, KJob::Unit::Files, KJob::Unit::Directories})
    {
        qIn() << "  processedAmount:" << kjob->processedAmount(unit);
        qIn() << "  totalAmount:" << kjob->totalAmount(unit);
    }
    qIn() << "State info:";
    qIn() << " " << M_NAME_VAL(kjob->isSuspended());
    qIn() << " " << M_NAME_VAL(kjob->isAutoDelete());
    qIn() << " " << M_NAME_VAL(kjob->error());
    if(kjob->error() != 0)
    {
        // Per KF5 docs (https://api.kde.org/frameworks/kcoreaddons/html/classKJob.html#ae0ac2567b61681f4811d128825fbcd0b),
        // "[errorString() and errorText()] Only call if error is not 0.".
        qIn() << " " << M_NAME_VAL(kjob->errorText());
        qIn() << " " << M_NAME_VAL(kjob->errorString());
    }
    else
    {
        qIn() << "  kjob->errorText(): N/A (error()==0)";
        qIn() << "  kjob->errorString(): N/A (error()==0)";
    }

    // QMetaObject info.
	const QMetaObject* metaObject = kjob->metaObject();
	auto method_count = metaObject->methodCount();
	auto first_this_method_offset = metaObject->methodOffset();
	qIn() << "All Methods (" << metaObject->methodCount() << "):";
	for(int i = 0; i < metaObject->methodCount(); ++i)
	{
		auto metamethod = metaObject->method(i);
		qIn() << " " << i << ":" << QString::fromLatin1(metamethod.methodSignature())
			<< "Type:" << metamethod.methodType()
			<< "Access:" << metamethod.access();
	}
}

void AMLMJob::defaultBegin(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
    Q_CHECK_PTR(this);
    Q_CHECK_PTR(self);

    qDb() << "ENTER defaultBegin, self/this:" << self << this;
    qDb() << "Current TW::DebugLevel:" << ThreadWeaver::Debug << ThreadWeaver::DebugLevel;

    qDb() << "TWJob status:" << status();

    /// Essentially a duplicate of QObjectDecorator's implementation, which does this:
    /// @code
    ///   Q_ASSERT(job());
    ///   Q_EMIT started(self);
    ///   job()->defaultBegin(self, thread);
    /// @endcode
    /// @link https://cgit.kde.org/threadweaver.git/tree/src/qobjectdecorator.cpp?id=a36f37705746561edf10affd77d22852076469b4
    /// We're actually in the job()->defaultBegin() part here, so we don't make that call.
    /// started() hasn't been emitted yet.

    // Make connections which we need the "real" self for.
    connections_make_defaultBegin(self, thread);

    qDb() << "EMITTING TW STARTED on TW::JobPointer:" << self;
    Q_EMIT started(self);

    // ThreadWeaver::Job::defaultBegin() does literally nothing.
    this->ThreadWeaver::Job::defaultBegin(self, thread);
}

void AMLMJob::defaultEnd(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
    // Remember that self is a QSharedPointer<ThreadWeaver::JobInterface>.

    Q_CHECK_PTR(this);
    Q_CHECK_PTR(self);

    qDb() << "ENTER defaultEnd, self/this:" << self << this;
    qDb() << "Current TW::DebugLevel:" << ThreadWeaver::Debug << ThreadWeaver::DebugLevel;

    // Cast self to an AMLMJobPtr, it should be one.
    AMLMJobPtr amlm_self = qSharedPtrToQPointerDynamicCast<AMLMJob>(self);

    // We've either completed our work or been cancelled.
    if(wasCancelRequested())
    {
        // Cancelled.
        // KJob Success == false is correct in the cancel case.
        amlm_self->setSuccessFlag(false);
        amlm_self->setWasCancelled(true);
    }
    else
    {
        // Successful completion.
        amlm_self->setSuccessFlag(true);
    }

    // Essentially a duplicate of TW::QObjectDecorator's implementation.
    /// @link https://cgit.kde.org/threadweaver.git/tree/src/qobjectdecorator.cpp?id=a36f37705746561edf10affd77d22852076469b4
    // TW::QObjectDecorator does this, and ~never calls the base class:
    //    Q_ASSERT(job());
    //    job()->defaultEnd(self, thread);
    //    if (!self->success()) {
    //        Q_EMIT failed(self);
    //    }
    //    Q_EMIT done(self);
    // job() is not self, it's the decorated job (TW::JobInterface*) passed to the IdDecorator constructor.
    // So the call to job()->defaultEnd() is the call we're currently in, so we don't call it again which would
    // infinitely recurse us.
    /// @note run() must have set the correct success() value prior to exiting.

    if(!self->success())
    {
        qWr() << objectName() << "FAILED";
        Q_EMIT /*TW::QObjectDecorator*/ failed(self);
    }
    else
    {
        qDb() << objectName() << "Succeeded";
        // @note No explicit succeeded signal.  Success is done() signal plus success() == true.
    }
    qDb() << objectName() << "EMITTING DONE";
    Q_EMIT /*TW::QObjectDecorator*/ done(self);

    // Call base class defaultEnd() implementation.
    // ThreadWeaver::Job::defaultEnd() calls:
    //   d()->freeQueuePolicyResources(job);, which loops over an array of queuePolicies and frees them.
    //   Not certain, but assume doing that here at the very end is the safest place to do this.
    this->ThreadWeaver::Job::defaultEnd(self, thread);

}

bool AMLMJob::doKill()
{
    // KJob::doKill().
    qDb() << "ENTER KJob::doKill()";

    // Kill the TW::Job.
    requestAbort();

    /// @todo I think we need to not do this, and rather wait for ::run() to abort.
    //onKJobDoKill();

    /// @todo Need to wait for the final kill here?
    /// A: Not completely clear.  It looks like KJob::kill() shouldn't return until:
    /// - finished is emitted
    /// - result is optionally emitted
    /// - deleteLater() is optionally called on the job.

    qDb() << "EXIT KJob::doKill()";

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

void AMLMJob::setProgressUnit(KJob::Unit prog_unit)
{
    /// @todo This "KJobPrivate" crap is crap.
	//    d_ptr->progressUnit = prog_unit;

	/// And if this works, it's gross.
	const QMetaObject* metaObject = this->metaObject();
	QStringList methods;
	for(int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i)
	{
	    methods << QString::fromLatin1(metaObject->method(i).methodSignature());
	}
	qDb() << methods;

//    m_progress_unit = prog_unit;
}

void AMLMJob::make_connections()
{
    qDb() << "MAKING CONNECTIONS, this:" << this;

    // @note TW::Job connections made in connections_make_defaultBegin().

    // Connect up KJob signals/slots.

    // result()
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

    /// @name Make connections to the TW::QObjectDecorator-like connections.
    /// These are the only started/ended connections between the "wrapped" ThreadWeaver::Job and
    /// the KJob.
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
}

/**
 * Break connections we can only break while in defaultExit() and have the real JobPointer.
 */
void AMLMJob::connections_break_defaultExit(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
    qDb() << "ENTER connections_break_defaultExit";
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

    qDb() << "success()?:" << success();

    // The TW::Job indicated completion.
    // If the TW::Job failed, there's a failed() signal in flight as well.

    // Convert TW::done to a KJob::result(KJob*) signal, only in the success case.
    // There could be a TW::failed() signal in flight as well, so we have to be careful we don't call KF5::emitResult() twice.
    // We'll similarly deal with the fail case in onTWFailed().
    if(/*TW::*/success())
    {
    	// Set the KJob::error() code.
        setError(NoError);

        // Tell the KJob to:
        // - Set d->isFinished
        // - Quit the d->eventLoop if applicable.
        // - emit finished(this)
        // - emit result(this)
        // - if the KJob is set to autoDelete(), call deleteLater().
        emitResult();
    }
}

void AMLMJob::onTWFailed(ThreadWeaver::JobPointer twjob)
{
    qDb() << "ENTER onTWFailed";
    Q_CHECK_PTR(twjob);

    // The TW::Job indicated failure.
    // There's a TW::done() signal in flight as well, so we have to be careful we don't call KF5::emitResult() twice.
    // Convert to a KJob result signal.

    // Shouldn't be getting into here with a non-false success.
    Q_ASSERT(twjob->success() != true);

    if(!/*TW::*/twjob->success())
    {
        // Set the KJob error info.
    	if(this->m_tw_job_was_cancelled)
        {
            // Cancelled.
    		// KJob
            setError(KilledJobError);
        }
        else
        {
            // Some other error.
            // KJob
            setError(KJob::UserDefinedError);
            setErrorText(QString("Unknown, non-Killed-Job error on ThreadWeaver job"));
        }

        // Regardless of success or fail of the TW::Job, we need to call emitResult() only once.
        // We handle the success case in done/success above, so we handle the fail case here.
        // Tell the KJob to:
		// - Set d->isFinished
		// - Quit the d->eventLoop if applicable.
		// - emit finished(this)
		// - emit result(this)
		// - if the KJob is set to autoDelete(), call deleteLater().
        emitResult();
    }
}

void AMLMJob::onKJobDoKill()
{
    qDb() << "ENTER onKJobDoKill";

    qDb() << "EXIT onKJobDoKill";
}

void AMLMJob::onKJobResult(KJob *kjob)
{
    Q_CHECK_PTR(kjob);

    /// Called when the KJob is finished.
    qDb() << "KJOB RESULT" << kjob;

    if(kjob->error())
    {
        // There was an error.
        qWr() << "KJOB ERROR:" << kjob->errorString() << kjob->errorText() << kjob->error();
    }
}

void AMLMJob::onKJobFinished(KJob *kjob)
{
    Q_CHECK_PTR(kjob);
    qDb() << "KJOB FINISHED" << kjob;
}

