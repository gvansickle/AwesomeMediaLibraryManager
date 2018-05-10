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

/// KF5
class KJob;
#include <KAbstractWidgetJobTracker>
#include <QSharedPointer>

/// Ours
//#include "BaseActivityProgressWidget.h"
#include <concurrency/AMLMJob.h>
class ActivityProgressMultiTracker;
class BaseActivityProgressStatusBarWidget;


class ActivityProgressStatusBarTracker;
using ActivityProgressStatusBarWidgetPtr = ActivityProgressStatusBarTracker*;// QSharedPointer<ActivityProgressStatusBarTracker>;


/**
 * K*WidgetJobTracker representing the progress/status/controls of a single KJob.
 * Suitable for use in a status bar.
 */
class ActivityProgressStatusBarTracker: public KAbstractWidgetJobTracker//, public QEnableSharedFromThis<ActivityProgressStatusBarTracker>
{
	Q_OBJECT

    using BASE_CLASS = KAbstractWidgetJobTracker;

public:
    ActivityProgressStatusBarTracker(AMLMJobPtr job, ActivityProgressMultiTracker* parent_tracker, QWidget *parent);

    ~ActivityProgressStatusBarTracker() override;

    /// Override of pure virtual base class version.  Takes a raw KJob*.
    QWidget* widget(KJob* job) override;

    virtual QWidget* widget(AMLMJobPtr job);

    /// The tracker tracking this tracker.
    ActivityProgressMultiTracker* m_parent_tracker {nullptr};

    /// The AMLMJob being tracked by this tracker.
    AMLMJobPtr m_job { nullptr };

public Q_SLOTS:
    void registerJob(KJob *job) override;
    void unregisterJob(KJob *job) override;

    virtual void registerJob(AMLMJobPtr job);
    virtual void unregisterJob(AMLMJobPtr job);


protected Q_SLOTS:

    /// @todo There's a bunch of logic in here (tracking number of completed units, speed, etc.) which probably
    /// should be pushed down into a base class.

    /**
     * The following slots are inherited from KAbstractWidgetJobTracker etc.
     */
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
    // depend on the job supporting kill/suspend/resume.
//    void slotResume(KJob *job) override;
//    void slotStop(KJob *job) override;
//    void slotSuspend(KJob *job) override;


protected:

    /// Create the widget.
    /// Called by the constructor.
    void init(AMLMJobPtr job, QWidget *parent);

    qulonglong m_processedSize {0};
    bool m_is_total_size_known {false};
    qulonglong m_totalSize {0};

    QTime m_start_time;

private:
    Q_DISABLE_COPY(ActivityProgressStatusBarTracker)

    /// The actual widget.
    BaseActivityProgressStatusBarWidget* m_widget {nullptr};
};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSSTATUSBARTRACKER_H_ */
