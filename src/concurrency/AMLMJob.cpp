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

#include <config.h>

#include "AMLMJob.h"

/// Qt5
#include <QPointer>

/// KF5
#include <KJob>
#include <KJobWidgets>
#include <KDialogJobUiDelegate>

/// Ours
#include "utils/DebugHelpers.h"
#include "utils/UniqueIDMixin.h"
#include "utils/QtCastHelpers.h"
#include "utils/TheSimplestThings.h"
#include <gui/MainWindow.h>


AMLMJob::AMLMJob(QObject *parent) : KJob(parent)
{
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


///**
// * KJob override.
// *
// * "Subclasses must implement start(), which should trigger the execution of the job (although the work should be done
// *  asynchronously)."
// *
// * @note Per comments, KF5 KIO::Jobs autostart; this is overridden to be a no-op.
// */
//void AMLMJob::start()
//{
//#if 0
//    // Lock the mutex.
//    QMutexLocker lock(&m_cancel_pause_resume_mutex); // == std::unique_lock<std::mutex> lock(m_mutex);

//    // Have we been cancelled before we started?
//    if(m_flag_cancel)
//    {
//        // Yes, fake a "done()" signal FBO doKill().
//        // KJob Success == false is correct in the cancel case.
//        setSuccessFlag(false);
//        setWasCancelled(true);
//        Q_EMIT done(qSharedPointerDynamicCast<ThreadWeaver::JobInterface>(this));
//    }
//#endif

//    Q_ASSERT(!m_use_extasync);

//    /// Kjob::setAutoDelete()
//    setAutoDelete(false);
//    Q_ASSERT_X(!isAutoDelete(), __PRETTY_FUNCTION__, "AMLMJob needs to not be autoDelete");

//    /// @note The TW::Job starts as soon as it's added to a TW::Queue/Weaver.

////    qDb() << "AMLMJob::start() called on:" << this << "TWJob status:" << status();
////    /// By default for now, we'll do the simplest thing and queue the TW::job up on the default TW::Queue.
////    ThreadWeaver::Queue* queue = ThreadWeaver::Queue::instance();
////    auto stream = queue->stream();
////    start(stream);
//}

bool AMLMJob::wasCancelRequested()
{
    QMutexLocker lock(&m_cancel_pause_resume_mutex); // == std::unique_lock<std::mutex> lock(m_mutex);

    // Were we told to abort?
    if(get_future_ref().isCanceled())
    {
        return true;
    }

    return false;
}

void AMLMJob::setSuccessFlag(bool success)
{
    /// Called from underlying ExtAsync thread.
    qDb() << "SETTING SUCCESS/FAIL:" << success;
    m_success = success;
    m_tw_job_run_reported_success_or_fail = 1;
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

void AMLMJob::defaultEnd()
{
	/// @note We're in a non-GUI worker thread here.

    auto extfutureref = get_future_ref();

M_WARNING("SHOULD MAKE USE OF extfutureref.status() somewhere, Status_Success,_RUNNING,_Failed,etc");

    Q_CHECK_PTR(this);

qDb() << "ENTER defaultEnd()";

        // We've either completed our work or been cancelled.
        if(wasCancelRequested())
        {
            // Cancelled.
            // KJob Success == false is correct in the cancel case.
            qDb() << "Cancelled";
            setSuccessFlag(false);
            setWasCancelled(true);
        }
        else
        {
            // Wasn't a cancel, so run()/worker_function() finished and should have explicitly set success/fail.
            Q_ASSERT(m_tw_job_run_reported_success_or_fail == 1);
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

        // Call base class defaultEnd() implementation.
        /// @note Vestige of ThreadWeaver support for clearing queuePolicies, probably no longer needed for anything.

        if(!m_success)
        {
            qWr() << objectName() << "FAILED";
//            Q_EMIT /*TW::QObjectDecorator*/ failed(self);
        }
        else
        {
            qDb() << objectName() << "Succeeded";
            // @note No explicit TW::succeeded signal.  Success is TW::done() signal plus TW::success() == true.
        }

//        qDb() << objectName() << "EMITTING DONE";

        // Flag that the TW::Job is finished.
        m_tw_job_is_done = 1;

        /// @todo Direct call to onUnderlyingAsyncJobDone(), or should we send a signal?
//        onUnderlyingAsyncJobDone(m_success);
        onUnderlyingAsyncJobDone(!extfutureref.isCanceled());
}

bool AMLMJob::doKill()
{
    // KJob::doKill().
    /// @note The calling thread has to have an event loop.

    if(!(capabilities() & KJob::Capability::Killable))
    {
        Q_ASSERT_X(0, __func__, "Trying to kill an unkillable AMLMJob.");
    }

    qDb() << "START EXTASYNC DOKILL";
    auto ef = get_future_ref();
    ef.cancel();
    ef.waitForFinished();
    qDb() << "END EXTASYNC DOKILL";


    // Try to detect that we've survived at least to this point.
    Q_ASSERT(!m_i_was_deleted);

    // We should never get here before the TW::Job has signaled that it's done.
M_WARNING("TODO: got_done is never set by anything, cancelled is set by defaultEnd() but comes up 0 here.");
//    qDb() << M_NAME_VAL(m_tw_flag_cancel) << M_NAME_VAL(m_tw_job_is_done) << M_NAME_VAL(m_tw_job_was_cancelled);
//    throwif(!!(m_tw_flag_cancel && !m_tw_job_is_done && !m_tw_job_was_cancelled));

    return true;
}

bool AMLMJob::doSuspend()
{
    /// @todo // KJob::doSuspend().
    qDb() << "DOSUSPEND";
    Q_ASSERT_X(capabilities() & KJob::Capability::Suspendable, __func__, "Trying to suspend an unsuspendable AMLMJob.");
    get_future_ref().setPaused(true);
    return true;
}

bool AMLMJob::doResume()
{
    /// @todo // KJob::doResume().
    qDb() << "TODO: DORESUME";
    Q_ASSERT_X(capabilities() & KJob::Capability::Suspendable, __func__, "Trying to resume an unresumable AMLMJob.");
    get_future_ref().setPaused(false);
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

//void AMLMJob::make_connections()
//{
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

    // finished()
    // Emitted by calling emitResult() and kill().
    // Intended to notify UIs that should detach from the job.
    /// @todo This event fires and gets to AMLMJob::onKJobFinished() after this has been destructed.
//    connect(this, &KJob::finished, this, &AMLMJob::onKJobFinished);

//#error "BOTH THE ABOVE NEED TO BE RETHOUGHT"

//    qDb() << "MADE CONNECTIONS, this:" << this;
//}

void AMLMJob::KJobCommonDoneOrFailed(bool success)
{
    // We're out of the TW context and in a context with an event loop here.
    /// Not sure if that matters....
    AMLM_ASSERT_IN_GUITHREAD();

    // Convert TW::done to a KJob::result(KJob*) signal, only in the success case.
    // There could be a TW::failed() signal in flight as well, so we have to be careful we don't call KF5::emitResult() twice.
    // We'll similarly deal with the fail case in onTWFailed().
    if(success)
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
            setErrorText(QString("Unknown, non-Killed-Job error on AMLMJob: %1").arg(this->objectName()));
        }
    }
}

void AMLMJob::onUnderlyingAsyncJobDone(bool success)
{
    qDb() << "ENTER onUnderlyingAsyncJobDone";

    qDb() << "success?:" << success;

    // The TW::Job indicated completion.
    // If the TW::Job failed, there's a failed() signal in flight as well.
qDb() << "PARENT:" << parent();
    KJobCommonDoneOrFailed(success);

    // Regardless of success or fail of the underlying job, we need to call KJob::emitResult() only once.
    // We handle both success and fail cases here, since we always should get a ::done() event.
    // Tell the KJob to:
    // - Set d->isFinished
    // - Quit the d->eventLoop if applicable.
    // - emit finished(this)
    // - emit result(this)
    // - if the KJob is set to autoDelete(), call deleteLater().
    qDb() << "ABOUT TO EMITRESULT():" << this << "isAutoDelete?:" << isAutoDelete();
M_WARNING("ASSERTS HERE IF NO FILES FOUND.");
    emitResult();

    qDb() << "EXIT onUnderlyingAsyncJobDone";
    /// @fixme
//#error "ON CANCEL, THINGS START TO FAIL HERE.  WE ARE IMMEDIATELY DESTRUCTED FOR SOME REASON."
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

    /////////////////
    /**
      * More Kjob notes:
      *
      * - On a kill():
      * -- the kjob error will first be set to KilledJobError,
      * -- finishJob(false):
      * -- then d->isFinished = true;
      * -- then the signals are emitted:
      * -- signal finished() is always emitted
      * XX signal result() may not be emitted, if this is a kill(quietly).
      * -- if job is autodelete, deleteLater.
      *
      * - On normal completion:
      * -- Shoudn't be any kjob error.
      * -- finishJob(true):
      * -- then d->isFinished = true;
      * -- then the signals are emitted.
      * -- signal finished() is always emitted
      * ++ signal result() is emitted()
      * -- if job is autodelete, deleteLater.
      *
      * - On error:
      * -- Same as normal completion but with an error code?
      *
      * - signal finished() is always emitted, and always before result(), which won't be emitted on cancel.
      *
      * So:
      * - Catch finished(), check error code, and if it's KilledJobError we're being cancelled, and need to propagate the cancel.
      * - If error code is not KilledJobError, we're going to get a result() signal.  There we need to
      *   check the error code again, and it'll be either success or fail.  Fail probably looks much like cancel, need any chains
      *   to be killed.
      *
      * */
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



