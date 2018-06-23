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
#include <KDialogJobUiDelegate>
#include <QPointer>

/// KF5
#include <ThreadWeaver/DebuggingAids>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/QObjectDecorator>
#include <ThreadWeaver/Queue>
#include <KJob>
#include <KJobWidgets>

/// Ours
#include "utils/DebugHelpers.h"
#include "utils/UniqueIDMixin.h"
#include "utils/QtCastHelpers.h"
#include "utils/TheSimplestThings.h"
#include <gui/MainWindow.h>

/**
 * Object life cycles
 *
 * External     AMLMJob     KJob        TW::Job
 *	--start-->   start()
 */

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
}

AMLMJob::~AMLMJob()
{
    // The KJob should have finished/been killed before we get deleted.
//    Q_ASSERT(isFinished());

    Q_ASSERT(!m_i_was_deleted);
    m_i_was_deleted = true;

    // KJob destructor checks if KJob->isFinished and emits finished(this) if so.
    // Doesn't cancel the job.
    qDb() << "AMLMJob DELETED" << this;
}

bool AMLMJob::success() const
{
    return m_success;
}


void AMLMJob::requestAbort()
{
    // Using a mutex/condition variable combo to allow both abort and pause/resume.
    // This is sort of easier with C++11+, but it's Qt5, so....

    /**
     * @todo There's still something wrong between this and requestAbort() and doKill() and
     * I don't know what all else.  We get multiple of these from a single cancel button push,
     * and the TW::Job doesn't actually end until much later (after several slotStop()s).
     * Similar with KIO::Jobs.
     */

    // Lock the mutex.
    QMutexLocker lock(&m_cancel_pause_resume_mutex); // == std::unique_lock<std::mutex> lock(m_mutex);

    qDb() << "AMLM:TW: SETTING CANCEL FLAG ON AMLMJOB:" << this;

    Q_ASSERT_X(!isAutoDelete(), __PRETTY_FUNCTION__, "AMLMJob needs to not be autoDelete");

    // Signal to the run() loop that it should cancel.
    m_flag_cancel = true;

    // Unlock the mutex immediately prior to notify.  This prevents a waiting thread from being immediately woken up
    // by the notify, and only to temporarily block again because we still hold the mutex.
	lock.unlock(); // == lock.unlock(); (ok, so that one's the same, still...)

	// Notify all threads waiting on the condition variable that there's new status to look at.
    // Really only one thread might be watching (in doKill()), but not much difference here.
    m_cancel_pause_resume_waitcond.notify_all(); // == m_cv.notify_all(); (...well, it's about time Qt learned some modern C++.)

    qDb() << "AMLM:TW: LEAVING requestAbort():" << this;
}

/**
 * KJob override.
 *
 * "Subclasses must implement start(), which should trigger the execution of the job (although the work should be done
 *  asynchronously)."
 *
 * @note Per comments, KF5 KIO::Jobs autostart; this is overridden to be a no-op.
 */
void AMLMJob::start()
{
#if 0
    // Lock the mutex.
    QMutexLocker lock(&m_cancel_pause_resume_mutex); // == std::unique_lock<std::mutex> lock(m_mutex);

    // Have we been cancelled before we started?
    if(m_flag_cancel)
    {
        // Yes, fake a "done()" signal FBO doKill().
        // KJob Success == false is correct in the cancel case.
        setSuccessFlag(false);
        setWasCancelled(true);
        Q_EMIT done(qSharedPointerDynamicCast<ThreadWeaver::JobInterface>(this));
    }
#endif

    /// Kjob::setAutoDelete()
    setAutoDelete(false);
    Q_ASSERT_X(!isAutoDelete(), __PRETTY_FUNCTION__, "AMLMJob needs to not be autoDelete");

    /// @note The TW::Job starts as soon as it's added to a TW::Queue/Weaver.

    qDb() << "AMLMJob::start() called on:" << this << "TWJob status:" << status();
    /// By default for now, we'll do the simplest thing and queue the TW::job up on the default TW::Queue.
    ThreadWeaver::Queue* queue = ThreadWeaver::Queue::instance();
    auto stream = queue->stream();
    start(stream);
}

void AMLMJob::start(ThreadWeaver::QueueStream &qstream)
{
    // Simply queue this TW::Job onto the given QueueStream.  Job should start immediately.
    qstream << this;
}

