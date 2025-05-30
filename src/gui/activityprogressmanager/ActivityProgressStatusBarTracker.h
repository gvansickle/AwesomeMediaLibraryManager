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

#ifndef SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSSTATUSBARTRACKER_H_
#define SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSSTATUSBARTRACKER_H_

#include <config.h>

// Qt
class QWidget;
class QLabel;
class QToolButton;
class QProgressBar;
#include <QTime>
#include <QMap>
#include <QPointer>
#include <QSharedPointer>
#include <QMutex>

// KF
class KJob;
#include <KAbstractWidgetJobTracker>

// Ours
#include <utils/TheSimplestThings.h>
#include <utils/UniqueIDMixin.h>
#include <concurrency/AMLMJob.h>
#include <concurrency/ThreadsafeMap.h>

#include "CumulativeStatusWidget.h"
#include "CumulativeAMLMJob.h"
class BaseActivityProgressStatusBarWidget;
#include "ExpandingFrameWidget.h"


class ActivityProgressStatusBarTracker;
using ActivityProgressStatusBarTrackerPtr = ActivityProgressStatusBarTracker*;

using TSActiveActivitiesMap = ThreadsafeMap<QPointer<KJob>, QPointer<BaseActivityProgressStatusBarWidget>>;


/**
 * K*WidgetJobTracker tracking the progress/status/controls of a collection of KJobs/AMLMJobs.
 *
 * Sort of a mix of KWidgetJobTracker and KStatusBarJobTracker, but better.
 *
 * @note Derived from KAbstractWidgetJobTracker instead of simply using KStatusBarJobTracker or
 *       KWidgetJobTracker.  The latter is great, but presents a UI really only suitable for use in a QDialog,
 *       while the former would be usable in a status bar, but is missing a lot of basic tracking functionality.
 *
 * @note Due to inheritance from KAbstractWidgetJobTracker, the assumption is that one instance of this class
 *       tracks one AMLMJob instance.  In our use here, that's kind of not applicable, unless you make the
 *       abstraction of all child trackers and jobs into a single "metaAMLMJob", which we don't have yet.
 *       Causes complications; currently the KJob is a nullptr, which causes its own complications.
 *
 */
class ActivityProgressStatusBarTracker: public KAbstractWidgetJobTracker,
        public UniqueIDMixin<ActivityProgressStatusBarTracker>
{
	Q_OBJECT

    using BASE_CLASS = KAbstractWidgetJobTracker;

    using UniqueIDMixin<ActivityProgressStatusBarTracker>::uniqueQObjectName;

Q_SIGNALS:
    /// @name Inherited from KAbstractWidgetJobTracker
    /// @{

    /**
     * "Emitted when the user aborted the operation"
     * KAbstractWidgetJobTracker::slotStop(KJob*) emits this after calling job->kill(KJob::EmitResults).
     */
//    void stopped(KJob *job);
    /**
     * KAbstractWidgetJobTracker::slotSuspend(KJob*) emits this after calling job->suspend().
     * "Emitted when the user suspended the operation"
     */
//    void suspend(KJob *job);
    /**
     * KAbstractWidgetJobTracker::slotResume(KJob*) emits this after calling job->resume().
     * "Emitted when the user resumed the operation".
     */
//    void resume(KJob *job);

    /// @}

    /// For signalling when the number of tracked jobs changes.
    void number_of_jobs_changed(long long new_num_jobs);

    /// @name Internal signals
    /// @{

    /// For forwarding signals from kjobs to their widgets.
    void SIGNAL_finished(KJob*);// (also connected to the unregisterJob() slot)
    void SIGNAL_suspended(KJob*);
    void SIGNAL_resumed(KJob*);
    void SIGNAL_description(KJob *kjob, const QString &title, const std::pair<QString, QString> &field1, const std::pair<QString, QString> &field2);
    void SIGNAL_infoMessage(KJob*);
    void SIGNAL_totalAmount(KJob*, KJob::Unit unit, qulonglong amount);
    void SIGNAL_processedAmount(KJob *job, KJob::Unit unit, qulonglong amount);
    void SIGNAL_percent(KJob*);
    void SIGNAL_speed(KJob*, unsigned long value);


    /// FBO cancelAll().
    void INTERNAL_SIGNAL_slotStop(KJob* kjob);

    /// @}

public:
	/**
     * Constructor.
     * @note parent does have to be a QWidget not a QObject, since that's what the base class takes.
     *       base class' base class KJobTrackerInterface OTOH takes only a QObject* as parent.
     */
    explicit ActivityProgressStatusBarTracker(QWidget* parent = nullptr);
    ~ActivityProgressStatusBarTracker() override;

    /**
     * @link https://api.kde.org/frameworks/kcoreaddons/html/classKJobTrackerInterface.html
     * @link https://api.kde.org/frameworks/kcoreaddons/html/kjobtrackerinterface_8cpp_source.html
     * @link https://api.kde.org/frameworks/kjobwidgets/html/classKAbstractWidgetJobTracker.html
     * @link https://github.com/KDE/kjobwidgets/blob/master/src/kabstractwidgetjobtracker.cpp
     *
     * @note KAbstractWidgetJobTracker inherits from KJobTrackerInterface, and adds some useful functionality:
     *       - "QWidget *widget(KJob *job)" pure-virtual interface for generating/returning the associated QWidget.
     *       - void setStopOnClose(KJob *job, bool stopOnClose) functionality.  Sets whether the KJob should be stopped
     *           if the widget is closed.
     *       - void setAutoDelete(KJob* job, bool autoDelete) functionality.  Sets whether to delete
     *           or only clean the widget.
     *       - New protected slot "slotClean(KJob*)" does nothing, needs to be overridden if any action is necessary.
     *       - Three new signals:
     *         - resume(KJob*)
     *         - stopped(KJob*)
     *         - suspend(KJob*)
     *       - Three related protected slots:
     *         - slotStop(KJob*)/slotSuspend(KJob*)/slotResume(KJob*)
     *         These all have default implementations which call the KJob functions, and look like they'll work
     *         without reimplementation, but something needs to connect signals to them.
     *       - Inherited and overridden protected slot:
     *         - void finished(KJob*)
     *         still does nothing, same as KJobTrackerInterface.
     *
     *  Forwards registerJob()/unregisterJob() to KJobTrackerInterface unchanged.
     */

    /// Override of pure virtual base class version.  Takes a raw KJob*.
    QWidget* widget(KJob* job) override;

    ///
    QWidget* get_status_bar_widget();

    /// Adapted from KWidgetJobTracker::Private, sort of.
    /// There's some sort of weirdness going on with these in KAbstractWidgetJobTracker::Private.
    /// Comment reads:
    /// "### KDE 5: make this methods virtual on KAbstractWidgetJobTracker and get rid out of this workaround. (ereslibre)"
    /// Well it's KF5 now, so let's see if/what difference these make.
    /// They're public in ::Private, so I guess that means they should be private: here.
    /// KAbstractWidgetJobTracker however has them as *non-virtual public* functions which just call
    /// the KAbstractWidgetJobTracker::Private versions (which do nothing).
    /// The KWidgetJobTracker::Private overrides however do do something, so we'll have to duplicate that functionality here.
    ///
    /// ...Yeah, something is pretty wrong with KF5 here.  KJob has at least public setAutoDelete() and isAutoDelete()
    /// members (but still non-virtual).  This is the best I've been able to come up with.
    /// At this point I'm not sure that these members can work through the tracker interface at all.
//public:
//    virtual void setStopOnClose(KJob* kjob, bool stopOnClose);
//    virtual bool stopOnClose(KJob *job) const;
    virtual void setAutoDelete(KJob *kjob, bool autoDelete);
	virtual bool autoDelete(KJob *kjob) const;

    /**
     * @return The number of jobs currently being tracked by this tracker.
     */
    int getNumTrackedJobs() const;

public Q_SLOTS:

    /**
     * Register a KJob, AMLMJob, KIO::Job, or any other job derived from KJob with this tracker.
     *
     * Connects the signals from the passed KJob* to slots in this class of the same name.
     *
     * At some point calls the base class impl, KAbstractWidgetJobTracker.
     * KAbstractWidgetJobTracker::registerJob(KJob *job) simply calls:
     *   KJobTrackerInterface::registerJob(KJob *job) does nothing but connect
     *   many of the KJob signals to slots of this:
     *
     * "The default implementation connects the following KJob signals
     * to the respective protected slots of this class:
     *  - finished() (also connected to the unregisterJob() slot)
     *  - suspended()
     *  - resumed()
     *  - description() (all params)
     *  - infoMessage() (all params)
     *  - warning()     (all params)
     *  - totalAmount()
     *  - processedAmount()
     *  - percent()
     *  - speed()"
     *
     * Other than unregisterJob(), these default slots do nothing in KJobTrackerInterface, and are not overridden
     * in KAbstractWidgetJobTracker.
     *
     * @note Example of the connections made by KJobTrackerInterface::registerJob(KJob *job):
     * @code
     * QObject::connect(job, SIGNAL(processedAmount(KJob*,KJob::Unit,qulonglong)),
     *               this, SLOT(processedAmount(KJob*,KJob::Unit,qulonglong)));
     * @endcode
     *
     * @note KIO::Jobs
     * When we get into KIO Jobs, there are a few things to be aware of:
     *
     * - They appear to registerJob() themselves with KIO::getJobTracker() automatically
     *   if the HideProgressInfo flag is not given to the job's constructor.
     *   This would be OK, but the registration doesn't seem to work properly; i.e. cancel buttons
     *   etc. don't seem to get hooked up unless we manually register the job instead.
     *   Possibly because it's *KJobTrackerInterface* *getJobTracker();???
     *
     * - They start to use KJob signals and slots which aren't automatically connected by the tracker
     * for some reason:
     *  // Emitted when we know the size of this job (data size in bytes for transfers,
     *  // number of entries for listings, etc).  Private signal, emitted by calling setTotalAmount().
     *  - totalSize(KJob*, alonglong size)
     *  // Regularly emitted to show the progress of this job
     *  // (current data size in bytes for transfers, entries listed, etc.).
     *  // Private signal, emit by calling setProcessedAmount().
     *  - processedSize(KJob *job, qulonglong size)
     *
     *  While the "size" value is stored in KJob, there's no accessors to it other than these signals.
     */
    void registerJob(KJob *kjob) override;

    /**
     * A slightly better registration interface, taking a std::weak_ptr to an AMLMJob.
     * @param kjob
     */
//    template <class JobTypePtr>
    void registerJob(/*JobTypePtr*/QSharedPointer<AMLMJob> kjob)
    {
//        if constexpr (std::is_convertible_v</*JobTypePtr*/std::shared_ptr<AMLMJob>, std::weak_ptr<AMLMJob>>)
//        {
            registerJob(kjob.get());
//        }
//        else
//        {
//            static_assert("FAIL");
//        }
    }

    /**
     * From KJobTrackerInterface:
     * "You need to manually call this method only if you re-implemented registerJob() without connecting KJob::finished to this slot."
     * KJTI does this:
     *  void KJobTrackerInterface::unregisterJob(KJob *job)
     *  {
     *     job->disconnect(this);
     *  }
     * KAbstractWJT just calls the above.
     * KJTI does connect the signal->slot (many of them, job->this) in registerJob(), so as long as we ultimately call the base class
     * implementation we're good.
     */
    void unregisterJob(KJob *kjob) override;

    /**
     * Signaled by the destroyed() signal from a registered KJob*.
     */
    void SLOT_onKJobDestroyed(QObject* kjob);

    /**
     * Cancel all tracked KJobs.
     * This blocks waiting for them to report canceled.
     */
    void SLOT_CancelAllKJobs();

protected Q_SLOTS:

	///
    void toggleSubjobDisplay(bool checked);

    /// Slot to display the progress widget for @a kjob.
    void SLOT_onShowProgressWidget(KJob *kjob);

    /**
     * Connections/signal/slots notes
     *
     * KWidgetJobTracker sets the progress signal/slot chain up like this:
     * @code
     * KJob* some_job = ...;
     * KWidgetJobTracker::registerJob(some_job);
     *      -> Creates a widget for the job
     *      -> Adds both to a KWidgetJobTracker::Private job->widget map
     *      KAbstractWidgetJobTracker::registerJob(some_job);
     *          -> KJobTrackerInterface::registerJob(some_job);
     *              -> Signal/slot connections which look like this:
     *              QObject::connect(some_job, SIGNAL(processedAmount(KJob*,KJob::Unit,qulonglong)),
                     this, SLOT(processedAmount(KJob*,KJob::Unit,qulonglong)));
     * @endcode
         So at this point, the KJob's signals are connected to the tracker's
         slots of the same name.

         @note KJobTrackerInterface's slots almost all do nothing.
     *
     * KWidgetJobTracker's slots are overloaded from KJobTrackerInterface's.  In general they
     * follow this pattern:
     * @code
     * void KWidgetJobTracker::processedAmount(KJob *job, KJob::Unit unit, qulonglong amount)
        {
            // Get ptr to widget associated with the given job:
            KWidgetJobTracker::Private::ProgressWidget *pWidget = d->progressWidget.value(job, nullptr);
            if (!pWidget) {
                return;
            }
            // Directly call the appropriate member of the widget with the new status values.
            pWidget->processedAmount(unit, amount);
                -> The widget contains almost all the progress and status information,
                   not the tracker.  E.g.:
                        qulonglong totalSize;
                        qulonglong totalFiles;
                        qulonglong totalDirs;
                        qulonglong processedSize;
                        qulonglong processedDirs;
                        qulonglong processedFiles;
        }
     * @endcode
     *
     * The last part there isn't clear.  KJobPrivate itself contains what should be what's needed
     * and canonical:
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
     *     Sets the processed size. The processedAmount() and percent() signals
     *      are emitted if the values changed. The percent() signal is emitted
     *      only for the progress unit.
     *     void setProcessedAmount(Unit unit, qulonglong amount);
     *
     * Notification of the progress widget
     *
     * We still need to notify the widgets.  In this tracker class, our analogous overrides
     * such as KWidgetJobTracker::processedAmount() will be where it needs to happen.  Options:
     * - Follow the same "directly call the appropriate widget function" pattern
     * - Send a signal instead of a direct call
     * - Send a single "update your status" signal to the widget, and the widget then updates itself by
     *   querying the KJob it's connected to.
     * - ???
     */

    /// @todo There's a bunch of logic in here (tracking number of completed units, speed, etc.) which probably
    /// should be pushed down into a base class.


    /// @name The following slots are inherited from KAbstractWidgetJobTracker and/or KJobTrackerInterface.
    /// @{

    /**
     * Slot finished(), overridden from KJobTrackerInterface, KAbstractWidgetJobTracker.
     *
     * Called when a job is finished, in any case.
     * It is used to notify that the job is terminated and that progress UI (if any) can be hidden.
     * KAbstractWidgetJobTracker implementation does nothing.
     *
     * We send a signal to the the widget to hide itself.
     *
     * @override KJobTrackerInterface, KAbstractWidgetJobTracker.
     */
    void finished(KJob *job) override;
//    void suspended(KJob *job) override;
//    void resumed(KJob *job) override;

    /**
     * Slot, KJobTrackerInterface.
     * "Called to display general description of a job.
     *  A description has a title and two optional fields which can be used to complete the description.
     *  Examples of titles are "Copying", "Creating resource", etc.
     *  The fields of the description can be "Source" with an URL, and, "Destination" with an URL for a "Copying" description."
     */
    void description(KJob *kjob, const QString &title,
                             const std::pair<QString, QString> &field1,
                             const std::pair<QString, QString> &field2) override;
    /**
     * Slot
     * "Called to display state information about a job.
     * Examples of message are "Resolving host", "Connecting to host...", etc."
     */
    void infoMessage(KJob *job, const QString &message) override;
    /**
     * Slot
     * "Emitted to display a warning about a job."
     */
    void warning(KJob *job, const QString &plain) override;

    /// @name Progress tracking protected slots
    /// @{
    /**
     * Directly supported by KJob:
     * - setTotalAmount(Unit,amount)
     * - public qulonglong processedAmount(Unit unit) const;
     * - var in KJobPrivate.
     */
    /// Slot from kjob indicating that setTotalAmount() has been called and d->totalAmount[unit] has
    /// been updated.
    void totalAmount(KJob *kjob, KJob::Unit unit, qulonglong amount) override;
    /**
     * Slot
     * Directly supported by KJob::processedAmount() (setProcessedAmount(Unit,amount), var in KJobPrivate).
     */
    void processedAmount(KJob *job, KJob::Unit unit, qulonglong amount) override;

    /**
     * Directly supported by KJob::percent() (var in KJobPrivate).
     * Also a KJob Q_PROPERTY().
     */
    void percent(KJob *job, unsigned long percent) override;
    void speed(KJob *job, unsigned long value) override;

    /**
     * "This method is called when the widget should be cleaned (after job is finished).
     * redefine this for custom behavior."
     * KAbstractWidgetJobTracker implementation does nothing.
     */
    void slotClean(KJob *job) override;

    // These next three slotXxxx() slots all seem to have reasonable implementations in KAbstractWidgetJobTracker, and only
    // depend on the KJob supporting kill/suspend/resume.

    /**
     * "This [protected Q_SLOT] should be called for correct cancellation of IO operation
     *  Connect this to the progress widgets buttons etc."
     *
     * Calls job->kill(KJob::EmitResult) and emits stopped(job).
     * This override just calls base class.
     *
     * @override KAbstractWidgetJobTracker.
     */
    void slotStop(KJob *kjob) override;

    /**
     * "This method should be called for pause/resume
     * Connect this to the progress widgets buttons etc."
     * Calls job->resume() and emits resume(job).
     */
//    void slotResume(KJob *job) override;
    /**
     * "This method should be called for pause/resume
     * Connect this to the progress widgets buttons etc."
     * Calls job->suspend() and emits suspend(job).
     */
//    void slotSuspend(KJob *job) override;

    /// @} /// END Inherited from KAbstractWidgetJobTracker and/or KJobTrackerInterface.


    /// @name Ours: Protected slots for KJob's "Size" progress.
    /// @{
    virtual void totalSize(KJob *kjob, qulonglong amount);
    virtual void processedSize(KJob* kjob, qulonglong amount);
    /// @}

protected: // Methods

    /// Templated kjob->widget lookup function.
    template <typename JobPointerType, typename Lambda>
    inline void with_widget_or_skip(JobPointerType kjob, Lambda l) const
    {
        Q_CHECK_PTR(kjob);
        // Check if the caller wanted the cumulative widget.
        /// @todo Maybe put this in the regular map, and just be careful not to delete it on e.g. clearAll().
        if(kjob == m_cumulative_status_job)
        {
            l(m_cumulative_status_widget);
            return;
        }

        QPointer<BaseActivityProgressStatusBarWidget> widget = m_amlmjob_to_widget_map.value(kjob, nullptr);
        if(widget)
        {
            l(widget);
            return;
        }

        qWr() << "NO WIDGET FOUND FOR JOB:" << kjob;
//        Q_ASSERT(0);
    }

    /**
     * Returns true if kjob is the "master" KJob.
     */
    bool is_cumulative_status_job(KJob* kjob);

    void make_internal_connections();

    /// Most signal/slot connections will have already been made by the base classes.
    void make_connections_with_newly_registered_job(KJob* kjob, QWidget* wdgt);

    /**
     * Only call this from unregisterJob() and ??? with the mutex locked.
     */
    void INTERNAL_unregisterJob(KJob *kjob);

//    void removeJobAndWidgetFromMap(KJob* kjob, QWidget *widget);

protected:

    int calculate_summary_percent();

protected: // Variable members

    /// Mutex for protecting the tracked job state.
    /// @todo Does this really need to be recursive?
    QRecursiveMutex m_tracked_job_state_mutex;

    /// Map of all registered sub-jobs (KJob*) to sub-job-widgets (QPointer<BaseActivityProgressStatusBarWidget>'s).
    TSActiveActivitiesMap m_amlmjob_to_widget_map;

    /// The QWidget parent of this Tracker, not necessarily it's widget.
    QPointer<QWidget> m_parent_widget {nullptr};

    /// The status widget showing the cumulative status of all registered sub-trackers.
    QPointer<CumulativeStatusWidget> m_cumulative_status_widget {nullptr};
    QPointer<CumulativeAMLMJob> m_cumulative_status_job {nullptr};

    /// Showable/hidable window containing all sub-trackers.
    QPointer<ExpandingFrameWidget> m_expanding_frame_widget {nullptr};

private:
    Q_DISABLE_COPY(ActivityProgressStatusBarTracker)

private:
    /// For the pop-up window.
    void showSubJobs();
    void hideSubJobs();
    void subjobFinished(KJob*);

private Q_SLOTS:

    /**
     * Connected to the App's aboutToShutdown() signal in the constructor.
     * Intent is to kill() all registered jobs.
     * Similar to mechanism in KDevelop's ::ICore & ::ImportProjectJob.
     * @todo How do we kill unkillable kjobs?
     */
    void SLOT_onAboutToShutdown();
};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSSTATUSBARTRACKER_H_ */
