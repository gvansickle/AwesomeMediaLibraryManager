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

/// Qt5
class QWidget;
class QLabel;
class QToolButton;
class QProgressBar;
#include <QTime>
#include <QMap>
#include <QPointer>
#include <QSharedPointer>
#include <QMutex>

/// KF5
class KJob;
#include <KAbstractWidgetJobTracker>

/// Ours
#include <utils/TheSimplestThings.h>
#include <utils/UniqueIDMixin.h>
#include <concurrency/AMLMJob.h>
#include <concurrency/ThreadsafeMap.h>
//#include "BaseActivityProgressStatusBarWidget.h"
#include "CumulativeStatusWidget.h"
class BaseActivityProgressStatusBarWidget;
//class ExpandingFrameWidget;
#include "ExpandingFrameWidget.h"


class ActivityProgressStatusBarTracker;
using ActivityProgressStatusBarWidgetPtr = ActivityProgressStatusBarTracker*;

using TSActiveActivitiesMap = ThreadsafeMap<KJob*, QPointer<BaseActivityProgressStatusBarWidget>>;

/**
 * K*WidgetJobTracker tracking the progress/status/controls of a single AMLMJob.
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
class ActivityProgressStatusBarTracker: public KAbstractWidgetJobTracker, public UniqueIDMixin<ActivityProgressStatusBarTracker>
{
	Q_OBJECT

    using BASE_CLASS = KAbstractWidgetJobTracker;

    using UniqueIDMixin<ActivityProgressStatusBarTracker>::uniqueQObjectName;

Q_SIGNALS:
    /// @name Inherited from KAbstractWidgetJobTracker
    /// @{

    /// KAbstractWidgetJobTracker::slotStop(KJob*) emits this after calling job->kill(KJob::EmitResults).
//    void stopped(KJob *job);
    /// KAbstractWidgetJobTracker::slotSuspend(KJob*) emits this after calling job->suspend().
//    void suspend(KJob *job);
    /// KAbstractWidgetJobTracker::slotResume(KJob*) emits this after calling job->resume().
//    void resume(KJob *job);

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

public Q_SLOTS:

    /**
     * Register a KJob (or AMLMJob or any other derived job) with this tracker.
     *
     * At some point calls the base class impl, KAbstractWidgetJobTracker.
     * KAbstractWidgetJobTracker::registerJob(KJob *job) simply calls:
     *   KJobTrackerInterface::registerJob(KJob *job) does nothing but connect
     *   many of the KJob signals to slots of this.
     */
    void registerJob(KJob *kjob) override;

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

    /// FBO closeNow().
    /// This is emitted by child progress widgets in their "closeNow()" members.
    void SLOT_removeJobAndWidgetFromMap(KJob *ptr, QWidget* widget);

    /// FBO closeEvent()
    void SLOT_directCallSlotStop(KJob* kjob);

protected Q_SLOTS:

	/// @todo NEW
    void toggleSubjobDisplay(bool checked);
    void onShowProgressWidget(KJob *kjob);

    /// Cancel all tracked KJobs.
    void cancelAll();

    /// @todo There's a bunch of logic in here (tracking number of completed units, speed, etc.) which probably
    /// should be pushed down into a base class.

    /**
     * The following slots are inherited from KAbstractWidgetJobTracker etc.
     */
    /// Called when a job is finished, in any case.
    /// It is used to notify that the job is terminated and that progress UI (if any) can be hidden.
    /// KAbstractWidgetJobTracker implementation does nothing.
    void finished(KJob *job) override;
//    void suspended(KJob *job) override;
//    void resumed(KJob *job) override;


    /**
     * "Called to display general description of a job.
     *  A description has a title and two optional fields which can be used to complete the description.
     *  Examples of titles are "Copying", "Creating resource", etc.
     *  The fields of the description can be "Source" with an URL, and, "Destination" with an URL for a "Copying" description."
     */
    void description(KJob *job, const QString &title,
                             const QPair<QString, QString> &field1,
                             const QPair<QString, QString> &field2) override;
    /**
     * "Called to display state information about a job.
     * Examples of message are "Resolving host", "Connecting to host...", etc."
     */
    void infoMessage(KJob *job, const QString &plain, const QString &rich) override;
    /**
     * "Emitted to display a warning about a job."
     */
    void warning(KJob *job, const QString &plain, const QString &rich) override;

    void totalAmount(KJob *job, KJob::Unit unit, qulonglong amount) override;
    void processedAmount(KJob *job, KJob::Unit unit, qulonglong amount) override;
    void percent(KJob *job, unsigned long percent) override;
    void speed(KJob *job, unsigned long value) override;

    /// KAbstractWidgetJobTracker implementation does nothing.
    void slotClean(KJob *job) override;

    // These all seem to have reasonable implementations in KAbstractWidgetJobTracker, and only
    // depend on the KJob supporting kill/suspend/resume.
//    void slotResume(KJob *job) override;
//    void slotStop(KJob *job) override;
//    void slotSuspend(KJob *job) override;

protected: // Methods

    /// Templated job->widget lookup function.
    template <typename JobPointerType, typename Lambda>
    void with_widget_or_skip(JobPointerType job, Lambda l)
    {
        QPointer<BaseActivityProgressStatusBarWidget> widget = m_amlmjob_to_widget_map.value(job, nullptr);
        if(widget)
        {
            l(widget);
        }
    }

    void make_connections_with_newly_registered_job(KJob* kjob, QWidget* wdgt);

    void removeJobAndWidgetFromMap(KJob* ptr, QWidget *widget);
    void directCallSlotStop(KJob *kjob);

    int calculate_summary_percent();

protected: // Variable members

    /// Map of all registered sub-jobs (KJob*) to sub-job-widgets (QPointer<BaseActivityProgressStatusBarWidget>'s).
    TSActiveActivitiesMap m_amlmjob_to_widget_map;

    /// Mutex for protecting the public Thread-Safe Interface.
    QMutex m_tsi_mutex;

    /// @todo Another map? or?? KWidgetJobTracker has all these tracking vars in the Widget, which
    /// seems pretty wrong.
    qulonglong m_processedSize {0};
    bool m_is_total_size_known {false};
    qulonglong m_totalSize {0};
    /// @todo KJobs each have one of these in KJobPrivate.
    QTime m_start_time;

    /// The QWidget parent of this Tracker, not necessarily it's widget.
    QPointer<QWidget> m_parent_widget {nullptr};

    /// The status widget showing the cumulative status of all registered sub-trackers.
    QPointer<CumulativeStatusWidget> m_cumulative_status_widget {nullptr};

    /// Showable/hidable window containing all sub-trackers.
    QPointer<ExpandingFrameWidget> m_expanding_frame_widget {nullptr};

private:
    Q_DISABLE_COPY(ActivityProgressStatusBarTracker)

	/// @todo NEW
    void showSubJobs();
    void hideSubJobs();
    void subjobFinished(KJob*);

};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSSTATUSBARTRACKER_H_ */
