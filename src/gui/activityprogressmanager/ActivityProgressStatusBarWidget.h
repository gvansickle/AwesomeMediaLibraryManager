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

#ifndef SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSSTATUSBARWIDGET_H_
#define SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSSTATUSBARWIDGET_H_

/// Qt5
class QWidget;
class QLabel;
class QProgressBar;
#include <QTime>

/// KF5
class KJob;
#include <KAbstractWidgetJobTracker>

/// Ours
#include "BaseActivityProgressWidget.h"


/**
 * Widget representing the progress/status/controls of a single KJob.
 * Suitable for use in a status bar.
 */
class ActivityProgressStatusBarWidget: public KAbstractWidgetJobTracker
{
	Q_OBJECT

    using BASE_CLASS = KAbstractWidgetJobTracker;

public:
    ActivityProgressStatusBarWidget(KJob *job, BaseActivityProgressWidget* object, QWidget *parent);
	virtual ~ActivityProgressStatusBarWidget();

    QWidget *widget(KJob *job) override;


    BaseActivityProgressWidget *const q;
    KJob *const m_job;
    bool m_being_deleted;

protected Q_SLOTS:

    /// @todo There's a bunch of logic in here (tracking number of completed units, speed, etc.) which probably
    /// should be pushed down into a base class.

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

protected:

    /// Create the widget.
    /// Called by the constructor.
    void init(KJob *job, QWidget *parent);

    qulonglong m_processedSize;
    bool m_is_total_size_known;
    qulonglong m_totalSize;

    QTime startTime;

private:
    Q_DISABLE_COPY(ActivityProgressStatusBarWidget)

    /// The actual widget and its components.
    /// This could maybe be factored out into its own class.
    QWidget* m_widget;
    QLabel* m_current_activity_label;
    QLabel* m_text_status_label;
    QProgressBar* m_progress_bar;


};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSSTATUSBARWIDGET_H_ */
