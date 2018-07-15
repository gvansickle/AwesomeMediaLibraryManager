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

    qDbo() << M_NAME_VAL(this);

    /// @todo This is sort of horrible, we should find a just-in-time way to do the uiDelegate.
    /// ...and also, while this prevents crashes, we don't get any dialog etc. output on fail.
    /// So not at all clear what's happening here.
    KJobWidgets::setWindow(this, MainWindow::instance());
    setUiDelegate(new KDialogJobUiDelegate());

//    connect_or_die(app??, &??::SIGNAL_aboutToShutDown, this, &AMLMJob::SLOT_extfuture_aboutToShutdown);
}

AMLMJob::~AMLMJob()
{
    // The ExtAsync job should not be running here.
    /// @todo Except we can't check it here, since get_extfuture_ref()is pure virtual,
    /// and the derived class' destructor will have already been called.
//    Q_ASSERT(this->get_extfuture_ref().isRunning() == false);

    // The KJob should have finished/been killed before we get deleted.
//    Q_ASSERT(isFinished());

    Q_ASSERT(!m_i_was_deleted);
    m_i_was_deleted = true;

    // KJob destructor checks if KJob->isFinished and emits finished(this) if so.
    // Doesn't cancel the job.
    qDbo() << "AMLMJob DELETED" << this;
}


bool AMLMJob::wasCancelRequested()
{
//    Q_ASSERT(!m_possible_delete_later_pending);
    Q_ASSERT(!m_i_was_deleted);

    // Were we told to abort?
    return asDerivedTypePtr()->get_extfuture_ref().isCanceled();
}

void AMLMJob::setSuccessFlag(bool success)
{
    //Q_ASSERT(!m_possible_delete_later_pending);

    /// Called from underlying ExtAsync thread.
    qDbo() << "SETTING SUCCESS/FAIL:" << success;
    m_success = success;
    m_tw_job_run_reported_success_or_fail = 1;
}

qulonglong AMLMJob::totalSize() const
{
    //Q_ASSERT(!m_possible_delete_later_pending);

    return totalAmount(progressUnit());
}

qulonglong AMLMJob::processedSize() const
{
    //Q_ASSERT(!m_possible_delete_later_pending);

    return processedAmount(progressUnit());
}

void AMLMJob::start()
{
    QMutexLocker lock(&m_start_vs_cancel_mutex);
//    m_watcher = new QFutureWatcher<void>(this);
//    auto& ef = this->get_extfuture_ref();
//    connect_or_die(m_watcher, &QFutureWatcher<void>::finished, this, &AMLMJob::SLOT_extfuture_finished);
//    connect_or_die(m_watcher, &QFutureWatcher<void>::canceled, this, &AMLMJob::SLOT_extfuture_canceled);

    // Just let ExtAsync run the run() function, which will in turn run the runFunctor().
    // Note that we do not use the returned ExtFuture<Unit> here; that control and reporting
    // role is handled by the ExtFuture<> ref returned by get_extfuture_ref().
    // Note that calling the destructor of (by deleting) the returned future is ok:
    // http://doc.qt.io/qt-5/qfuture.html#dtor.QFuture
    // "Note that this neither waits nor cancels the asynchronous computation."
    /// Indicate to the cancel logic that we did start.
    m_run_was_started.release();
    ExtAsync::run(asDerivedTypePtr(), &AMLMJob::run);
//    m_watcher->setFuture(this->get_extfuture_ref());
}


/**
 * KJob completion notes:
 *
 * - On normal completion:
 * - Subclass calls emitResult(), which directly calls finishJob(true).
 * -- Shoudn't be any kjob error.
 * -- finishJob(true):
 * -- then d->isFinished = true;
 * -- then the signals are emitted.
 * -- signal finished() is always emitted
 * ++ signal result() is emitted()
 * -- if job is autodelete, deleteLater.
 *
 * - On a KJob::kill():
 * -- the kjob error will first be set to KilledJobError,
 * -- finishJob(emitResult = (verbosity != Quietly)):
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
 * - Regarding QObject::deleteLater():
 * -- @link http://doc.qt.io/qt-5/qobject.html#deleteLater
 *    "if deleteLater() is called on an object that lives in a thread with no running event loop, the object
 *    will be destroyed when the thread finishes."
 *
 * So:
 * - Catch finished(), check error code, and if it's KilledJobError we're being cancelled, and need to propagate the cancel.
 * - If error code is not KilledJobError, we're going to get a result() signal.  There we need to
 *   check the error code again, and it'll be either success or fail.  Fail probably looks much like cancel, need any chains
 *   to be killed.
 *
 */



