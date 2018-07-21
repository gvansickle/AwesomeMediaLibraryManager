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

#ifndef SRC_GUI_ACTIVITYPROGRESSMANAGER_BASEACTIVITYPROGRESSSTATUSBARWIDGET_H_
#define SRC_GUI_ACTIVITYPROGRESSMANAGER_BASEACTIVITYPROGRESSSTATUSBARWIDGET_H_

/// Qt5
#include <QObject>
#include <QFrame>
#include <QPointer>
#include <QLabel>
class QLabel;
class QProgressBar;
class QToolButton;

/// KF5
class KJob;
class KAbstractWidgetJobTracker;
class KToolTipWidget;

/// Ours
#include <utils/TheSimplestThings.h>
#include "concurrency/AMLMJob.h"
class ActivityProgressStatusBarTracker;

/**
 * A widget for displaying the progress/status/controls of a single AMLMJob.
 * Suitable for use in a status bar.
 */
class BaseActivityProgressStatusBarWidget: public QFrame
{
    Q_OBJECT

    using BASE_CLASS = QFrame;

Q_SIGNALS:
    /// To the tracker: kill the job.
    void cancel_job(KJob* job);

    /// To the tracker: pause the job.
    void pause_job(KJob* job);

    /// To the tracker: resume the job.
    void resume_job(KJob* job);

protected:
    /// Private constructor to get us a fully-constructed vtable so we can
    /// call virtual functions in the non-default constructor.
    explicit BaseActivityProgressStatusBarWidget(QWidget *parent);

public:
    explicit BaseActivityProgressStatusBarWidget(KJob* job, ActivityProgressStatusBarTracker* tracker, QWidget *parent);
    ~BaseActivityProgressStatusBarWidget() override;

    /// Add buttons to the rhs of the layout.
    virtual void addButton(QToolButton* new_button);

public Q_SLOTS:

    /// @name Slots for construction/setup.
    /// @{


    /// @}

    /// @name Public slots analogous to the private versions of KAbstractWidgetJobTracker/KJobTrackerInterface.
    /// @{

    virtual void description(KJob* kjob, const QString& title,
                             const QPair<QString, QString> &field1 = {QString(), QString()},
                        const QPair<QString, QString> &field2 = {QString(), QString()});
    virtual void infoMessage(KJob* kjob, const QString &text);
    virtual void warning(KJob* kjob, const QString &text);

    /// @name Status/progress update slots.
    /// Should be connected to the analogous signals or called from the analogous slots
    /// in the tracker.
    /// Note that these are public, vs. the protected slots in the KJob/Tracker classes.
    /// @{

    /**
     * Directly supported by KJob:
     * - setTotalAmount(Unit,amount)
     * - public qulonglong processedAmount(Unit unit) const;
     * - var in KJobPrivate.
     */
    virtual void totalAmount(KJob *kjob, KJob::Unit unit, qulonglong amount);
    /**
     * Directly supported by KJob::processedAmount() (setProcessedAmount(Unit,amount), var in KJobPrivate).
     */
    virtual void processedAmount(KJob *kjob, KJob::Unit unit, qulonglong amount);

    /**
     * Ours: Slots for "Size" progress.
     * @note These slots do not exist in the KWidgetJobTracker::Private::ProgressWidget class this is
     * somewhat based on, and the signals emitted from KJob are not reflected through the KJobTrackerInterface.
     * Unclear if we really want/need these or not.
     */
    virtual void totalSize(KJob *kjob, qulonglong amount);
    virtual void processedSize(KJob* kjob, qulonglong amount);

    /**
     * Directly supported by KJob::percent() (var in KJobPrivate).
     * Also a KJob Q_PROPERTY().
     */
    virtual void percent(KJob *job, unsigned long percent);

    /**
     * Slot which receives the KJob's speed in bytes/sec.
     * @param job
     * @param value  Speed in bytes/second.
     */
    virtual void speed(KJob *job, unsigned long value);

    /// @}

    /// @} // END Public slots analogous to the private versions of KAbstractWidgetJobTracker.

protected:

    /// Create the widget.
    /// Called by the public constructor.
    virtual void init(KJob* job, QWidget *parent);

    virtual void showTotals();

    virtual void updateMainTooltip();

    /**
     * Override the widget's event handler to intercept QEvent::ToolTip events.
     */
    bool event(QEvent* event) override;

    void closeEvent(QCloseEvent* event) override;

protected Q_SLOTS:

    /// Gives us a slot to hook the cancel button to, which will re-emit cancel_job(m_kjob).
    void INTERNAL_SLOT_emit_cancel_job();

    /// Invoke this to pause or resume the job.
    void INTERNAL_SLOT_SuspendResume(bool clicked);

private:
    Q_DISABLE_COPY(BaseActivityProgressStatusBarWidget)

    ActivityProgressStatusBarTracker* m_tracker {nullptr};
    QPointer<KJob> m_kjob {nullptr};

protected:

    /// @name Status tracking variables.
    /// @{

    /// @todo KWidgetJobTracker has all these tracking vars in the Widget, which
    /// seems pretty wrong.  KJob keeps at least some of this info in the KJob.  And even more is hidden in KJobPrivate.
    bool m_is_total_size_known {false};

    /// @todo KJobs each have one of these in KJobPrivate.
    QTime m_start_time;

    /// @}

    /// @name Child widgets
    /// @{
    /// Primary text.
    QLabel* m_current_activity_label {nullptr};
    /// Detail text.
    QLabel* m_text_status_label {nullptr};
    /// The progress bar.
    QProgressBar* m_progress_bar {nullptr};
    /// Cancel Operation button.
    QToolButton *m_cancel_button {nullptr};
    /// Pause/Resume button.
    QToolButton *m_pause_resume_button {nullptr};

    KToolTipWidget* m_tool_tip_widget {nullptr};
    QLabel* m_tool_tip_label {nullptr};

    /// @}
};

Q_DECLARE_METATYPE(BaseActivityProgressStatusBarWidget*)

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_BASEACTIVITYPROGRESSSTATUSBARWIDGET_H_ */
