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


/**
 * Design notes
 * To be the Alpha and Omega of Qt5/KF5 *Job classes is a lot of work.  Let's start with the KJob and TW::Job lifecycles.
 *
 * @note TW:
 * "It is essential for the ThreadWeaver library that as a kind of convention, the different creators of Job objects do
 *  not touch the protected data members of the Job until somehow notified by the Job."
 *
 * TW:Job lifecycle
 *  @note No QObject, no signals.
 *  - twj = TW::Job-derived instance created by something (TW::Job itself is abstract, at least ::run() must be overloaded).
 *  - twj submitted to TW::Queue/Weaver.
 *  - TW::Queue decides when twj runs.  When started:
 *  -- ::defaultBegin(JobPointer, Thread) (TW::Job default does literally nothing)
 *  -- ::run(JobPointer, Thread)
 *  --- ::run() runs to completion in Thread.  Control and reporting up to the run() override:
 *  ---   - Need to override ::success() and arrange for it to report true/false.
 *  ---   - Need to override ::requestAbort() and arrange for it to cause ::run() to abort.
 *  -- ::status() will return Status_Success if ::run() ran to completion.
 *  -- ::defaultEnd() (TW::Job default does some cleanup, is *not* empty).
 *
 *  TW::QObjectDecorator adds the following:
 *  - Signal started(TW:JobPtr), when TW:Job has started execution.
 *  - Signal done(TW:JobPtr), when TW:Job has completed execution, regardless of status.
 *  - Signal failed(TW::JobPtr), when TW:Job's ::success() returns false after job is executed.
 *  - defaultBegin() override which emits started(self) and calls job()->defaultBegin().
 *  - defaultEnd() override which:
 *    - Calls job()->defaultEnd()
 *    - if(!success) emits failed(self)
 *    - Always emits done(self).
 *  - autoDelete() support (via TW::IdDecorator), appears to be completely controlled by
 *    the decorator though:
 *     // Auto-delete the decoratee or not.
 *     void setAutoDelete(bool onOff);
 *     // Will the decoratee be auto-deleted?
 *     bool autoDelete() const;
 */

/// Qt5
#include <QObject>
#include <QPointer>
#include <QWeakPointer>
#include <QSharedPointer>
#include <QTime>
#include <QMutex>
#include <QWaitCondition>
#include <QSemaphore>

/// KF5
#include <KJob>
#include <KJobUiDelegate>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/QObjectDecorator>
#include <ThreadWeaver/QueueStream>

/// Ours
#include "utils/UniqueIDMixin.h"
#include "utils/ConnectHelpers.h"
#include "concurrency/function_traits.hpp"

/// Use the AMLMJobPtr alias to pass around refs to AMLMJob-derived jobs.
class AMLMJob;
using AMLMJobPtr = QPointer<AMLMJob>;

Q_DECLARE_METATYPE(AMLMJobPtr);

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
 * Base class for jobs which bridges the hard-to-understand gap between a
 * ThreadWeaver::Job and a KJob-derived class.
 *
 * Goal is to make this one object be both a floor wax and a dessert topping:
 * - A KJob to interfaces which need it, in particular:
 * -- KAbstractWidgetJobTracker and derived classes' registerJob()/unregisterJob() slots.
 * - A ThreadWeaver::Job to interfaces which need it
 *
 * @note Multiple inheritance in effect here.  Ok since only KJob inherits from QObject; ThreadWeaver::Job inherits only from from JobInterface.
 *
 */
class AMLMJob: public KJob, public ThreadWeaver::Job, public UniqueIDMixin<AMLMJob>
{

    Q_OBJECT

    /// ThreadWeaver::Job:
    /// - https://api.kde.org/frameworks/threadweaver/html/classThreadWeaver_1_1Job.html
    /// - Jobs are started by the Queue they're added to, depending on the Queue state.  In suspended state, jobs can be added to the queue,
    ///   but the threads remain suspended. In WorkingHard state, an idle thread may immediately execute the job, or it might be queued if
    ///   all threads are busy.
    /// - Jobs may not be executed twice.
    /// - Job objects do not inherit QObject. To connect to signals when jobs are started or finished, see QObjectDecorator.
    ///
    /// virtual void ThreadWeaver::Job::run(JobPointer self, Thread *thread)
    /// The Job will be executed in the specified thread. thread may be zero, indicating that the job is being executed some other way
    /// (for example, synchroneously by some other job). self specifies the job as the queue sees it. Whenever publishing information
    /// about the job to the outside world, for example by emitting signals, use self, not this. self is the reference counted object
    /// handled by the queue. Using it as signal parameters will amongst other things prevent thejob from being memory managed and deleted.
    ///
    /// KCoreAddons::KJob
    /// - Subclasses must implement start(), which should trigger the execution of the job (although the work should be done asynchronously).
    /// - errorString() should also be reimplemented by any subclasses that introduce new error codes.
    /// - KJob and its subclasses are meant to be used in a fire-and-forget way. Jobs will delete themselves when they finish using
    ///   deleteLater() (although this behaviour can be changed), so a job instance will disappear after the next event loop run.
    ///
    /// @note Two confusingly similar typedefs here:
    ///       From qobjectdecorator: "typedef QSharedPointer<QObjectDecorator> QJobPointer;".
    ///       From jobinterface.h:   "typedef QSharedPointer<JobInterface> JobPointer;"
    ///       Job is derived from JobInterface, which in turn derives from nothing.
    ///       All in the ThreadWeaver namespace.

    /**
     * @note CRTP: Still need this to avoid ambiguous name resolution.
     * @see https://stackoverflow.com/a/46916924
     */
    using UniqueIDMixin<AMLMJob>::uniqueQObjectName;

Q_SIGNALS:

    /// @name ThreadWeaver::QObjectDecorator-like signals, only three:
    /// @warning These are for AMLMJob internal-use only.
    /// @warning Qt5 signals are always public in the C++ sense.  Slots are similarly public when called
    ///          via the signal->slot mechanism, on direct calls they have the normal public/protected/private rules.
	/// @{

    /// This signal is emitted when this TW::Job is being processed by a thread.
    void started(ThreadWeaver::JobPointer);
    /// This signal is emitted when success() returns false after the job is executed.
    void failed(ThreadWeaver::JobPointer);
    /// This signal is emitted when the TW::Job has been finished (always, no matter if it succeeded or not).
    void done(ThreadWeaver::JobPointer);


    /// @}

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
    /// I'm not at all clear on how to really use the KJOB::speed functionality.  You call emitSpeed(value),
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



protected:
    /// Protected KJob-like constructor.
    /// Derive from and defer to this from derived classes, possibly as part of a two-stage constructor:
    /// Once the derived constructor is called and returns, we'll have a valid AMLMJob this and a valid derived this.
    /// We can then call virtual functions in subsequent constructors.
    /// @note Don't try that at home.
    ///
    /// @note KJob's constructor has this same signature.
    explicit AMLMJob(QObject* parent = nullptr);

public:
    AMLMJob() = delete;
    /// Destructor.
    ~AMLMJob() override;

    /// @name TW::Job public method overrides.
    /// @{

    /**
     * TW::success().
     * "Return whether the Job finished successfully or not.
     * The default implementation simply returns true. Overload in derived classes if the derived Job class can fail.
     *
     * If a job fails (success() returns false), it will *NOT* resolve its dependencies when it finishes. This will make sure that
     * Jobs that depend on the failed job will not be started.
     *
     * There is an important gotcha: When a Job object it deleted, it will always resolve its dependencies. If dependent jobs should
     * not be executed after a failure, it is important to dequeue those before deleting the failed Job. A Sequence may be
     * helpful for that purpose."
     */
    bool success() const override;

    /**
     * Abort the execution of the TW::Job.
     * Call this method to ask the Job to abort if it is currently executed.
     *
     * @note This method should return immediately, not after the abort has completed.
     *
     * @note TW::Job's default implementation of the method does nothing.
     * @note TW::IdDecorator calls the TW::Job's implementation.
     */
    void requestAbort() override;

    /// @} // END TW::Job overrides.

    /// @name KJob overrides.
    /// @{

    /**
     * "Subclasses must implement start(), which should trigger the execution of the job (although the work should be done
     *  asynchronously)."
     *
     * @note Per comments, KF5 KIO::Jobs autostart; this is overridden to be a no-op.
     */
    Q_SCRIPTABLE void start() override;

    /**
     * Start the TW::Job on the specified TW::QueueStream.
     */
    virtual void start(ThreadWeaver::QueueStream &qstream);

    /// @}

public: /// @warning FBO DERIVED CLASSES ACCESSING THROUGH A POINTER ONLY
    /// @name New public interfaces FBO derived classes' overloads of TW:Job::run().
    /// Need to be public so they can be accessed from the self pointer passed to run(), which may or may not be this.
    /// @{

    /// Call this in your derived tw::run() function to see if you should cancel the loop.
    bool wasCancelRequested();

    /// Derived tw::run() must call this before exiting.  FBO the TW::success() method.
    void setSuccessFlag(bool success);
    /// Derived tw::run() must call this before exiting.  FBO the onTWFailed() method to determine fail reason.
    void setWasCancelled(bool cancelled) { m_tw_job_was_cancelled = cancelled; }
    /// @}

    virtual qulonglong totalSize() const;
    virtual qulonglong processedSize() const;

public:
    /// Dump info about the given KJob.
    static void dump_job_info(KJob* kjob, const QString &header = QString());

public:

    /// @name Callback/pseudo-std-C++17+ interface.
    /// @{

    /**
     * .then(ctx, continuation) -> void
     */
    template <typename ContextType, typename Func>
    void then(ContextType&& ctx, Func&& f)
    {
        connect_or_die(this, &AMLMJob::result, ctx, [=](KJob* kjob){
				if(kjob->error())
				{
					// Report the error.
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

    /// Flag telling the base class whether to use the TW::Job or the ExtAsync underlying implementation.
    /// @note Temp, moving away from ThreadWeaver.
    bool m_use_extasync {false};

//    template<class T>
//    struct ExtAsyncWorkFunctionContext
//    {
//        /**
//         * The ExtAsync work function.
//         * @param the_future
//         */
//        template <class T, template<class> class ExtFutureT>
//        void work_function(ExtFutureT<T>& the_future) { Q_ASSERT(0); };

//        template <class T, template<class> class ExtFutureT>
//        static ExtFutureT<T> m_the_extfuture;
//    };

//    template <class T>
//    ExtAsyncWorkFunctionContext<T> m_extasync_wfctx;

//    virtual ExtFuture<>

    /// @name Override of TW::Job protected functions.
    /// @{

    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread) override = 0;
    void defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread) override;
    /**
     * The defaultEnd() function, called immediately after run() returns.
     * @note run() must have set the correct success() value prior to exiting.
     */
    void defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread) override;

    /// @}

    /// @name Override of KJob protected functions.
    /// @{

    /**
     * Kill the KJob.
     * Abort this job quietly.
     * Simply kill the job, no error reporting or job deletion should be involved.
     *
     * @note KJob::doKill() does nothing, simply returns false.
     *
     * What our override here does:
     * - Tell the TW::Job to kill itself with requestAbort();
     * - Call our onKJobDoKill(), which currently does nothing.
     *
     * @todo Not clear if this should block until the job has been killed or not.
     *       It looks like this should block until the job is really killed;
     *       KAbstractWidgetJobTracker::slotStop() does this:
     *         job->kill(KJob::EmitResult); // notify that the job has been killed
     *         emit stopped(job);
     *
     * @return true if job successfully killed, false otherwise.
     */
    bool doKill() override;

    /**
     * @note KJob::doSuspend() simply returns false.
     */
    bool doSuspend() override;

    /**
     * @note KJob::doResume() simply returns false.
     */
    bool doResume() override;

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

    /// Make the internal signal-slot connections.
    virtual void make_connections();
    virtual void connections_make_defaultBegin(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread);
    virtual void connections_break_defaultExit(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread);

    void TWCommonDoneOrFailed(ThreadWeaver::JobPointer twjob);

    /// @}

protected Q_SLOTS:

    /// @name Internal slots
    /// @{

    /// @name Connected to the TW::QObjectDecorator-like signals.
    /// @{

    /// Should be connected to the started(self) signal.
    /// started() should be getting emitted in defaultBegin().
    void onTWStarted(ThreadWeaver::JobPointer twjob);
    void onTWDone(ThreadWeaver::JobPointer twjob);
    void onTWFailed(ThreadWeaver::JobPointer twjob);
    /// @}

    /// Called from our doKill() operation.  Doesn't do anything but qDb() logging.
    /// @todo Should be really be doing that?
    void onKJobDoKill();

    /// Handle the KJob::result() signal when the job is finished (except when killed with KJob::Quietly).
    /// @note KJob::error() should only be called from this slot, per KF5 docs/comments.
    void onKJobResult(KJob* kjob);

    /// Always invoked by the KJob::finished signal regardless of reason.
    void onKJobFinished(KJob* kjob);

    /// @}

private:
    Q_DISABLE_COPY(AMLMJob)

    bool m_i_was_deleted = false;

	/// Mutex and wait condition for cancel/pause/resume.
    /// Mutex is created in unlocked state, non-recursive.
	QMutex m_cancel_pause_resume_mutex;
    QWaitCondition m_cancel_pause_resume_waitcond;
    /// Flag telling the AMLMJob run() thread to cancel.
    /// No need to be atomic due to the mutex/wc.
    bool m_tw_flag_cancel {false};

    QAtomicInt m_tw_job_run_reported_success_or_fail {0};
    QAtomicInt m_tw_job_is_done { 0 };
    QAtomicInt m_tw_job_was_cancelled { 0 };
    QAtomicInt m_success { 1 };

    /// Wishful thinking at the moment, but maybe I'll figure out how to separate "Size" from KJob::Bytes.
    KJob::Unit m_progress_unit { KJob::Unit::Bytes };
};

//Q_DECLARE_METATYPE(AMLMJob); /// @todo need default constructor and copy constructor.

#endif /* SRC_CONCURRENCY_AMLMJOB_H_ */
