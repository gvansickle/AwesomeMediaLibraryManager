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

#include <QObject>
#include <KJob>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/IdDecorator>
#include <ThreadWeaver/QObjectDecorator>

/// Use this typedef to pass around refs to AMLMJob-derived jobs.
class AMLMJob;
using AMLMJobPtr = QSharedPointer<AMLMJob>;

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
class AMLMJob: public KJob, public ThreadWeaver::Job, public QEnableSharedFromThis<AMLMJob>
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
	// void 	description (KJob *job, const QString &title, const QPair< QString, QString > &field1=QPair< QString, QString >(), const QPair< QString, QString > &field2=QPair< QString, QString >())
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

    /// Signal from KJob::doKill().
    void signalKJobDoKill();

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

public:

    /// KJob-like constructor.
    /// @warning Because of QEnableSharedFromThis<>/std::enable_shared_from_this<>, don't do a "new AMLMJob()",
    ///          or calling sharedFromThis() will/should throw ~std::bad_weak_ptr.  Use AMLMJob::create() instead.
    explicit AMLMJob(QObject* parent = nullptr);

    /// Destructor.
    ~AMLMJob() override;

//    static AMLMJobPtr make_shared(QObject* parent = nullptr);

    /// @name Convesion operators.
    /// @{

    /// To a TW JobPointer, i.e. a QSharedPointer<JobInterface>.
    explicit operator ThreadWeaver::JobPointer() { return asTWJobPointer(); }

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
     * Call this method to ask the Job to abort if it is currently executed. Default implementation of
     * the method does nothing.
     * This method should return immediately, not after the abort has completed.
     */
    void requestAbort() override { /* nothing */ }

    /// @} // END TW::Job overrides.

    /// @name KJob overrides.
    /// @{

    /**
     * "Subclasses must implement start(), which should trigger the execution of the job (although the work should be done asynchronously)."
     */
    Q_SCRIPTABLE void start() override;

    /// @}

    /// Convenience member for getting a KJob* to this.
    KJob* asKJob();

    /// Convenience member for getting a ThreadWeaver::JobPointer (QSharedPointer<JobInterface>) to this.
    ThreadWeaver::JobPointer asTWJobPointer();

    /// Convenience member for getting a ThreadWeaver::IdDecorator* to this.
    ThreadWeaver::IdDecorator* asIdDecorator();

public Q_SLOTS:

    /// From KJob:
    /// @{
    // Default KJob implementations appear to be sufficient.
//    bool kill(KJob::KillVerbosity verbosity=KJob::Quietly);
//    bool resume();
//    bool suspend();
    /// @}

public:

    /// @name KJob overrides.
    /// Making some of the Protected interface Public for the benefit of reporting.
    /// Don't really like this.
    /// @todo I think these don't need to be public now.
    /// @{


//    void setProcessedAmount(Unit unit, qulonglong amount);
//    void setTotalAmount(Unit unit, qulonglong amount);
//    void setPercent(unsigned long percentage);

protected:

    /// @name Override of TW::Job protected functions.
    /// @{
    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread) override = 0;
    void defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread) override;
    void defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread) override;
    /// @}

    /// @name Override of KJob protected functions.
    /// @{

    /**
     * Kill the job.  Default impl just returns false.
     * Abort this job quietly.
     * Simply kill the job, no error reporting or job deletion should be involved.
     *
     * @return true if job successfully killed, false otherwise.
     */
    bool doKill() override;

    bool doSuspend() override;

    bool doResume() override;

    /**
     * Emit the result signal, and suicide this job.
     * @note Deletes this job using deleteLater(). It first notifies the observers to hide the
     *       progress for this job using the finished() signal.
     *       KJob implementation calls finsihJob(), which:
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

    /// @}

    /// @name New protected methods
    /// @{
    /// Make the internal signal-slot connections.
    virtual void make_connections();
    /// @}

    /// New ThreadWeaver::Job-related members.
    /// @{
    bool m_aborted { false };
    bool m_success { true };
    /// @}

protected Q_SLOTS:

    /// @name Internal slots
    /// @{

    /// Handle the doKill() operation.
    void onKJobDoKill();

    /// Handle the KJob::result() signal when the job is finished (except when killed with KJob::Quietly).
    /// Only supposed to call KJob::error() from this slot.
    void onKJobResult(KJob* job);

    /// Always invoked by the KJob::finished signal regardless of reason.
    void onKJobFinished(KJob* job);

    /// @}

private:

    /// @todo Do we also want to keep a copy of JobInterface *decoratee in here?

    /**
     * QSharedPointer to the ThreadWeaver::QObjectDecorator() we'll create and attach as a sort of proxy between us and the
     *
     * @note Two confusingly similar typedefs here:
     *       From qobjectdecorator: "typedef QSharedPointer<QObjectDecorator> QJobPointer;".
     *       From jobinterface.h:   "typedef QSharedPointer<JobInterface> JobPointer;"
     *       Job is derived from JobInterface.  All in ThreadWeaver namespace.
     */
    ThreadWeaver::QJobPointer m_tw_job_qobj_decorator;
};


#endif /* SRC_CONCURRENCY_AMLMJOB_H_ */
