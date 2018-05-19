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

    virtual void setDescription(const QString& title,
                        const QPair<QString, QString> &field1,
                        const QPair<QString, QString> &field2);
    virtual void setInfoMessage(const QString &text);
    virtual void setWarning(const QString &text);

    bool m_is_job_registered { false };

    // Cribbed from KWidgetJobTracker.
    void ref();
    void deref();

public /*Q_SLOTS*/:
    virtual void setRange(int min, int max);
    virtual void setValue(int val);

protected:

    /// Create the widget.
    /// Called by the public constructor.
    virtual void init(KJob* job, QWidget *parent);

    void closeEvent(QCloseEvent* event) override;

    // Cribbed from KWidgetJobTracker.
    void closeNow();

protected Q_SLOTS:

    /// Invoke this to cancel the job.
    void stop();
    /// Invoke this to pause or resume the job.
    void pause_resume(bool);

private:
    Q_DISABLE_COPY(BaseActivityProgressStatusBarWidget)

    ActivityProgressStatusBarTracker* m_tracker {nullptr};
    KJob* m_job {nullptr};

    int m_refcount {0};

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
