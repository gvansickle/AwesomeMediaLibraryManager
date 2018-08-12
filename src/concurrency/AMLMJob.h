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

#ifndef SRC_CONCURRENCY_AMLMJOB_H_
#define SRC_CONCURRENCY_AMLMJOB_H_

#include <config.h>

// Qt5
#include <QObject>
#include <QPointer>
#include <QWeakPointer>
#include <QSharedPointer>
#include <QTime>
#include <QMutex>
#include <QWaitCondition>
#include <QSemaphore>

// KF5
#include <KJob>
#include <KJobUiDelegate>

// Ours
#include <src/future/function_traits.hpp>
#include <src/future/static_if.hpp>
#include "utils/UniqueIDMixin.h"
#include "utils/ConnectHelpers.h"
#include "concurrency/ExtAsync.h"


class AMLMJob;
/// Use the AMLMJobPtr alias to pass around refs to AMLMJob-derived jobs.
using AMLMJobPtr = QPointer<AMLMJob>;

Q_DECLARE_METATYPE(AMLMJobPtr);


// Ours


template <class T>
struct AMLMJob_traits
{
    using type = T;
    using ExtFutureType = typename T::ExtFutureT;

    ExtFutureType& get_future_ref() { return T::m_ext_future; }

};

template <class ExtFutureT>
struct AMLMJobFutureHelper
{
    ExtFutureT m_ext_future;
};

/**
* Where Does The State Live?
*
* KJobPrivate itself contains what should be what's needed and canonical:
*
* class KCOREADDONS_EXPORT KJobPrivate
* {
* public:
* [...]
*   QString errorText;
   int error;
   KJob::Unit progressUnit;
   QMap<KJob::Unit, qulonglong> processedAmount;
   QMap<KJob::Unit, qulonglong> totalAmount;
   unsigned long percentage;
*
* Most/all of this data can be accessed from protected or public KJob members.  E.g.:
* class KJob
* protected:
*     KJob::setProcessedAmount(Unit unit, qulonglong amount)
*      Sets the processed size. The processedAmount() and percent() signals
*      are emitted if the values changed. The percent() signal is emitted
*      only for the progress unit.
*     void setProcessedAmount(Unit unit, qulonglong amount);
*
* The code looks like this:
*
* @code
* protected: void KJob::setProcessedAmount(Unit unit, qulonglong amount)
    {
        Q_D(KJob);
        bool should_emit = (d->processedAmount[unit] != amount);

        d->processedAmount[unit] = amount;

        if (should_emit) {
            emit processedAmount(this, unit, amount);
            if (unit == d->progressUnit) {
                emit processedSize(this, amount);
                emitPercent(d->processedAmount[unit], d->totalAmount[unit]);
            }
        }
    }

    Q_SIGNAL: // Private, don't emit directly, call setProcessedAmount().
    void processedAmount(KJob *job, KJob::Unit unit, qulonglong amount);

    // Public read accessor.
     * Returns the processed amount of a given unit for this job.
     *
     * @param unit the unit of the requested amount
     * @return the processed size
    //
    Q_SCRIPTABLE qulonglong processedAmount(Unit unit) const;

* @endcode
*
* So it would appear that there's no need to maintain any such state in the widget.
* Except...
*
/// Amount vs. Size
/// KJob looks somewhat broken when it comes to "Size".  It supports "Amount" units of Kjob::Bytes,
/// KJob::Files, and KJob::Directories, and while there is a "KJob::Unit progressUnit;" member, it's
/// hidden behind the pImpl and there's no way to see or change it as far as I can tell.  It's defaulted
/// to "KJob::Bytes", and is the unit used as the "Size" for both read and write.
/// All KJob Amount and Size updates come from (protected) calls to setProcessedAmount() and
/// setSizeAmount().  They set the amount of the given unit in the qmap, and emit processed/totalAmount()
/// signals.  Additionally, if the unit matches "progressUnit" (==Bytes), processed/totalSize() signals
/// are emitted, the percent complete mechanism is updated, and percent() is emitted.
/// So bottom line, it looks like if you want to use percent at all, you have to update the "KJob::Units::Bytes"
/// *Amount()s.  I guess that's ok, you can always call it whatever you want in the UI.
///
*/

/**
 * Base class for jobs which bridges the gap between an ExtAsync job and a KJob-derived class.
 *
 * @note Multiple inheritance in effect here.  Ok since only KJob inherits from QObject.
 *
 */
class AMLMJob: public KJob, public UniqueIDMixin<AMLMJob>
{

    Q_OBJECT

    /// KCoreAddons::KJob
    /// - Subclasses must implement start(), which should trigger the execution of the job (although the work should be done asynchronously).
    /// - errorString() should also be reimplemented by any subclasses that introduce new error codes.
    /// - KJob and its subclasses are meant to be used in a fire-and-forget way. Jobs will delete themselves when they finish using
    ///   deleteLater() (although this behaviour can be changed), so a job instance will disappear after the next event loop run.

    /**
     * @note CRTP: Still need this to avoid ambiguous name resolution.
     * @see https://stackoverflow.com/a/46916924
     */
    using UniqueIDMixin<AMLMJob>::uniqueQObjectName;

	using BASE_CLASS = KJob;

Q_SIGNALS:

    /// @warning Qt5 signals are always public in the C++ sense.  Slots are similarly public when called
    ///          via the signal->slot mechanism, on direct calls they have the normal public/protected/private rules.


    /// @name User-public/subclass-private internal KJob signals.
    /// Here for reference only, these are KJob-private, i.e. can't be emitted directly.
    /// @{

    /// "Emitted when the job is finished, in any case. It is used to notify
    /// observers that the job is terminated and that progress can be hidden."
    /// Call emitResult(job) to emit.
//    void finished(KJob *job);
    /// "Emitted when the job is suspended."
    /// No direct way to emit this?
//    void suspended(KJob *job);
    /// "Emitted when the job is resumed."
    /// No direct way to emit this?
//    void resumed(KJob *job);
    /// "Emitted when the job is finished (except when killed with KJob::Quietly).
    /// Use error to know if the job was finished with error."
    /// Call emitResult(job) to emit.
//    void result(KJob *job);

    /// @}

    /// @name Public KJob signals, quite a few:
    ///
	// void 	description (KJob *job, const QString &title, const QPair< QString, QString > &field1=QPair< QString, QString >(), const QPair< QString, QString > &field2=QPair< QString, QString >())

    /// "Emitted when the job is finished, in any case.
    /// It is used to notify observers that the job is terminated and that progress can be hidden.
    /// *** This is a private signal, it can't be emitted directly by subclasses of KJob, use emitResult() instead.
    /// In general, to be notified of a job's completion, client code should connect to result() rather than finished(), so that kill(Quietly) is indeed quiet. However if you store a list of jobs
    /// and they might get killed silently, then you must connect to this instead of result(), to avoid dangling pointers in your list."
    // void finished (KJob *job)

    // void 	infoMessage (KJob *job, const QString &plain, const QString &rich=QString())
    // void 	warning (KJob *job, const QString &plain, const QString &rich=QString())


    // void result (KJob *job)
	// void 	resumed (KJob *job)
    /// KJob::Speed
    /// I'm not at all clear on how to really use the KJob::speed functionality.  You call emitSpeed(value),
    /// with whatever "value" is, it starts a 5 second timer, then... the timer times out... then I totally lose the plot.
    /// Ah well.
	// void 	speed (KJob *job, unsigned long speed)
    // void suspended (KJob *job)

	// void 	totalAmount (KJob *job, KJob::Unit unit, qulonglong amount)
	// void 	totalSize (KJob *job, qulonglong size)
    // void 	processedAmount (KJob *job, KJob::Unit unit, qulonglong amount)
    // void 	processedSize (KJob *job, qulonglong size)
    // void 	percent (KJob *job, unsigned long percent)

    /// KJobs are supported by KJobWidgets
	/// https://api.kde.org/frameworks/kjobwidgets/html/namespaceKJobWidgets.html
	///

    /// KJobTrackerInterface (== Watcher for KJob*s):
    /// https://cgit.kde.org/kcoreaddons.git/tree/src/lib/jobs/kjobtrackerinterface.h
    /// On call to registerJob(), The default implementation connects the following KJob signals
    /// to the respective protected slots of this class:
    //    *  - finished() (also connected to the unregisterJob() slot) // Emitted in KJob destructor.
    //    *  - suspended()
    //    *  - resumed()
    //    *  - description()
    //    *  - infoMessage()
    //    *  - totalAmount()
    //    *  - processedAmount() ///< @note KJob's xxxSize() signals aren't reflected in this interface.
    //    *  - percent()
    //    *  - speed()
    //    *
    //    * If you re-implement this method [registerJob()], you may want to call the default
    //    * implementation or add at least:
    //    *
    //    * @code
    //    * connect(job, &KJob::finished, this, &MyJobTracker::unregisterJob);
    //    * @endcode
    //    *
    //    * so that you won't have to manually call unregisterJob().

    void SIGNAL_internal_call_emitResult();

protected:
    /// Protected KJob-like constructor.
    /// Derive from and defer to this from derived classes, possibly as part of a two-stage constructor:
    /// Once the derived constructor is called and returns, we'll have a valid AMLMJob this and a valid derived this.
    /// We can then call virtual functions in subsequent constructors.
    /// @note Don't try that at home.
    ///
    /// @note KJob's constructor has this same signature, in particular nonconst pointer to parent.
    explicit AMLMJob(QObject* parent = nullptr);

public:
    AMLMJob() = delete;
    /// Destructor.
    ~AMLMJob() override;

    /// @name KJob overrides.
    /// @{

    /**
     * Starts the job asynchronously.
     *
     * When the job is finished, result() is emitted.
     *
     * Warning: Never implement any synchronous workload in this method. This method
     * should just trigger the job startup, not do any work itself. It is expected to
     * be non-blocking.
     *
     * This is the method all subclasses need to implement.
     * It should setup and trigger the workload of the job. It should not do any
     * work itself. This includes all signals and terminating the job, e.g. by
     * emitResult(). The workload, which could be another method of the
     * subclass, is to be triggered using the event loop, e.g. by code like:
     * \code
     * void ExampleJob::start()
     * {
     *  QTimer::singleShot(0, this, SLOT(doWork()));
     * }
     * \endcode
     *
     * @note GRVS: Per comments, KF5 KIO::Jobs autostart; this is overridden to be a no-op.
     */
//    Q_SCRIPTABLE void start() override;

    /// @}

public:
    /// @name New public interfaces FBO derived classes.
    /// Need to be public so they can be accessed from the self pointer passed to run(), which may or may not be this.
    /// @{


    /// Derived runFunctor() must call this before exiting.
    void setSuccessFlag(bool success);

    /// @}

    virtual qulonglong totalSize() const;
    virtual qulonglong processedSize() const;

public:
    /// Dump info about the given KJob.
    static void dump_job_info(KJob* kjob, const QString &header = QString());

public:

//    template<typename FutureT>
//    FutureT& get_extfuture_ref()
//    {
////        AMLMJob_traits<std::remove_reference_t<decltype(*this)>>::ExtFutureType retval = this->m_ext_future;
//        //return &decltype(FutureT)(std::declval<>this->m_ext_future);
////        auto& retval = AMLMJob_traits<std::remove_reference_t<decltype(*this)>>::get_future_ref();
//        auto& retval = AMLMJob_traits<std::remove_reference_t<decltype(*this->asDerivedTypePtr())>>::get_future_ref();
//        return retval;
//    }

    /// @name Callback/pseudo-std-C++17+ interface.
    /// @{

    /**
     * .then(ctx, continuation) -> void
     */
    template <typename ContextType, typename Func,
              REQUIRES(std::is_base_of_v<QObject, ContextType>)>
    void then(const ContextType *ctx, Func&& f)
    {
        qDb() << "ENTERED THEN";

//        QPointer<KJob> pkjob = kjob;

        // result(KJob*) signal:
        // "Emitted when the job is finished (except when killed with KJob::Quietly)."
        connect(this, &KJob::result, ctx, [=](KJob* kjob){

//            qDbo() << "IN THEN CALLBACK, KJob:" << kjob;

            // Need to determine if the result was success, error, or cancel.
            // In the latter two cases, we need to make sure any chained AMLMJobs are either
            // cancelled (or notified of the failure?).
            switch(kjob->error())
            {
            // "[kjob->error()] Returns the error code, if there has been an error.
            // Only call this method from the slot connected to result()."
            case NoError:
                break;
            case KilledJobError:
                break;
            default:
                // UserDefinedError or some other error.
                break;
            }

            if(kjob->error())
            {
                // Report the error.
                qWr() << "Reporting error via uiDelegate():" << kjob->error() << kjob->errorString() << ":" << kjob->errorText();
                kjob->uiDelegate()->showErrorMessage();
            }
            else
            {
                // Cast to the derived job type.
                using JobType = std::remove_pointer_t<argtype_t<Func, 0>>;
                auto* jobptr = dynamic_cast<JobType*>(kjob);
                Q_ASSERT(jobptr);
                // Call the continuation.
                f(jobptr);
            }
        });
    }

	/**
	 * .tap(ctx, continuation) -> void
	 *
	 * @tparam continuation  Invocable taking an ExtFuture<T> of that used by the derived type.
	 * @returns @todo Should be another ExtFuture.
	 */
//	template <typename ContextType, typename FuncType,
//			  REQUIRES(std::is_base_of_v<QObject, ContextType> &&
//			  ct::is_invocable_r_v<void, FuncType, std::result_of_t<this->get_extfuture_ref()>>)>
//	void tap(const ContextType *ctx, Func&& f)
//	{

//	}

    /// @}



public Q_SLOTS:

    /// @name KJob job control slots
    /// @note Default KJob implementations appear to be sufficient.  They call
    ///       the protected "doXxxx()" functions (which we do need to override below),
    ///       and then emit the proper signals to indicate the deed is done.
    /// @link https://api.kde.org/frameworks/kcoreaddons/html/kjob_8cpp_source.html#l00117
    /// @{

    /**
     * KJob::kill() does this:
     * https://cgit.kde.org/kcoreaddons.git/tree/src/lib/jobs/kjob.cpp
     * @code
     * if (doKill()) {
     *      setError(KilledJobError);
     *      finishJob(verbosity != Quietly);
     *      return true;
     *  } else {
     *      return false;
     *  }
     * @endcode
     * void finishJob(bool emitResult); is a private non-virt member, also called by emitResult() with param==true,
     * https://cgit.kde.org/kcoreaddons.git/tree/src/lib/jobs/kjob.cpp#n96
     * which does this:
     * @code
     * Q_D(KJob);
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
     * @endcode
     *
     */
//    bool kill(KJob::KillVerbosity verbosity=KJob::Quietly);

//    bool resume();
//    bool suspend();

    /// @}

protected:

    /// @name ExtAsync job support functions / function templates.
    /// @{

    virtual AMLMJob* asDerivedTypePtr() = 0;

    /// Last-stage wrapper around the runFunctor().
    /// Handles most of the common ExtFuture start/finished/canceled/exception code.
    /// Should not need to be overridded in derived classes.
//    virtual void run();

    /**
     * The function which is run by ExtAsync::run() to do the work.
     * Must be overridden in derived classes.
     * Reporting and control should be handled via the derived class's m_ext_future member.
     *
     */
    virtual void runFunctor() = 0;

    /**
     * Call this at the bottom of your runFunctor() override.
     * Handles pause/resume internally.
     * @return true if loop in runFunctor() should break due to being canceled.
     */
    bool functorHandlePauseResumeAndCancel();

    /// @}

    /// @name Override of KJob protected functions.
    /// @{

    /**
     * Kill the KJob.
     * Abort this job quietly.
     * Simply kill the job, no error reporting or job deletion should be involved.
     *
     * @note Not a slot.
     *
     * @note KJob::doKill() does nothing, simply returns false.
     *
     * What our override here does:
     * Tells the runFunctor() to kill itself by calling .cancel() on its ExtFuture<>, then
     * waits for it to finish via waitForFinished().
     *
     * @note It does look like this should block until the job is really confirmed to be killed.
     *       KAbstractWidgetJobTracker::slotStop() does this:
     * @code
     *         job->kill(KJob::EmitResult); // notify that the job has been killed
     *         emit stopped(job);
     * @endcode
     *
     * @warning this may/will be deleteLater()'ed at any time after this function returns true.
     *
     * @return true if job successfully killed, false otherwise.
     */
//    bool doKill() override;

    /**
     * Not a slot.
     * @note KJob::doSuspend() simply returns false.
     */
//    bool doSuspend() override;

    /**
     * Not a slot.
     * @note KJob::doResume() simply returns false.
     */
//    bool doResume() override;

    /**
     * KJob::emitResult()
     * Emit the result signal, and if job is autodelete, suicide this job.
     * @note Deletes this job using deleteLater(). It first notifies the observers to hide the
     *       progress for this job using the finished() signal.
     *       KJob implementation calls finshJob(), which:
     *       - Sets isFinished = true
     *       - quit()s the internal event loop
     *       - emits finished(this);
     *       - if(emitResult) emit result(this) //< emitResult == true in this case.
     *       - if(isAutoDelete) deleteLater();
     *       This is probably sufficient behavior and we don't need to overload this (non-virtual) function.
     */
    /// void emitResult();

    /**
     * KJob::emitPercent()
     */
//    void KJob::emitPercent(qulonglong processedAmount, qulonglong totalAmount);

    /// Defaults of these seem to be suitable.
//    void setError(int errorCode);
//    void setErrorText(const QString &errorText);
//    void setProcessedAmount(Unit unit, qulonglong amount);
//    void setTotalAmount(Unit unit, qulonglong amount);
//    void setPercent(unsigned long percentage);
    /// speed is in bytes/sec
//    void emitSpeed(unsigned long speed);

    /// @} /// END Override of KJob protected functions.

    /// @name New protected methods
    /// @{

    /// @warning For use only by KJob
    /// Give derived classes write access to progressUnit.
    /// Sets the Unit which will be used for percent complete and total/processedSize calculations.
    /// Defaults to KJob::Unit::Bytes.
    void setProgressUnit(KJob::Unit prog_unit);
    KJob::Unit progressUnit() const;

    virtual void setProcessedAmountAndSize(Unit unit, qulonglong amount);
    virtual void setTotalAmountAndSize(Unit unit, qulonglong amount);

    /**
     * Sets the KJob error code / string.
     *
     * @param success  true if the underlying job completed successfully and wasn't cancelled.  false otherwise.
     */
//    void setKJobErrorInfo(bool success);

    /// @}

protected Q_SLOTS:

    /// @name Internal slots
    /// @{

    void SLOT_on_destroyed(QObject* obj);

    void SLOT_extfuture_finished();
    void SLOT_extfuture_canceled();

    void SLOT_kjob_finished(KJob* kjob);
    void SLOT_kjob_result(KJob* kjob);

    void SLOT_call_emitResult();

    /// @}

private:
    Q_DISABLE_COPY(AMLMJob)

private Q_SLOTS:

    /**
     * Connected to the App's aboutToShutdown() signal in the constructor.
     * Simply calls kill().
     * Similar to mechanism in KDevelop's ::ICore & ::ImportProjectJob.
     */
    void SLOT_onAboutToShutdown();

public:

    bool m_i_was_deleted = false;

    /**
     * Semaphores for coordinating the sync and async operations in doKill().
     */
    QMutex m_start_vs_dokill_mutex;
    QSemaphore m_run_was_started {0};
    QSemaphore m_run_returned {0};


private:

    QFutureWatcher<void>* m_watcher;

    QAtomicInt m_success { 1 };

    /// Wishful thinking at the moment, but maybe I'll figure out how to separate "Size" from KJob::Bytes.
    KJob::Unit m_progress_unit { KJob::Unit::Bytes };
};

//Q_DECLARE_METATYPE(AMLMJob); /// @todo need default constructor and copy constructor.

template <class ExtFutureT>
class AMLMJobT : public AMLMJob
{
	using BASE_CLASS = AMLMJob;

public:

	using ExtFutureWatcherT = QFutureWatcher<typename ExtFutureT::value_type>;

	explicit AMLMJobT(QObject* parent = nullptr) : BASE_CLASS(parent), m_ext_watcher(parent)
	{
		qDbo() << "WORKED:" << m_ext_future.state();

		// Hook up signals and such to the ExtFuture<T>.
		HookUpExtFutureSignals();
	}

    void setFuture(ExtFutureT& ef)
    {
        m_ext_future = ef;
    }

    ExtFutureT& get_extfuture_ref()
    {
        return m_ext_future;
    }

    Q_SCRIPTABLE void start() override
    {
        QMutexLocker lock(&m_start_vs_dokill_mutex);

    //    m_watcher = new QFutureWatcher<void>(this);
    //    auto& ef = asDerivedTypePtr()->get_extfuture_ref();
    //    connect_or_die(m_watcher, &QFutureWatcher<void>::finished, this, &AMLMJob::SLOT_extfuture_finished);
    //    connect_or_die(m_watcher, &QFutureWatcher<void>::canceled, this, &AMLMJob::SLOT_extfuture_canceled);

        // Just let ExtAsync run the run() function, which will in turn run the runFunctor().
        // Note that we do not use the returned ExtFuture<Unit> here; that control and reporting
        // role is handled by the ExtFuture<> ref returned by get_extfuture_ref().
        // Note that calling the destructor of (by deleting) the returned future is ok:
        // http://doc.qt.io/qt-5/qfuture.html#dtor.QFuture
        // "Note that this neither waits nor cancels the asynchronous computation."

        // Indicate to the cancel logic that we did start.
        m_run_was_started.release();

        // Run.
        ExtAsync::run(this, &AMLMJobT::run);
    //    m_watcher->setFuture(asDerivedTypePtr()->get_extfuture_ref());
    }

protected Q_SLOT:

    virtual void SLOT_onResultsReadyAt(ExtFutureT& ef, int begin, int end) {}


protected:

    virtual void run()
    {
        /// @note We're in an arbitrary thread here probably without an event loop.

        /// @note void QThreadPoolThread::run() has a similar construct, and wraps this whole thing in:
        /// QMutexLocker locker(&manager->mutex);

        m_run_was_started.release();

        auto& ef = asDerivedTypePtr()->get_extfuture_ref();

        qDbo() << "ExtFuture<> state:" << ef.state();
        if(ef.isCanceled())
        {
            // We were canceled before we were started.
            /// @note Canceling alone won't finish the extfuture, so we finish it manually here.
            // Report (STARTED | CANCELED | FINISHED)
            ef.reportFinished();
            AMLM_ASSERT_EQ(ExtFutureState::state(ef), (ExtFutureState::Started | ExtFutureState::Canceled | ExtFutureState::Finished));

            m_run_returned.release();
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

        // Set the three KJob error fields.
        setKJobErrorInfo(!ef.isCanceled());

        qDbo() << "Calling emitResult():" << "isAutoDelete?:" << isAutoDelete();
        if(isAutoDelete())
        {
            // emitResult() may result in a this->deleteLater(), via finishJob().
            qWro() << "emitResult() may have resulted in a this->deleteLater(), via finishJob().";
        }

        // emitResult().
        // We use a signal/slot here since we're in an arbitrary context.
        Q_EMIT SIGNAL_internal_call_emitResult();

        // Notify any possible doKill() that we really truly have stopped the async worker thread.
        m_run_returned.release();
    }

    bool doKill() override { return doKillT(); }
    bool doSuspend() override { return doSuspendT(); }
    bool doResume() override { return doResumeT(); }

    bool doKillT()
    {
        // KJob::doKill().

        QMutexLocker lock(&m_start_vs_dokill_mutex);

        qDbo() << "START EXTASYNC DOKILL";

        /// @note The calling thread has to have an event loop, and actually AFAICT should be the main app thread.
        AMLM_ASSERT_IN_GUITHREAD();

        if(!(capabilities() & KJob::Capability::Killable))
        {
            Q_ASSERT_X(0, __func__, "Trying to kill an unkillable AMLMJob.");
        }

        auto& ef = asDerivedTypePtr()->get_extfuture_ref();

        // Is the underlying ExtAsync job currently running, or have we already been cancelled, or never started?
        bool run_returned = m_run_returned.tryAcquire();
        if(run_returned)
        {
            // run() completed, we have nothing to cancel.
            if(ef.isCanceled())
            {
                // We were have already been canceled.
                qWro() << "ExtAsync<> job already cancelled";
                AMLM_ASSERT_EQ(ef.isFinished(), true);
            }
            else if(ef.isFinished())
            {
                // run() finished normally.
                qIno() << "ExtAsync<> job already finished";
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
            AMLM_ASSERT_EQ(m_run_was_started.available(), 0);
            AMLM_ASSERT_EQ(m_run_returned.available(), 0);
    ///
            // Pretend it started and finished for the logic below, Unacquire the semaphore.
            ef.reportFinished();
            m_run_returned.release();
            AMLM_ASSERT_EQ(m_run_returned.available(), 1);

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
    //    setError(KilledJobError);
        setKJobErrorInfo(false);

        // Wait for the ExtFuture<> to report Finished or cancelled.
        /// @todo Is this even meaningful with the semaphore acquire below?
        ef.waitForFinished();

        // Wait for the async job to really finish, i.e. for the run() member to finish.
        /// @todo won't be acq'able if killed before started.
        m_run_returned.acquire();

        /// @todo Difference here btw cancel before and after start.
        /// Before: Started | Canceled, After: S|F|C.
        qDbo() << "POST-CANCEL FUTURE STATE:" << ExtFutureState::state(ef);

        //    Q_ASSERT(ef.isStarted() && ef.isCanceled() && ef.isFinished());
        // We should never get here before the undelying ExtAsync job is indicating canceled and finished.
        /// @note Seeing the assert below, sometimes not finished, sometimes is?  Started | Canceled always.
        ///       Kdevelop::ImportProjectJob does this through a QFutureWatcher set up in start().
    //    AMLM_ASSERT_EQ(ExtFutureState::state(ef), ExtFutureState::Started | ExtFutureState::Canceled | ExtFutureState::Finished);

    //    qDbo() << "END EXTASYNC DOKILL";


        // Try to detect that we've survived at least to this point.
        Q_ASSERT(!m_i_was_deleted);

        if(isAutoDelete())
        {
            /// @warning At any point after we return here, this may have been deleteLater()'ed by KJob::finishJob().
            qWro() << "doKill() returning, AMLMJob is autoDelete(), may result in a this->deleteLater(), via finishJob().";
        }
        return true;
    }

    bool doSuspendT()
    {
        //Q_ASSERT(!m_possible_delete_later_pending);

        /// KJob::doSuspend().
        Q_ASSERT_X(capabilities() & KJob::Capability::Suspendable, __func__, "Trying to suspend an unsuspendable AMLMJob.");
        asDerivedTypePtr()->get_extfuture_ref().setPaused(true);
        return true;
    }

    bool doResumeT()
    {
        //Q_ASSERT(!m_possible_delete_later_pending);

        /// KJob::doResume().
        Q_ASSERT_X(capabilities() & KJob::Capability::Suspendable, __func__, "Trying to resume an unresumable AMLMJob.");
        asDerivedTypePtr()->get_extfuture_ref().setPaused(false);
        return true;
    }

    /// Call this in your derived runFunctor() function to see if you should cancel the loop.
    bool wasCancelRequested()
    {
        Q_ASSERT(!m_i_was_deleted);

        // Were we told to abort?
        return m_ext_future.isCanceled();
    }

    bool functorHandlePauseResumeAndCancel()
    {
        auto& ef = m_ext_future;

        if (ef.isPaused())
        {
            ef.waitForResume();
        }
        if (ef.isCanceled())
        {
            // The job has been canceled.
            // The calling runFunctor() should break out of while() loop.
            return true;
        }
        else
        {
            return false;
        }
    }

	void HookUpExtFutureSignals()
	{
		// Main connection we need is results.
		// resultsReadyAt(range): There are results ready immediately at the given index range.
		connect_or_die(&m_ext_watcher, &ExtFutureWatcherT::resultsReadyAt, QApplication::instance(),
					   [=](int beginIndex, int endIndex) {
			// Directly call the overridden slot with all the info needed to get the results.
			/// @todo Hold extfuture here.
			SLOT_onResultsReadyAt(m_ext_future, beginIndex, endIndex);
			;});

		/// @todo EXP: Throttling.
//		m_ext_watcher.setPendingResultsLimit(2);

		// All connections made, so set the watched future.
        m_ext_watcher.setFuture(m_ext_future);
	}

    /// @name KJob-related support functions.
    /// @{

    void setKJobErrorInfo(bool success)
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
            if(m_ext_future.isCanceled())
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

    /// @}

	/// The ExtFuture.
	ExtFutureT m_ext_future;

	/// The watcher for the ExtFuture.
	ExtFutureWatcherT m_ext_watcher;

	AMLMJobT<ExtFutureT>* asDerivedTypePtr() override { return this; }

};

//template<class ExtFutureT>
//inline static auto* make_amlmjobt(ExtFutureT ef, QObject* parent = nullptr)
//{
//	auto job = new AMLMJobT(ef, parent);
//	qDebug() << "WORKED:" << ef;
//}

#endif /* SRC_CONCURRENCY_AMLMJOB_H_ */
