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

// Qt
#include <QPointer>

// KF
#include <KJob>
#include <KJobWidgets>
#include <KDialogJobUiDelegate>

/// Ours
#include <AMLMApp.h>
#include <gui/MainWindow.h>
#include "utils/DebugHelpers.h"
#include "utils/UniqueIDMixin.h"
#include "utils/QtCastHelpers.h"
#include "utils/TheSimplestThings.h"
#include <utils/RegisterQtMetatypes.h>


AMLM_QREG_CALLBACK([](){
    qIn() << "Registering KJob::Unit";
    qRegisterMetaType<KJob::Unit>();
});



AMLMJob::AMLMJob(QObject *parent) : KJob(parent)
{
//    setObjectName(uniqueQObjectName());

    /// @todo This is sort of horrible, we should find a just-in-time way to do the uiDelegate.
    /// ...and also, while this prevents crashes, we don't get any dialog etc. output on fail.
    /// So not at all clear what's happening here.
    KJobWidgets::setWindow(this, MainWindow::instance());
    setUiDelegate(new KDialogJobUiDelegate());

    connect_or_die(this, &KJob::result, this, &AMLMJob::SLOT_kjob_result);

    // Master app shutdown signal connection.
    /// @note KDevelop/ImportProjectJob() does exactly this, but instead of the app it's ICore::self().
	connect_or_die(AMLMApp::instance(), &AMLMApp::SIGNAL_aboutToShutdown, this, &AMLMJob::SLOT_onAboutToShutdown);
}

AMLMJob::~AMLMJob()
{
    // The ExtAsync job should not be running here.

    // The KJob should have finished/been killed before we get deleted.
//    Q_ASSERT(isFinished());

    // Q_ASSERT(!m_i_was_deleted);
    // m_i_was_deleted = true;

    // KJob destructor checks if KJob->isFinished and emits finished(this) if so.
    // Doesn't cancel the job.
//    qDbo() << "AMLMJob DELETED" << this;
}

qulonglong AMLMJob::totalSize() const
{
    return totalAmount(progressUnit());
}

qulonglong AMLMJob::processedSize() const
{
    return processedAmount(progressUnit());
}

/**
 * KJob completion notes:
 *
 * - On normal completion:
 * - Subclass calls emitResult(), which directly calls finishJob(true).
 *      - Shoudn't be any kjob error.
 *      - finishJob(true):
 *      - then d->isFinished = true;
 *      - then the signals are emitted.
 *      - signal finished() is always emitted
 *      - signal result() is emitted()
 *      - if job is autodelete, deleteLater.
 *
 * - On a KJob::kill():
 *      - the kjob error will first be set to KilledJobError,
 *      - finishJob(emitResult = (verbosity != Quietly)):
 *      - then d->isFinished = true;
 *      - then the signals are emitted:
 *      - signal finished() is always emitted
 *      - XX signal result() may not be emitted, if this is a kill(quietly).
 *      - if job is autodelete, deleteLater.
 *
 * - On error:
 *      - Same as normal completion but with an error code?
 *
 * - On KJob destruction, its destructor does this:
 *      - @code
 *          if(d->isFinished)
 *          {
 *              emit finished(this);
 *          }
 *          @endcode
 *      - delete d_ptr, speedtimer, and uiDelegate.
 *
 * - signal finished() is always emitted, and always before result(), which won't be emitted on cancel.
 *
 * - Regarding QObject::deleteLater():
 *      - @link http://doc.qt.io/qt-5/qobject.html#deleteLater
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


void AMLMJob::SLOT_kjob_finished(KJob *kjob)
{
    qDbo() << "GOT KJOB FINISHED:" << kjob;
}

void AMLMJob::SLOT_kjob_result(KJob *kjob)
{
    qDbo() << "GOT KJOB RESULT:" << kjob << ", error():" << kjob->error();
}


void AMLMJob::SLOT_onAboutToShutdown()
{
    /// @note KDevelop/ImportProjectJob() just does kill() here.

// I think we need 'already killed' reentrancy protection here or in kill() itself
/// @todo If left to run, loading a library leaves DirectoryScannerAMLMJob_0 laying around for some reason,
/// which then segfaults here on AboutToShutdown().
    qDb() << "SHUTDOWN, KILLING";
	this->kill();
    qDb() << "SHUTDOWN, KILLED";
}


/**
 * KJob::doKill() override.
 *
 * kill()/finishJob():
 *
 * So what happens is:
 * - So then it's this:
 *
 * @code
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
	@endcode
    ... and this (which is also called directly by emitResult()):
	@code
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
	@endcode
    So I think the answer is:
    - We need this object to survive doKill(),
    - We can't do anything else after that, due to finishJob() possibly destroying us.
 */
#if 0
bool AMLMJob::doKill()
{

}
#endif

void AMLMJob::setProcessedAmountAndSize(/*KJob::Unit*/int unit, qulonglong amount)
{
	KJob::Unit kjob_unit;
	kjob_unit = static_cast<KJob::Unit>(unit);

	if(m_progress_unit != KJob::Unit::Bytes && kjob_unit == m_progress_unit)
    {
        // Unit wasn't the progress unit, so also set Bytes so we get percent complete support.
        setProcessedAmount(KJob::Unit::Bytes, amount);
    }
	setProcessedAmount(kjob_unit, amount);
}

void AMLMJob::setTotalAmountAndSize(/*KJob::Unit*/int unit, qulonglong amount)
{
    if(m_progress_unit != KJob::Unit::Bytes && unit == m_progress_unit)
    {
        // Unit wasn't the progress unit, so also set Bytes so we get percent complete support.
        setTotalAmount(KJob::Unit::Bytes, amount);
    }
	setTotalAmount(KJob::Unit(unit), amount);
}

void AMLMJob::setProgressUnit(int prog_unit)
{
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
	m_progress_unit = static_cast<KJob::Unit>(prog_unit);
}

KJob::Unit AMLMJob::progressUnit() const
{
    return m_progress_unit;
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
    for(auto unit : {KJob::Unit::Bytes, KJob::Unit::Files, KJob::Unit::Directories, KJob::Unit::Items})
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
        // Per KF docs (https://api.kde.org/frameworks/kcoreaddons/html/classKJob.html#ae0ac2567b61681f4811d128825fbcd0b),
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


