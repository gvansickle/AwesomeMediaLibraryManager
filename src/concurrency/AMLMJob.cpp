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
#include <utils/RegisterQtMetatypes.h>


AMLM_QREG_CALLBACK([](){
    qIn() << "Registering KJob::Unit";
    qRegisterMetaType<KJob::Unit>();
});


AMLMJob::AMLMJob(QObject *parent) : KJob(parent)
{
    setObjectName(uniqueQObjectName());
//    setUniqueId();

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


bool AMLMJob::wasCancelRequested()
{
    // Were we told to abort?
    return get_extfuture_ref().isCanceled();
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

void AMLMJob::start()
{
    // Just let ExtAsync run the run() function, which will in turn run the runFunctor().
    // Note that we do not use the returned ExtFuture<Unit> here; that control and reporting
    // role is handled by the ExtFuture<> ref returned by get_extfuture_ref().
    // Note that calling the destructor of (by deleting) the returned future is ok:
    // http://doc.qt.io/qt-5/qfuture.html#dtor.QFuture
    // "Note that this neither waits nor cancels the asynchronous computation."
    ExtAsync::run(this, &AMLMJob::run);
}


/**
  * More Kjob notes:
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
  * - On a kill():
  * -- the kjob error will first be set to KilledJobError,
  * -- finishJob(false):
  * -- then d->isFinished = true;
  * -- then the signals are emitted:
  * -- signal finished() is always emitted
  * XX signal result() may not be emitted, if this is a kill(quietly).
  * -- if job is autodelete, deleteLater.
  *
  * - On error:
  * -- Same as normal completion but with an error code?
  *
  * - On KJob destruction, its destructor does this:
  * -- if(d->isFinished)
  *   {
  *     emit finished(this);
  *   }
  *   delete d_ptr, speedtimer, and uiDelegate.
  *
  * - signal finished() is always emitted, and always before result(), which won't be emitted on cancel.
  *
  * So:
  * - Catch finished(), check error code, and if it's KilledJobError we're being cancelled, and need to propagate the cancel.
  * - If error code is not KilledJobError, we're going to get a result() signal.  There we need to
  *   check the error code again, and it'll be either success or fail.  Fail probably looks much like cancel, need any chains
  *   to be killed.
  *
  */

void AMLMJob::run()
{
    auto ef = get_extfuture_ref();

    qDb() << "ExtFuture<> state:" << ExtFutureState::state(ef);
    if(ef.isCanceled())
    {
        // We were canceled before we were started.
        /// @note Canceling alone won't finish the extfuture.
        // Report (STARTED | CANCELED | FINISHED)
        ef.reportFinished();
        Q_ASSERT(ExtFutureState::state(ef) == (ExtFutureState::Started | ExtFutureState::Canceled | ExtFutureState::Finished));
        return;
    }
#ifdef QT_NO_EXCEPTIONS
#error "WE NEED EXCEPTIONS"
#else
    try
    {
#endif
        // Start the work.  We should be in the Running state if we're in here.
        /// @todo But we're not Running here.  Not sure why.
//        Q_ASSERT(ExtFutureState::state(ef) == (ExtFutureState::Started | ExtFutureState::Running));
        qDb() << "Pre-functor ExtFutureState:" << ExtFutureState::state(ef);
        this->runFunctor();
        qDb() << "Functor complete, ExtFutureState:" << ExtFutureState::state(ef);
    }
    catch(QException &e)
    {
        /// @note RunFunctionTask has QFutureInterface<T>::reportException(e); here.
        ef.reportException(e);
    }
    catch(...)
    {
        ef.reportException(QUnhandledException());
    }

    qDb() << "REPORTING FINISHED";
    ef.reportFinished();
    Q_ASSERT(ef.isStarted() && ef.isFinished());

    // Do the post-run work.
    qDb() << "Calling default end";
    runEnd();
}

void AMLMJob::runEnd()
{
    /// @note We're in a non-GUI worker thread here.

    auto extfutureref = get_extfuture_ref();

    Q_CHECK_PTR(this);

    qDb() << objectName() << "ENTER defaultEnd()";

    // We've either completed our work or been cancelled.
    if(wasCancelRequested())
    {
        // Cancelled.
        // KJob Success == false is correct in the cancel case.
        qDb() << "Cancelled";
        setSuccessFlag(false);
    }
    else
    {
        // Wasn't a cancel, so runFunctor() finished and should have explicitly set success/fail.
        Q_ASSERT(m_tw_job_run_reported_success_or_fail == 1);
    }

    /// @note run() must have set the correct success() value prior to exiting.

    if(!m_success)
    {
        qWr() << objectName() << "FAILED";
    }
    else
    {
        qDb() << objectName() << "Succeeded";
    }

    /// @todo Direct call to onUnderlyingAsyncJobDone(), or should we send a signal?
//        onUnderlyingAsyncJobDone(m_success);
    onUnderlyingAsyncJobDone(!extfutureref.isCanceled());
}

void AMLMJob::onUnderlyingAsyncJobDone(bool success)
{
    qDb() << "ENTER onUnderlyingAsyncJobDone";

    qDb() << "success?:" << success;

qDb() << "PARENT:" << parent();
    setKJobErrorInfo(success);

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
    /**
     * So what happens is:
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

}

bool AMLMJob::doKill()
{
    // KJob::doKill().
    /// @note The calling thread has to have an event loop.

    if(!(capabilities() & KJob::Capability::Killable))
    {
        Q_ASSERT_X(0, __func__, "Trying to kill an unkillable AMLMJob.");
    }

    // Cancel and wait for the runFunctor() to finish.
//    qDb() << "START EXTASYNC DOKILL";
    auto ef = get_extfuture_ref();
    ef.cancel();
    ef.waitForFinished();
    Q_ASSERT(ef.isStarted() && ef.isCanceled() && ef.isFinished());
//    qDb() << "END EXTASYNC DOKILL";


    // Try to detect that we've survived at least to this point.
    Q_ASSERT(!m_i_was_deleted);

    // We should never get here before the undelying job has signaled that it's done.
    Q_ASSERT(ExtFutureState::state(ef) == (ExtFutureState::Started | ExtFutureState::Canceled | ExtFutureState::Finished));

    return true;

    /// @warning At any point after we return here, this may have been deleteLater()'ed by KJob::finishJob().
}

bool AMLMJob::doSuspend()
{
    /// KJob::doSuspend().
    Q_ASSERT_X(capabilities() & KJob::Capability::Suspendable, __func__, "Trying to suspend an unsuspendable AMLMJob.");
    get_extfuture_ref().setPaused(true);
    return true;
}

bool AMLMJob::doResume()
{
    /// KJob::doResume().
    Q_ASSERT_X(capabilities() & KJob::Capability::Suspendable, __func__, "Trying to resume an unresumable AMLMJob.");
    get_extfuture_ref().setPaused(false);
    return true;
}

void AMLMJob::setProcessedAmountAndSize(KJob::Unit unit, qulonglong amount)
{
    if(m_progress_unit != KJob::Unit::Bytes && unit == m_progress_unit)
    {
        // Unit wasn't the progress unit, so also set Bytes so we get percent complete support.
        setProcessedAmount(KJob::Unit::Bytes, amount);
    }
    setProcessedAmount(unit, amount);
}

void AMLMJob::setTotalAmountAndSize(KJob::Unit unit, qulonglong amount)
{
    if(m_progress_unit != KJob::Unit::Bytes && unit == m_progress_unit)
    {
        // Unit wasn't the progress unit, so also set Bytes so we get percent complete support.
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

void AMLMJob::setKJobErrorInfo(bool success)
{
    // We're still in the underlying ExtAsync::run() context and may not have an event loop here.
    /// @note GRVS: Threadsafety not clear.  KJob doesn't do any locking FWICT around these variable sets.
//    AMLM_ASSERT_IN_GUITHREAD();

    // Convert underlying finished to a KJob::result(KJob*) signal, but only in the success case.
    // We have to be careful we don't call KF5::emitResult() twice.
    // We'll similarly deal with the fail case in onTWFailed().
    if(success)
    {
        // Set the KJob::error() code.
        setError(NoError);
    }
    else
    {
        // Set the KJob error info.
        if(get_extfuture_ref().isCanceled())
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