/**
 * qt-creator FileSearch functor (inherits from nothing) has this structure:
 *
 * FileSearch::operator()(QFutureInterface<FileSearchResultList> &futureInterface,...
 * {
 *  if (futureInterface.isCanceled())
        return;
    if(start conditions fail)
    {
        futureInterface.cancel(); // failure
        return;
    }
    while(not at end)
    {
        // Do work

        // At end of loop do this:
        if (futureInterface.isPaused())
            futureInterface.waitForResume();
        if (futureInterface.isCanceled())
            break;
    }
    if (file.isOpen())
        file.close();
    if (!futureInterface.isCanceled()) {
        futureInterface.reportResult(results);
        futureInterface.setProgressValue(1);
    }
 * }
 *
 * ...except they call it with mapReduce().
 */
/// @note Canceling alone won't finish the extfuture, so we finish it manually here.
/// I think this is the right thing to do.
///
/// QtCreator::runextensions.h::AsyncJob::run() does the same thing here:
/// @code
/// if (futureInterface.isCanceled())
/// {
///     futureInterface.reportFinished();
///     return;
/// }
/// runHelper(...);
/// @endcode
///
/// QFIBase::reportFinished() does nothing if we're already isFinished(), else does:
/// @code
/// 	switch_from_to(d->state, Running, Finished);
/// 	d->waitCondition.wakeAll();
/// 	d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Finished));
/// @endcode
/// QFutureInterface<T>::reportFinished(const T* result) adds a possible reportResult() call:
/// @code
/// if (result)
///    reportResult(result);
/// QFutureInterfaceBase::reportFinished();
/// @endcode

void AMLMJob::run()
{
    /// @note We're in an arbitrary thread here probably without an event loop.

    /// @note void QThreadPoolThread::run() has a similar construct, and wraps this whole thing in:
    /// QMutexLocker locker(&manager->mutex);

    m_run_was_started.release();

    auto& ef = asDerivedTypePtr()->get_extfuture_ref();

    qDbo() << "ExtFuture<> state:" << ExtFutureState::state(ef);
    if(ef.isCanceled())
    {
        // We were canceled before we were started.
        /// @note Canceling alone won't finish the extfuture, so we finish it manually here.
        // Report (STARTED | CANCELED | FINISHED)
        ef.reportFinished();
        AMLM_ASSERT_EQ(ExtFutureState::state(ef), (ExtFutureState::Started | ExtFutureState::Canceled | ExtFutureState::Finished));
        return;
    }
#ifdef QT_NO_EXCEPTIONS
#error "WE NEED EXCEPTIONS"
#else
    try
    {
#endif
        // Start the work by calling the functor.  We should be in the Running state if we're in here.
        /// @todo But we're not Running here.  Not sure why.
//        AMLM_ASSERT_EQ(ef.isRunning(), true);
        qDbo() << "Pre-functor ExtFutureState:" << ExtFutureState::state(ef);
        asDerivedTypePtr()->runFunctor();
        qDbo() << "Functor complete, ExtFutureState:" << ExtFutureState::state(ef);
    }
    catch(QException &e)
    {
        /// @note RunFunctionTask has QFutureInterface<T>::reportException(e); here.
        ef.reportException(e);
    }
    catch(...)
    {
        /// @note RunFunctionTask has QFutureInterface<T>::reportException(e); here.
        ef.reportException(QUnhandledException());
    }

    /// QThreadPoolThread::run():
    /// locker.relock();

    /// @note Ok, runFunctor() has either completed successfully, been canceled, or thrown an exception, so what do we do here?
    /// QtCreator::runextensions.h::AsyncJob::run() calls runHelper(), which then does this here:
    /// @code
    /// // invalidates data, which is moved into the call
    /// runAsyncImpl(futureInterface, std::move(std::get<index>(data))...); // GRVS: The runFunctor() above.
    /// if (futureInterface.isPaused())
    ///         futureInterface.waitForResume();
    /// futureInterface.reportFinished();
    /// @endcode
    /// So it seems we should be safe doing the same thing.

    if(ef.isPaused())
    {
        // ExtAsync<> is paused, so wait for it to be resumed.
        qWro() << "ExtAsync<> is paused, waiting for it to be resumed....";
        ef.waitForResume();
    }

    qDbo() << "REPORTING FINISHED";
    ef.reportFinished();

    // We should only have two possible states here, excl. exceptions for the moment:
    // - Started | Finished
    // - Started | Canceled | Finished if job was canceled.
    AMLM_ASSERT_EQ(ef.isStarted(), true);
    AMLM_ASSERT_EQ(ef.isFinished(), true);

    // Do the post-run work.
    qDbo() << "Calling runEnd()";
///////////////
//    // We've either completed our work or been cancelled.
//    if(wasCancelRequested())
//    {
//        // Cancelled.
//        // KJob Success == false is correct in the cancel case.
//        qDbo() << "Cancelled";
//        setSuccessFlag(false);
//    }
//    else
//    {
//        // Wasn't a cancel, so runFunctor() finished and should have explicitly set success/fail.
//        Q_ASSERT(m_tw_job_run_reported_success_or_fail == 1);
//    }
////////////////////////
//    runEnd();

    // Set the three KJob error fields.
    setKJobErrorInfo(!ef.isCanceled());

    qDbo() << "Calling emitResult():" << "isAutoDelete?:" << isAutoDelete();
    if(isAutoDelete())
    {
        // emitResult() may result in a this->deleteLater(), via finishJob().
        qWro() << "emitResult() may have resulted in a this->deleteLater(), via finishJob().";
    }
    emitResult();

    // Notify any possible doKill() that we really truly have stopped the async worker thread.
    m_run_returned.release();
}

bool AMLMJob::functorHandlePauseResumeAndCancel()
{
    auto& extfutureref = asDerivedTypePtr()->get_extfuture_ref();

    if (extfutureref.isPaused())
    {
        extfutureref.waitForResume();
    }
    if (extfutureref.isCanceled())
    {
        // Caller should break out of while() loop.
        return true;
    }
    else
    {
        return false;
    }
}

/////
/// These are similar to kdevelop::ImportProjectJob().  Now I'm not sure they make sense for us....
///
void AMLMJob::SLOT_extfuture_finished()
{
    //Q_ASSERT(!m_possible_delete_later_pending);

    m_watcher->deleteLater();
    emitResult();
}

void AMLMJob::SLOT_extfuture_canceled()
{
    //Q_ASSERT(!m_possible_delete_later_pending);

    // Nothing but deleteLater() the watcher.
    m_watcher->deleteLater();
}

void AMLMJob::SLOT_extfuture_aboutToShutdown()
{
    kill();
}
////
///
///

/**
 * KJob::doKill() override.
 *
 * kill()/finishJob():
 *
 * So what happens is:
 * - So then it's this:
 *
 * if (doKill())
 * {
      setError(KilledJobError);

      finishJob(verbosity != Quietly);
      return true;
   }
   else
   {
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
bool AMLMJob::doKill()
{
    QMutexLocker lock(&m_start_vs_cancel_mutex);

    qDbo() << "START EXTASYNC DOKILL";

    // KJob::doKill().
    /// @note The calling thread has to have an event loop, and actually AFAICT should be the main app thread.
    AMLM_ASSERT_IN_GUITHREAD();

    if(!(capabilities() & KJob::Capability::Killable))
    {
        Q_ASSERT_X(0, __func__, "Trying to kill an unkillable AMLMJob.");
    }

    auto& ef = asDerivedTypePtr()->get_extfuture_ref();

    // Is the underlying async job actually running, or have we already been cancelled, or never started?
    bool run_returned = m_run_returned.tryAcquire();
    if(run_returned)
    {
        // We have nothing to cancel.
        if(ef.isCanceled())
        {
            // We were have already been canceled.
            qWro() << "ExtAsync<> job already cancelled";
        }
        else if(ef.isFinished())
        {
            // run() finished normally.
            qIno() << "ExtAsync<> job already finished normally";
        }
        // Unacquire the semaphore so we can pend on it below if we've been called twice or something like that.
        m_run_returned.release();
        return true;
    }
    // Else run() hasn't returned, and we didn't acquire the semaphore.

    bool run_was_started = m_run_was_started.tryAcquire();
    if(!run_was_started)
    {
        // run() was never even started.
        qIno() << "ExtAsync<> job never started";
        // We'll cancel the future.
    }
    else
    {
        // run() was started.
        // Unacquire the semaphore.
        m_run_was_started.release();
    }

    // Cancel and wait for the runFunctor() to actually report Finished, not just Canceled.
    ef.cancel();

    /// Kdevelop::ImportProjectJob::doKill() sets the KJob error info here on a kill.
    setError(KilledJobError);

    // Wait for the ExtFuture<> to report Finished or cancelled.
    /// @todo Is this even meaningful with the semaphore acquire below?
    ef.waitForFinished();

    // Wait for the async job to really finish, i.e. for the run() member to finish.
    m_run_returned.acquire();

    qDbo() << "POST-CANCEL FUTURE STATE:" << ExtFutureState::state(ef);

    //    Q_ASSERT(ef.isStarted() && ef.isCanceled() && ef.isFinished());
    // We should never get here before the undelying ExtAsync job is indicating canceled and finished.
    /// @note Seeing the assert below, sometimes not finished, sometimes is?  Started | Canceled always.
    ///       Kdevelop::ImportProjectJob does this through a QFutureWatcher set up in start().
//    AMLM_ASSERT_EQ(ExtFutureState::state(ef), ExtFutureState::Started | ExtFutureState::Canceled | ExtFutureState::Finished);

//    qDbo() << "END EXTASYNC DOKILL";


    // Try to detect that we've survived at least to this point.
    Q_ASSERT(!m_i_was_deleted);

    /// @warning At any point after we return here, this may have been deleteLater()'ed by KJob::finishJob().
    qWro() << "doKill() returning, may result in a this->deleteLater(), via finishJob().";
    return true;
}

bool AMLMJob::doSuspend()
{
    //Q_ASSERT(!m_possible_delete_later_pending);

    /// KJob::doSuspend().
    Q_ASSERT_X(capabilities() & KJob::Capability::Suspendable, __func__, "Trying to suspend an unsuspendable AMLMJob.");
    asDerivedTypePtr()->get_extfuture_ref().setPaused(true);
    return true;
}

bool AMLMJob::doResume()
{
    //Q_ASSERT(!m_possible_delete_later_pending);

    /// KJob::doResume().
    Q_ASSERT_X(capabilities() & KJob::Capability::Suspendable, __func__, "Trying to resume an unresumable AMLMJob.");
    asDerivedTypePtr()->get_extfuture_ref().setPaused(false);
    return true;
}

void AMLMJob::setProcessedAmountAndSize(KJob::Unit unit, qulonglong amount)
{
    //Q_ASSERT(!m_possible_delete_later_pending);

    if(m_progress_unit != KJob::Unit::Bytes && unit == m_progress_unit)
    {
        // Unit wasn't the progress unit, so also set Bytes so we get percent complete support.
        setProcessedAmount(KJob::Unit::Bytes, amount);
    }
    setProcessedAmount(unit, amount);
}

void AMLMJob::setTotalAmountAndSize(KJob::Unit unit, qulonglong amount)
{
    //Q_ASSERT(!m_possible_delete_later_pending);

    if(m_progress_unit != KJob::Unit::Bytes && unit == m_progress_unit)
    {
        // Unit wasn't the progress unit, so also set Bytes so we get percent complete support.
        setTotalAmount(KJob::Unit::Bytes, amount);
    }
    setTotalAmount(unit, amount);
}

void AMLMJob::setProgressUnit(KJob::Unit prog_unit)
{
    //Q_ASSERT(!m_possible_delete_later_pending);

#ifdef THIS_IS_EVER_NOT_BROKEN
    /// @todo This "KJobPrivate" crap is crap.
	//    d_ptr->progressUnit = prog_unit;

	/// And if this works, it's gross.
    const QMetaObject* metaObject = asDerivedTypePtr()->metaObject();
	QStringList methods;
	for(int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i)
	{
	    methods << QString::fromLatin1(metaObject->method(i).methodSignature());
	}
    qDbo() << methods;
#endif
    m_progress_unit = prog_unit;
}

KJob::Unit AMLMJob::progressUnit() const
{
    //Q_ASSERT(!m_possible_delete_later_pending);

    return m_progress_unit;
}

void AMLMJob::setKJobErrorInfo(bool success)
{
    //Q_ASSERT(!m_possible_delete_later_pending);

    // We're still in the underlying ExtAsync::run() context and don't have an event loop here.
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
        if(asDerivedTypePtr()->get_extfuture_ref().isCanceled())
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
            setErrorText(QString("Unknown, non-Killed-Job error on AMLMJob: %1").arg(asDerivedTypePtr()->objectName()));
        }
    }
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
