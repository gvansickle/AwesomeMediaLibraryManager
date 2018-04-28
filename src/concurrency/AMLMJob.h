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

/**
 * Class which bridges the hard-to-understand gap between a ThreadWeaver::{QObject,Id}Decorator and a KJob-derived class.
 * Note multiple inheritance in effect here.  Ok since ThreadWeaver::IdDecorator doesn't inherit from QObject.
 *
 * Goal is to make this one object be:
 * - A KJob to interfaces which need it, in particular:
 * -- KAbstractWidgetJobTracker and derived classes' registerJob()/unregisterJob() slots.
 * - A ThreadWeaver::IdDecorator for consumption by ThreadWeaver::Job::run(JobPointer self, Thread *thread)
 */
class AMLMJob: public KJob, public ThreadWeaver::IdDecorator
{

    Q_OBJECT

    /// ThreadWeaver::Job:
    /// - https://api.kde.org/frameworks/threadweaver/html/classThreadWeaver_1_1Job.html
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
    /// ThreadWeaver::QObjectDecorator signals, only three:
    /*
    *  // This signal is emitted when this job is being processed by a thread.
    *  void started(ThreadWeaver::JobPointer);
    *  // This signal is emitted when the job has been finished (no matter if it succeeded or not).
    *  void done(ThreadWeaver::JobPointer);
    *  // This signal is emitted when success() returns false after the job is executed.
    *  void failed(ThreadWeaver::JobPointer);
    */
    void started(ThreadWeaver::JobPointer);
    void done(ThreadWeaver::JobPointer);
    void failed(ThreadWeaver::JobPointer);

	/// KJob signals, quite a few:
	// void 	description (KJob *job, const QString &title, const QPair< QString, QString > &field1=QPair< QString, QString >(), const QPair< QString, QString > &field2=QPair< QString, QString >())
	// void 	infoMessage (KJob *job, const QString &plain, const QString &rich=QString())
	// void 	percent (KJob *job, unsigned long percent)
	// void 	processedAmount (KJob *job, KJob::Unit unit, qulonglong amount)
	// void 	processedSize (KJob *job, qulonglong size)
	// void 	result (KJob *job)
	// void 	resumed (KJob *job)
	// void 	speed (KJob *job, unsigned long speed)
	// void 	suspended (KJob *job)
	// void 	totalAmount (KJob *job, KJob::Unit unit, qulonglong amount)
	// void 	totalSize (KJob *job, qulonglong size)
	// void 	warning (KJob *job, const QString &plain, const QString &rich=QString())
	/// KJobs are supported by KJobWidgets
	/// https://api.kde.org/frameworks/kjobwidgets/html/namespaceKJobWidgets.html
	///

    /// KJobTrackerInterface:
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
    /**
     * ThreadWeaver::QObjectDecorator-like constructor.
     */
    AMLMJob(ThreadWeaver::JobInterface *decoratee, bool autoDelete, QObject *parent = nullptr);
    ~AMLMJob() override;

//    static AMLMJob* make_amlmjob(ThreadWeaver::Job* tw_job);

    Q_SCRIPTABLE void start() override;

public:
    // Making some of the Protected interface Public for the benefit of reporting.
    // Don't really like this.

    void setError(int errorCode);
    void setErrorText(const QString &errorText);
    void setProcessedAmount(Unit unit, qulonglong amount);
    void setTotalAmount(Unit unit, qulonglong amount);
    void setPercent(unsigned long percentage);

    void emitResult();

protected:
    /**
     * Override of ::IdDecorator.
     */
    void defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread) override;
    void defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread) override;

private:

    /// @todo Do we also want to keep a copy of JobInterface *decoratee in here?

    /**
     * The ThreadWeaver::QObjectDecorator() we'll create and attach as a sort of proxy between us and the
     */
    ThreadWeaver::QObjectDecorator* m_tw_job_qobj_decorator;
};

#endif /* SRC_CONCURRENCY_AMLMJOB_H_ */