bool AMLMJob::wasCancelRequested()
{
    QMutexLocker lock(&m_cancel_pause_resume_mutex); // == std::unique_lock<std::mutex> lock(m_mutex);

    // Were we told to abort?
    if(m_flag_cancel)
    {
        return true;
    }

    // Wait if we have to.
    // Well, here we don't have to.  Until we add pause/resume, we just have an expensive abort flag.
//    m_cancel_pause_resume_waitcond.wait(lock.mutex());
    return false;
}

void AMLMJob::setSuccessFlag(bool success)
{
    qDb() << "SETTING SUCCESS/FAIL:" << success;
    m_success = success;
}

qulonglong AMLMJob::totalSize() const
{
    return totalAmount(progressUnit());
}

qulonglong AMLMJob::processedSize() const
{
    return processedAmount(progressUnit());
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
	/// @note We're in a non-GUI worker thread here.
    Q_CHECK_PTR(this);
    Q_CHECK_PTR(self);

    Q_ASSERT_X(!isAutoDelete(), __PRETTY_FUNCTION__, "AMLMJob needs to not be autoDelete");

    qDb() << "ENTER defaultBegin, self/this:" << self << this;

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
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void AMLMJob::defaultEnd(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread)
{
	/// @note We're in a non-GUI worker thread here.
    // Remember that self is a QSharedPointer<ThreadWeaver::JobInterface>.

    Q_CHECK_PTR(this);
    Q_CHECK_PTR(self);
    Q_ASSERT_X(!isAutoDelete(), __PRETTY_FUNCTION__, "AMLMJob needs to not be autoDelete");

    qDb() << "ENTER defaultEnd, self/this:" << self << this;

    // We've either completed our work or been cancelled.
    if(wasCancelRequested())
    {
        // Cancelled.
        // KJob Success == false is correct in the cancel case.
        setSuccessFlag(false);
        setWasCancelled(true);
    }
    else
    {
        // Successful completion.
        setSuccessFlag(true);
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

    Q_ASSERT_X(!isAutoDelete(), __PRETTY_FUNCTION__, "AMLMJob needs to not be autoDelete");

    qDb() << "START OF BASE IMPL";

    // Call base class defaultEnd() implementation.
    // ThreadWeaver::Job::defaultEnd() calls:
    //   d()->freeQueuePolicyResources(job);, which loops over an array of queuePolicies and frees them.
    ThreadWeaver::Job::defaultEnd(self, thread);

    if(!self->success())
    {
        qWr() << objectName() << "FAILED";
        Q_EMIT /*TW::QObjectDecorator*/ failed(self);
    }
    else
    {
        qDb() << objectName() << "Succeeded";
        // @note No explicit TW::succeeded signal.  Success is TW::done() signal plus TW::success() == true.
    }

    Q_ASSERT_X(!isAutoDelete(), __PRETTY_FUNCTION__, "AMLMJob needs to not be autoDelete");

    qDb() << objectName() << "EMITTING DONE";

    Q_EMIT /*TW::QObjectDecorator*/ done(self);
}

bool AMLMJob::doKill()
{
    // KJob::doKill().
    qDb() << "ENTER KJob::doKill()";

    Q_ASSERT_X(!isAutoDelete(), __PRETTY_FUNCTION__, "AMLMJob needs to not be autoDelete");

    /**
     * @todo There's still something wrong between this and requestAbort() and doKill() and
     * I don't know what all else.  We get multiple of these from a single cancel button push,
     * and the TW::Job doesn't actually end until much later (after several slotStop()s).
     * Similar with KIO::Jobs.
     */

//
//    connect(this, &AMLMJob::done, &local_event_loop, &QEventLoop::quit);

    /// @note The calling thread has to have an event loop.
//    connect_blocking_or_die(this, INTERNAL_SIGNAL_requestAbort, , );

    // Tell the TW::Job to stop.
    requestAbort();

qDb() << "START WAIT:" << objectName();
    // Now wait for it to signal that it really did stop.

Q_ASSERT_X(!isAutoDelete(), __PRETTY_FUNCTION__, "AMLMJob needs to not be autoDelete");

//    sleep(5);
    auto loop = new QEventLoop();
    connect_or_die(this, &AMLMJob::done, loop, &QEventLoop::quit);
    loop->exec();
    qDb() << "WAIT: BROKE OUT OF LOOP";
M_WARNING("WE NEVER GET PAST THIS POINT, looks like we've been deleted just before the above qDb()");
    loop->deleteLater();

Q_ASSERT_X(!isAutoDelete(), __PRETTY_FUNCTION__, "AMLMJob needs to not be autoDelete");


qDb() << "END WAIT:" << objectName();

    /// @todo Need to wait for the final kill here?
    /// A: Not completely clear.  It looks like KJob::kill() shouldn't return until:
    /// - finished is emitted
    /// - result is optionally emitted
    /// - deleteLater() is optionally called on the job.

    qDb() << "EXIT KJob::doKill()";

    // Try to detect that we've survived at least to this point.
    Q_ASSERT(!m_i_was_deleted);

    // We should never get here before the TW::Job has signaled that it's done.
    qDb() << M_NAME_VAL(m_flag_cancel) << M_NAME_VAL(m_tw_got_done_or_fail) << M_NAME_VAL(m_tw_job_was_cancelled);
    Q_ASSERT(!(m_flag_cancel && !m_tw_got_done_or_fail && !m_tw_job_was_cancelled));

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

void AMLMJob::setProcessedAmountAndSize(KJob::Unit unit, qulonglong amount)
{
    if(m_progress_unit != KJob::Unit::Bytes && unit == m_progress_unit)
    {
        // Wasn't Bytes, also set Bytes so we get percent complete support.
        setProcessedAmount(KJob::Unit::Bytes, amount);
    }
    setProcessedAmount(unit, amount);
}

void AMLMJob::setTotalAmountAndSize(KJob::Unit unit, qulonglong amount)
{
    if(m_progress_unit != KJob::Unit::Bytes && unit == m_progress_unit)
    {
        // Wasn't Bytes, also set Bytes so we get percent complete support.
        setTotalAmount(KJob::Unit::Bytes, amount);
    }
    setTotalAmount(unit, amount);
}

void AMLMJob::setProgressUnit(KJob::Unit prog_unit)
{
#ifdef THIS_IS_EVER_NOT_BROKEN
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
#endif
    m_progress_unit = prog_unit;
}

KJob::Unit AMLMJob::progressUnit() const
{
    return m_progress_unit;
}

void AMLMJob::make_connections()
{
    //    qDb() << "MAKING CONNECTIONS, this:" << this;

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
//    connect(this, &KJob::result, this, &AMLMJob::onKJobResult);

//    // Emitted by calling emitResult() and kill().
//    // Intended to notify UIs that should detach from the job.
//    /// @todo This event fires and gets to AMLMJob::onKJobFinished() after this has been destructed.
//    connect(this, &KJob::finished, this, &AMLMJob::onKJobFinished);

//#error "BOTH THE ABOVE NEED TO BE RETHOUGHT"

//    qDb() << "MADE CONNECTIONS, this:" << this;
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

void AMLMJob::TWCommonDoneOrFailed(ThreadWeaver::JobPointer twjob)
{
    // We're out of the TW context and in a context with an event loop here.
    AMLM_ASSERT_IN_GUITHREAD();

    // Convert TW::done to a KJob::result(KJob*) signal, only in the success case.
    // There could be a TW::failed() signal in flight as well, so we have to be careful we don't call KF5::emitResult() twice.
    // We'll similarly deal with the fail case in onTWFailed().
    if(/*TW::*/twjob->success())
    {
        // Set the KJob::error() code.
        setError(NoError);
    }
    else
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
    }
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
    Q_ASSERT_X(!isAutoDelete(), __PRETTY_FUNCTION__, "AMLMJob needs to not be autoDelete");

    qDb() << "success()?:" << success();

    // The TW::Job indicated completion.
    // If the TW::Job failed, there's a failed() signal in flight as well.

    TWCommonDoneOrFailed(twjob);

    Q_ASSERT_X(!isAutoDelete(), __PRETTY_FUNCTION__, "AMLMJob needs to not be autoDelete");

    // Regardless of success or fail of the TW::Job, we need to call emitResult() only once.
    // We handle both success and fail cases here, since we always should get a ::done() event.
    // Tell the KJob to:
    // - Set d->isFinished
    // - Quit the d->eventLoop if applicable.
    // - emit finished(this)
    // - emit result(this)
    // - if the KJob is set to autoDelete(), call deleteLater().
    qDb() << "ABOUT TO EMITRESULT()" << this;
M_WARNING("ASSERTS HERE IF NO FILES FOUND.");
    emitResult();

    Q_ASSERT_X(!isAutoDelete(), __PRETTY_FUNCTION__, "AMLMJob needs to not be autoDelete");


    qDb() << "EXIT onTWDone";
    /// @fixme
#error "ON CANCEL, THINGS START TO FAIL HERE.  WE ARE IMMEDIATELY DESTRUCTED FOR SOME REASON."
    /**
[22:30:34.689 GUIThread______ DEBUG] AMLMJob::onTWFailed:658 - ENTER onTWFailed
[22:30:34.689 GUIThread______ DEBUG] AMLMJob::onTWDone:561 - ENTER onTWDone
[22:30:34.690 GUIThread______ DEBUG] AMLMJob::onTWDone:565 - success()?: false
[22:30:34.690 GUIThread______ DEBUG] AMLMJob::onTWDone:582 - ABOUT TO EMITRESULT() DirectoryScannerAMLMJob(0x60d000abfa60, name = "DirectoryScannerAMLMJob_0")
[22:30:34.690 GUIThread______ DEBUG] ActivityProgressStatusBarTracker::INTERNAL_unregisterJob:456 - UNREGISTERING JOB: DirectoryScannerAMLMJob(0x60d000abfa60, name = "DirectoryScannerAMLMJob_0")
[22:30:34.690 GUIThread______ DEBUG] ActivityProgressStatusBarTracker::INTERNAL_unregisterJob:471 - SIGNALS DISCONNECTED: DirectoryScannerAMLMJob(0x60d000abfa60, name = "DirectoryScannerAMLMJob_0")
[22:30:34.690 GUIThread______ DEBUG] ActivityProgressStatusBarTracker::removeJobAndWidgetFromMap:487 - REMOVING FROM MAP: DirectoryScannerAMLMJob(0x60d000abfa60, name = "DirectoryScannerAMLMJob_0") BaseActivityProgressStatusBarWidget(0x60d000abfc00)
[22:30:34.690 GUIThread______ DEBUG] ActivityProgressStatusBarTracker::INTERNAL_unregisterJob:482 - JOB UNREGISTERED: DirectoryScannerAMLMJob(0x60d000abfa60, name = "DirectoryScannerAMLMJob_0")
[22:30:34.690 GUIThread______ DEBUG] AMLMJob::onTWDone:589 - EXIT onTWDone
[22:30:34.690 GUIThread______ DEBUG] DirectoryScannerAMLMJob::~DirectoryScannerAMLMJob:56 - DirectoryScannerAMLMJob DELETED: DirectoryScannerAMLMJob(0x60d000abfa60, name = "DirectoryScannerAMLMJob_0")
[22:30:34.690 GUIThread______ DEBUG] AMLMJob::~AMLMJob:79 - AMLMJob DELETED AMLMJob(0x60d000abfa60, name = "DirectoryScannerAMLMJob_0")
[22:30:34.693 GUIThread______ DEBUG] AMLMJob::doKill:371 - WAIT: BROKE OUT OF LOOP
     */
    /**
     * So what happens is:
     * 1 - We leave here
     * 2 - the AMLMJob gets deleted
     * 3*** - The doKill() event loop exits, which then allows KJob::kill() to continue, even though we now have no object.
     * - So then it's this:
     *
     * if (doKill()) {
        setError(KilledJobError);

        finishJob(verbosity != Quietly);
        return true;
    } else {
        return false;
    }

    ... and this (which is also called directly by emitResult()):

void KJob::finishJob(bool emitResult)
{
    Q_D(KJob);
    d->isFinished = true;

    if (d->eventLoop) {
        d->eventLoop->quit();
    }

    // If we are displaying a progress dialog, remove it first.
    emit finished(this, QPrivateSignal());

    if (emitResult) {
        emit result(this, QPrivateSignal());
    }

    if (isAutoDelete()) {
        deleteLater();
    }
}

So I think the answer is:
 - We need this object to survive doKill(),
 - We can't do anything else after that, due to finishJob() possibly destroying us.
     */
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

    TWCommonDoneOrFailed(twjob);
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
    /**
     * @todo There's still something wrong between this and requestAbort() and doKill() and
     * I don't know what all else.  We get multiple of these from a single cancel button push,
     * and the TW::Job doesn't actually end until much later (after several slotStop()s).
     * Similar with KIO::Jobs.
     */

    Q_CHECK_PTR(kjob);
    qDb() << "KJOB FINISHED" << kjob;
}

