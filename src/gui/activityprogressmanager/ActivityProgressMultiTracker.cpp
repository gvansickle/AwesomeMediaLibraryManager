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

#include "ActivityProgressMultiTracker.h"

#include "BaseActivityProgressStatusBarWidget.h"
#include "ExpandingFrameWidget.h"

/// QT5
#include <QWindow>
#include <QWidget>
#include <QToolButton>
#include <QDialog>
#include <QSharedPointer>

/// KF5
#include <KJob>
#include <KToolTipWidget>
#include <QTimer>

/// Ours
#include <concurrency/AMLMJob.h>
#include <concurrency/AMLMCompositeJob.h>
#include <gui/MainWindow.h>
#include "ActivityProgressStatusBarTracker.h"
#include "utils/TheSimplestThings.h"

template <typename Lambda>
void with_widget_or_skip(BaseActivityProgressStatusBarWidget* widget, Lambda l)
{
    if(widget)
    {
        l(widget);
    }
}

ActivityProgressMultiTracker::ActivityProgressMultiTracker(QWidget *parent) : BASE_CLASS(parent),
    m_parent(parent)
{
	// The summary widget.
	/// @todo nullptr AMLMJob.
    m_widget = new BaseActivityProgressStatusBarWidget(nullptr, this, parent);

    // Expand jobs button.
    auto button_show_all_jobs = new QToolButton(parent);
    button_show_all_jobs->setPopupMode(QToolButton::InstantPopup);
    button_show_all_jobs->setArrowType(Qt::UpArrow); // Instead of a normal icon.
    button_show_all_jobs->setCheckable(true);

    m_widget->addButton(button_show_all_jobs);

    m_expanding_frame_widget = new ExpandingFrameWidget(m_widget);

    /// @note Set Window type to be a top-level window, i.e. a Qt::Window, Qt::Popup, or Qt::Dialog (and a few others mainly Mac).
//    m_expanding_frame_widget->setWindowFlags(Qt::Popup);
//    m_expanding_frame_widget->setParent(parent);
//    m_expanding_frame_widget->windowHandle()->setTransientParent(MainWindow::instance()->windowHandle());

    m_expanding_frame_widget->hide();

    connect(button_show_all_jobs, &QToolButton::toggled, this, &ActivityProgressMultiTracker::toggleSubjobDisplay);
}

ActivityProgressMultiTracker::~ActivityProgressMultiTracker()
{
    delete m_expanding_frame_widget;
    m_expanding_frame_widget = nullptr;
}

QWidget* ActivityProgressMultiTracker::RootWidget()
{
    return m_widget;
}

void ActivityProgressMultiTracker::registerJob(KJob *job)
{
    Q_ASSERT(0);
    KAbstractWidgetJobTracker::registerJob(job);

    QTimer::singleShot(500, this, &ActivityProgressMultiTracker::onShowProgressWidget);
}

void ActivityProgressMultiTracker::unregisterJob(KJob *job)
{
M_WARNING("CRASH: Cancel can cause job == 0 here, not always though.");
//    auto amlmptr = qobject_cast<AMLMJob*>(job);
    auto amlmptr = qObjectCast<AMLMJob>(job);
    if(amlmptr)
    {
        qWr() << "GOT AMLMPTR as KJOB*, forwarding";
        // Forward to the AMLM unregister.
        unregisterJob(amlmptr);
    }
    else
    {
        Q_ASSERT(0);
        KAbstractWidgetJobTracker::unregisterJob(job);
    }

    /// @todo removeAll(job) from queue.


}

void ActivityProgressMultiTracker::registerJob(AMLMJobPtr job)
{
    Q_ASSERT(job != nullptr);

    qDb() << "KJobTrk: AMLMJobPtr:" << job;
    qDb() << "KJobTrk:" << M_NAME_VAL(job->capabilities())
          << M_NAME_VAL(job->isSuspended())
          << M_NAME_VAL(job->isAutoDelete())
          << M_NAME_VAL(job->error())
          << M_NAME_VAL(job->errorText())
          << M_NAME_VAL(job->errorString());

    // Create a new widget-based single-job tracker for this job.
    /// @note In KWidgetJobTracker, this derives from QWidget.
    auto new_tracker = new ActivityProgressStatusBarTracker(job, /*parent_tracker=*/this, /*parent QObject=*/m_parent);

    Q_ASSERT(new_tracker != nullptr);

    m_amlmjob_to_tracker_map.insert(job, new_tracker);

    /// @todo progressWidgetsToBeShown.enqueue(job);?

    /// Add tracker's widget to the frame.
    m_expanding_frame_widget->addWidget(m_amlmjob_to_tracker_map[job]->widget(job));
//    m_expanding_frame_widget->addWidget(pw);
    m_expanding_frame_widget->reposition();

#if 0
    /// @todo Already registered in child tracker, need/want this too?
//    BASE_CLASS::registerJob(job);
#else
    // Not calling the base class's registerJob() here, so need to connect job/finished to this/unregisterJob.
//    QObject::connect(job, /*&AMLMJob::*/SIGNAL(finished(KJob*)), this, /*&ActivityProgressMultiTracker::*/SLOT(unregisterJob(AMLMJobPtr)));
//    QObject::connect(job, &KJob::finished, this, [=](KJob* kjob) {
////        AMLMJobPtr amlm_job_sp = qSharedPointerObjectCast<AMLMJob>(kjob);
//        AMLMJobPtr amlm_job_sp = qobject_cast<AMLMJob*>(kjob);
//        unregisterJob(amlm_job_sp);});
    connect(job, &KJob::finished, this, &ActivityProgressMultiTracker::finished);
#endif

    QTimer::singleShot(500, this, &ActivityProgressMultiTracker::onShowProgressWidget);
}

void ActivityProgressMultiTracker::unregisterJob(AMLMJobPtr job)
{
    /// @todo Already unregistered in child tracker, need/want this too?
    /// @answ Possibly, KAbstWJT keeps a map of jobs here too.  If we register above, we need to unregister.
//    BASE_CLASS::unregisterJob(job);

    /// @todo remove from to-be-shown queue?

    auto p_tracker = m_amlmjob_to_tracker_map.value(job, nullptr);
    if(!p_tracker)
    {
        qWr() << "DIDNT FIND TRACKER FOR JOB:" << job;
        return;
    }

    qWr() << "REMOVING TRACKER:" << p_tracker << "FOR JOB:" << job;
    m_amlmjob_to_tracker_map.remove(job);

    /// @todo less? More?
    p_tracker->deleteLater();
}

void ActivityProgressMultiTracker::toggleSubjobDisplay(bool checked)
{
    if(checked)
    {
        showSubJobs();
    }
    else
    {
        hideSubJobs();
    }
}

void ActivityProgressMultiTracker::onShowProgressWidget()
{
    // Called on a timer timeout after a new job is registered.

    /// @todo If queue is empty return.

    /// else dequeue job, look up qwidget, and show it.

}

void ActivityProgressMultiTracker::finished(KJob *job)
{
    // Any progress UI related to job can be hidden.

    qDb() << "SLOT FINISHED, KJOB:" << job;

    AMLMJobPtr amlm_job_sp = qobject_cast<AMLMJob*>(job);
    Q_CHECK_PTR(amlm_job_sp);
    unregisterJob(amlm_job_sp);

/// @todo    pw->destroyLater();
}

void ActivityProgressMultiTracker::showSubJobs()
{
    // Get the parent-relative geometry of the "root widget".
    auto rect = RootWidget()->frameGeometry();
    qDb() << "Root Frame Rect:" << rect << "parent:" << RootWidget()->parentWidget();

//    m_expanding_frame_widget->windowHandle()->setTransientParent(RootWidget()->windowHandle());


    // Translate the the root widget's topLeft() to MainWindow coords.
    auto pos_tl_global = RootWidget()->mapToGlobal(rect.topLeft());
    qDb() << "Root Frame topLeft(), Global:" << pos_tl_global;

//    m_expanding_frame_widget->popup(pos_tl_global);
    m_expanding_frame_widget->raise();
    m_expanding_frame_widget->show();

#if 0
    m_expanding_frame_widget->raise();
    m_expanding_frame_widget->show();

//    m_expanding_frame_widget->updateGeometry();

    // Get the parent-relative geometry of the "root widget".
    auto rect = RootWidget()->frameGeometry();
    qDb() << "Root Frame Rect:" << rect << "parent:" << RootWidget()->parentWidget();

    // Translate the the root widget's topLeft() to MainWindow coords.
    auto pos_tl_global = RootWidget()->mapToGlobal(rect.topLeft());
    qDb() << "Root Frame topLeft(), Global:" << pos_tl_global;

    // Get the parent-relative pos of the expanding frame.
    auto frame_pos_pr = m_expanding_frame_widget->pos();
    qDb() << "Exp Frame topLeft():" << frame_pos_pr << "parent:" << m_expanding_frame_widget->parentWidget();

    // Global.
    auto frame_pos_global = m_expanding_frame_widget->mapToGlobal(frame_pos_pr);
    qDb() << "Exp Frame topLeft() Global:" << frame_pos_pr;

    auto frame_rect_pr = m_expanding_frame_widget->frameGeometry();
    qDb() << "Exp Frame frameGeometry():" << frame_rect_pr;

    // New width.
    auto new_exp_w = rect.width();

    // Move the bottomLeft() of the frame to the topLeft() of the root widget.
    frame_rect_pr.moveBottomLeft(m_expanding_frame_widget->parentWidget()->mapFromGlobal(pos_tl_global));
    Q_ASSERT(frame_rect_pr.isValid());
    m_expanding_frame_widget->setGeometry(frame_rect_pr);
    m_expanding_frame_widget->setMaximumWidth(new_exp_w);
    qDb() << "Max size:" << m_expanding_frame_widget->maximumSize();
#endif
}

void ActivityProgressMultiTracker::hideSubJobs()
{
    m_expanding_frame_widget->hide();
}

void ActivityProgressMultiTracker::subjobFinished(KJob *job)
{

}

QWidget *ActivityProgressMultiTracker::widget(KJob *job)
{
    /// @todo Should only be called with a nullptr KJob*, indicating it's the master progress widget that's being asked for.
    Q_ASSERT(job == nullptr);

    Q_ASSERT(0);
}

QWidget *ActivityProgressMultiTracker::widget(AMLMJobPtr amlmjob)
{
    Q_CHECK_PTR(amlmjob);

    /// Look up the widget for this job.
    auto tracker = m_amlmjob_to_tracker_map.value(amlmjob, nullptr);
    Q_CHECK_PTR(tracker);

    auto widget = tracker->widget(amlmjob);
    Q_CHECK_PTR(widget);

    return widget;
}



