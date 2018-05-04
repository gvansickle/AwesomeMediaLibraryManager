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

#ifndef SRC_GUI_ACTIVITYPROGRESSMANAGER_BASEACTIVITYPROGRESSWIDGET_H_
#define SRC_GUI_ACTIVITYPROGRESSMANAGER_BASEACTIVITYPROGRESSWIDGET_H_

/// QT5
class QWidget;
#include <QMap>

/// KF5
class KJob;
#include <KAbstractWidgetJobTracker>

/// Ours
class ActivityProgressStatusBarWidget;
class BaseActivityProgressStatusBarWidget;
class ExpandingFrameWidget;
class AMLMCompositeJob;

/**
 * Base class for AMLMJob widget-based multiple-job trackers.
 *
 * @note Derived from KAbstractWidgetJobTracker instead of simply using KStatusBarJobTracker or
 *       KWidgetJobTracker.  The latter is great, but presents a UI really only suitable for use in a QDialog,
 *       while the former would be usable in a status bar, but is missing a lot of basic tracking functionality.
 *
 * @note Due to inheritenace from KAbstractWidgetJobTracker, one instance of this class tracks one AMLMJob instance.
 *       That's fine because that's the intent here.
 */
class BaseActivityProgressWidget: public KAbstractWidgetJobTracker
{
    Q_OBJECT

    using BASE_CLASS = KAbstractWidgetJobTracker;

Q_SIGNALS:

public:
    explicit BaseActivityProgressWidget(QWidget* parent = nullptr);
	~BaseActivityProgressWidget() override;

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

    QWidget *widget(KJob *job) override;

public Q_SLOTS:
    void registerJob(KJob *job) override;
    void unregisterJob(KJob *job) override;

public Q_SLOTS:


protected:
    using ActiveActivitiesMap = QMap<KJob*, ActivityProgressStatusBarWidget*>;

    /// Map of all registered sub-Activities to sub-job-trackers.
    ActiveActivitiesMap m_activities_to_widgets_map;

    QWidget* m_parent {nullptr};

    /// The status widget showing the cumulative status of all registered sub-trackers.
    BaseActivityProgressStatusBarWidget* m_widget {nullptr};

    /// Showable/hidable window containing all sub-trackers.
    ExpandingFrameWidget* m_expanding_frame_widget {nullptr};

    AMLMCompositeJob* m_composite_job {nullptr};

protected Q_SLOTS:
    /**
     * The following slots are inherited.
     */
//    void finished(KJob *job) override;
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

};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_BASEACTIVITYPROGRESSWIDGET_H_ */
