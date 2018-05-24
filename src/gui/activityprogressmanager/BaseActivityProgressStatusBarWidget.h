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
class QLabel;
class QProgressBar;
class QToolButton;

/// KF5
class KJob;
class KAbstractWidgetJobTracker;

/// Ours
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
//    void pause_job(AMLMJobPtr job);

    /// To the tracker: resume the job.
//    void resume_job(AMLMJobPtr job);

    /// To the tracker: Remove the this widget and its job from the map.
    void signal_removeJobAndWidgetFromMap(KJob* job, QWidget *parent);

private:
    /// Private constructor to get us a fully-constructed vtable so we can
    /// call virtual functions in the non-default constructor.
    BaseActivityProgressStatusBarWidget(QWidget *parent);

public:
    explicit BaseActivityProgressStatusBarWidget(KJob* job, ActivityProgressStatusBarTracker* tracker, QWidget *parent);
    ~BaseActivityProgressStatusBarWidget() override;

    /// Add buttons to the rhs of the layout.
    virtual void addButton(QToolButton* new_button);



    bool m_is_job_registered { false };

    // Cribbed from KWidgetJobTracker.
    void ref();
    void deref();

public Q_SLOTS:
    virtual void setDescription(const QString& title,
                        const QPair<QString, QString> &field1,
                        const QPair<QString, QString> &field2);
    virtual void setInfoMessage(const QString &text);
    virtual void setWarning(const QString &text);

    virtual void setTotalAmount(KJob *kjob, KJob::Unit unit, qulonglong amount);
    virtual void setProcessedAmount(KJob *kjob, KJob::Unit unit, qulonglong amount);

    virtual void setRange(int min, int max);
    virtual void setValue(int val);

    /// @todo This feels hackish.  These really should be slots if anything, and should
    /// be connected to the associated tracker or maybe KJob.
    /// The whole KJob status interface is protected.  In this percent() case, the KJob has
    /// a protected setPercent(unsigned long pct) function which then emits the percent() signal.
    /// This is a simple public forwarder FBO ActivityProgressStatusBarTracker's cumulative progress widget.
    virtual void setPercent(unsigned long pct);

protected:

    /// Create the widget.
    /// Called by the public constructor.
    virtual void init(KJob* job, QWidget *parent);

    /// Make the necessary connections between this Widget, the KJob, and the tracker.
    /// @todo Currently unused.
    void make_connections(void);

    void closeEvent(QCloseEvent* event) override;

    // Cribbed from KWidgetJobTracker.
    void closeNow();

protected Q_SLOTS:

    /// @todo Not clear why the functionality of these two are in the Widget; seems like it should be in the tracker.

    /// Invoke this to cancel the job.
    void stop();

    /// Invoke this to pause or resume the job.
    void pause_resume(bool);

private:
    Q_DISABLE_COPY(BaseActivityProgressStatusBarWidget)

    ActivityProgressStatusBarTracker* m_tracker {nullptr};
    QPointer<KJob> m_kjob {nullptr};

    int m_refcount {0};

public: /// @todo FBO tracker, could friend.

    /// @name Status tracking variables.
    /// @{

    /// @todo Another map? or?? KWidgetJobTracker has all these tracking vars in the Widget, which
    /// seems pretty wrong.  KJob keeps at least some of this info in the KJob.  And even more is hidden in KJobPrivate.
    qulonglong m_processedSize {0};
    bool m_is_total_size_known {false};
    qulonglong m_totalSize {0};
    /// @todo KJobs each have one of these in KJobPrivate.
    QTime m_start_time;

    /// @}

protected: ///< FBO CumulativeStatusWidget.  Probably a better way to do this.

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
    /// @}
};

Q_DECLARE_METATYPE(BaseActivityProgressStatusBarWidget*)

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_BASEACTIVITYPROGRESSSTATUSBARWIDGET_H_ */
