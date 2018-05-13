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
 * To be the Alpha and Omega of *Job classes is a lot of work.  Let's start with the KJob and TW::Job lifecycles.
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
 *  - defaultBegin() override which:
 *    - Calls job()->defaultEnd()
 *    - if(!success) emits failed(self)
 *    - Always emits done(self).
 *  - autoDelete() support.
 */

#include <QObject>
#include <KJob>
#include <QPointer>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/IdDecorator>
#include <ThreadWeaver/QObjectDecorator>

#include "utils/UniqueIDMixin.h"

/// Use the AMLMJobPtr alias to pass around refs to AMLMJob-derived jobs.
class AMLMJob;
using AMLMJobPtr = AMLMJob*; //QSharedPointer<AMLMJob>;

/**
 * Base class for jobs which bridges the hard-to-understand gap between a
 * ThreadWeaver::Job and a KJob-derived class.
 *
 * Goal is to make this one object be both a floor wax and a dessert topping:
 * - A KJob to interfaces which need it, in particular:
 * -- KAbstractWidgetJobTracker and derived classes' registerJob()/unregisterJob() slots.
 * - A ThreadWeaver::Job to interfaces which need it
 * - A ThreadWeaver::IdDecorator for consumption by ThreadWeaver::Job::run(JobPointer self, Thread *thread)
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

    /// ThreadWeaver::QObjectDecorator-like signals, only three:
	/// @{

	// This signal is emitted when this TW::Job is being processed by a thread.
    void started(ThreadWeaver::JobPointer);
    // This signal is emitted when the TW::Job has been finished (no matter if it succeeded or not).
    void done(ThreadWeaver::JobPointer);
    // This signal is emitted when success() returns false after the job is executed.
    void failed(ThreadWeaver::JobPointer);

    /// @}

	/// KJob signals, quite a few:
    ///
	// void 	description (KJob *job, const QString &title, const QPair< QString, QString > &field1=QPair< QString, QString >(), const QPair< QString, QString > &field2=QPair< QString, QString >())

    /// "Emitted when the job is finished, in any case.
    /// It is used to notify observers that the job is terminated and that progress can be hidden.
    /// *** This is a private signal, it can't be emitted directly by subclasses of KJob, use emitResult() instead.
    /// In general, to be notified of a job's completion, client code should connect to result() rather than finished(), so that kill(Quietly) is indeed quiet. However if you store a list of jobs
    /// and they might get killed silently, then you must connect to this instead of result(), to avoid dangling pointers in your list."
    // void finished (KJob *job)

    // void 	infoMessage (KJob *job, const QString &plain, const QString &rich=QString())

    // void 	percent (KJob *job, unsigned long percent)
	// void 	processedAmount (KJob *job, KJob::Unit unit, qulonglong amount)
	// void 	processedSize (KJob *job, qulonglong size)
    // void result (KJob *job)
	// void 	resumed (KJob *job)
	// void 	speed (KJob *job, unsigned long speed)
    // void suspended (KJob *job)
	// void 	totalAmount (KJob *job, KJob::Unit unit, qulonglong amount)
	// void 	totalSize (KJob *job, qulonglong size)
	// void 	warning (KJob *job, const QString &plain, const QString &rich=QString())

    /// KJobs are supported by KJobWidgets
	/// https://api.kde.org/frameworks/kjobwidgets/html/namespaceKJobWidgets.html
	///

    /// @name Internal signals

    // @todo

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
    //    *  - processedAmount()
    //    *  - percent()
    //    *  - speed()
    //    *
    //    * If you re-implement this method, you may want to call the default
    //    * implementation or add at least:
    //    *
    //    * @code
    //    * connect(job, &KJob::finished, this, &MyJobTracker::unregisterJob);
    //    * @endcode
    //    *
    //    * so that you won't have to manually call unregisterJob().

protected:
    /// Protected KJob-like constructor.
    /// Derive from and defer to this from derived classes as part of a two-stage constructor:
    /// Once the derived constructor is called and returns, we'll have a valid AMLMJob this and a valid derived this.
    /// We can then call virtual functions in subsequent constructors.
    /// @note Don't try this at home.
    ///
    /// @warning Because of QEnableSharedFromThis<>/std::enable_shared_from_this<>, don't do a "new AMLMJob()",
    ///          or calling sharedFromThis() will/should throw ~std::bad_weak_ptr.  Use AMLMJob::create() instead.
    ///
    /// @warning This is an abstract base class, there is no AMLMJob::create().
    ///
    explicit AMLMJob(QObject* parent = nullptr);
//    explicit AMLMJob(ThreadWeaver::JobInterface* job, bool auto_delete = true, QObject* parent = nullptr);

public:
    AMLMJob() = delete;
    /// Destructor.
    ~AMLMJob() override;


    /// @name Convesion operators.
    /// @{

    /// To a TW JobPointer, i.e. a QPointer<JobInterface>.
//    explicit operator ThreadWeaver::JobPointer() { return asTWJobPointer(); }

    /// To a QPointer<KJob>.
//    explicit operator QPointer<KJob>() { return asKJobSP(); }

    /// @}

    /// @name TW::Job overrides.
    /// @{

    /**
     * Return whether the Job finished successfully or not.
     * The default implementation simply returns true. Overload in derived classes if the derived Job class can fail.
     *
     * If a job fails (success() returns false), it will *NOT* resolve its dependencies when it finishes. This will make sure that
     * Jobs that depend on the failed job will not be started.
     *
     * There is an important gotcha: When a Job object it deleted, it will always resolve its dependencies. If dependent jobs should
     * not be executed after a failure, it is important to dequeue those before deleting the failed Job. A Sequence may be
     * helpful for that purpose.
     */
    bool success() const override { return m_success; }

    /**
     * Abort the execution of the job.
     * Call this method to ask the Job to abort if it is currently executed.
     * This method should return immediately, not after the abort has completed.
     *
     * @note TW::Job's default implementation of the method does nothing.
     * @note TW::IdDecorator calls the TW::Job's implementation.
     */
    void requestAbort() override;

    /// @} // END TW::Job overrides.

    /// @name KJob overrides.
    /// @{

    /**
     * "Subclasses must implement start(), which should trigger the execution of the job (although the work should be done asynchronously)."
     */
    Q_SCRIPTABLE void start() override;

    /// @}

    /// @name Public interface FBO TW:Job::run().
    /// @{

    /// FBO the success() call.
    void setSuccessFlag(bool success);

//    void setAborted

    /// @}

public Q_SLOTS:

    /// @name KJob job control slots
    /// @note Default KJob implementations appear to be sufficient.  They call
    ///       the protected "doXxxx()" functions which we do need to override below,
    ///       and then emit the proper signals to indicate the deed is done.
    /// @link https://api.kde.org/frameworks/kcoreaddons/html/kjob_8cpp_source.html#l00117
    /// @{

//    bool kill(KJob::KillVerbosity verbosity=KJob::Quietly);
//    bool resume();
//    bool suspend();

    /// @}

protected:

    /// @name Override of TW::IdDecorator protected functions.
    /// @{
    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread) override = 0;
    void defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread) override;
    void defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread) override;
    /// @}

    /// @name Override of KJob protected functions.
    /// @{

    /**
     * Kill the job.
     * Abort this job quietly.
     * Simply kill the job, no error reporting or job deletion should be involved.
     *
     * @note KJob::doKill() simply returns false.
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
     * Emit the result signal, and suicide this job.
     * @note Deletes this job using deleteLater(). It first notifies the observers to hide the
     *       progress for this job using the finished() signal.
     *       KJob implementation calls finshJob(), which:
     *       - Sets isFinished = true
     *       - quit()s the internam event loop
     *       - emits finished(this);
     *       - if(emitResult) emit result(this)
     *       - if(isAutoDelete) deleteLater();
     *       This is probably sufficient behavior and we don't need to overload this (non-virtual) function.
     */
    /// void emitResult();

    /// Defaults of these seem to be suitable.
//    void setError(int errorCode);
//    void setErrorText(const QString &errorText);

    /// @} /// END Override of KJob protected functions.

    /// @name New protected methods
    /// @{

    /// Make the internal signal-slot connections.
    virtual void make_connections();
    virtual void connections_make_defaultBegin(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread);
    virtual void connections_make_defaultExit(const ThreadWeaver::JobPointer &self, ThreadWeaver::Thread *thread);



    /// @}

    /// New protected ThreadWeaver::Job-related members.
    /// @{

public:
    /// Call this in your derived tw::run() function to see if you should cancel the loop.
    bool twWasCancelRequested() const { return m_flag_cancel != 0; }

    /// @}

protected Q_SLOTS:

    /// @name Internal slots
    /// @{

    /// @name From TW::QObjectDecorator's signals.
    void onTWStarted(ThreadWeaver::JobPointer twjob);
    void onTWDone(ThreadWeaver::JobPointer twjob);
    void onTWFailed(ThreadWeaver::JobPointer twjob);

    /// Handle the doKill() operation.
    void onKJobDoKill();

    /// Handle the KJob::result() signal when the job is finished (except when killed with KJob::Quietly).
    /// Only supposed to call KJob::error() from this slot.
    void onKJobResult(KJob* job);

    /// Always invoked by the KJob::finished signal regardless of reason.
    void onKJobFinished(KJob* job);

    /// @}

protected:

private:

    /// Control structs/flags
    QAtomicInt m_flag_cancel {0};
    QAtomicInt m_aborted { 0 };
    QAtomicInt m_success { 1 };
};

#endif /* SRC_CONCURRENCY_AMLMJOB_H_ */
