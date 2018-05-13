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

#ifndef SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSMULTITRACKER_H_
#define SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSMULTITRACKER_H_

/// QT5
class QWidget;
class QDialog;
#include <QMap>

/// KF5
class KJob;
class KToolTipWidget;
#include <KAbstractWidgetJobTracker>

/// Ours
#include "ActivityProgressStatusBarTracker.h"
class BaseActivityProgressStatusBarWidget;
class ExpandingFrameWidget;
class AMLMCompositeJob;

/**
 * (Base?) class for AMLMJob widget-based multiple-job trackers.
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
class ActivityProgressMultiTracker: public KAbstractWidgetJobTracker
{
    Q_OBJECT

    using BASE_CLASS = KAbstractWidgetJobTracker;

Q_SIGNALS:

public:
    /**
     * Constructor.
     * @note parent does have to be a QWidget not a QObject, since that's what the base class takes.
     *       base class' base class KJobTrackerInterface OTOH takes only a QObject* as parent.
     */
    explicit ActivityProgressMultiTracker(QWidget* parent = nullptr);
    ~ActivityProgressMultiTracker() override;

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

    QWidget* RootWidget();

    /// @todo De-public?
    QWidget *widget(KJob *job) override;

    virtual QWidget* widget(AMLMJobPtr amlmjob);

public Q_SLOTS:
    void registerJob(KJob *job) override;
    void unregisterJob(KJob *job) override;

    /// Creates a new tracker for @a job and registers it in the job->tracker map.
    virtual void registerJob(AMLMJobPtr job);

    /**
     * From KJobTrackerInterface:
     * "You need to manually call this method only if you re-implemented registerJob() without connecting KJob::finished to this slot."
     * KJTI does this:
     *  void KJobTrackerInterface::unregisterJob(KJob *job)
     *  {
     *     job->disconnect(this);
     *  }
     * KAbstractWJT just calls the above.
     * KJTI does connect the signal->slot, so as long as we ultimately call the base class implementation in register we're good.
     */
    virtual void unregisterJob(AMLMJobPtr job);

protected:

    using ActiveActivitiesMap = QMap<AMLMJobPtr, ActivityProgressStatusBarTracker*>;

    /// Map of all registered sub-Activities (AMLMJobPtrs) to sub-job-trackers (ActivityProgressStatusBarTracker*'s).
    ActiveActivitiesMap m_amlmjob_to_tracker_map;

    QWidget* m_parent {nullptr};

    /// The status widget showing the cumulative status of all registered sub-trackers.
    BaseActivityProgressStatusBarWidget* m_widget {nullptr};

    /// Showable/hidable window containing all sub-trackers.
    ExpandingFrameWidget* m_expanding_frame_widget {nullptr};

protected Q_SLOTS:

    void toggleSubjobDisplay(bool checked);

    void onShowProgressWidget();

    /**
     * The following slots are inherited.
     */
    /// Called when a job is finished, in any case.
    /// It is used to notify that the job is terminated and that progress UI (if any) can be hidden.
    /// KAbstractWidgetJobTracker implementation does nothing.
    void finished(KJob *job) override;
//    void suspended(KJob *job) override;
//    void resumed(KJob *job) override;

//    void description(KJob *job, const QString &title,
//                             const QPair<QString, QString> &field1,
//                             const QPair<QString, QString> &field2) override;
//    void infoMessage(KJob *job, const QString &plain, const QString &rich) override;
//    void warning(KJob *job, const QString &plain, const QString &rich) override;

//    void totalAmount(KJob *job, KJob::Unit unit, qulonglong amount) override;
//    void processedAmount(KJob *job, KJob::Unit unit, qulonglong amount) override;
//    void percent(KJob *job, unsigned long percent) override;
//    void speed(KJob *job, unsigned long value) override;

//    void slotClean(KJob *job) override;

private:
    void showSubJobs();
    void hideSubJobs();
    void subjobFinished(KJob*);

};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSMULTITRACKER_H_ */
