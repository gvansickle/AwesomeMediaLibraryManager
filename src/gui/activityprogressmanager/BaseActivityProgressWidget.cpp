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

#include "BaseActivityProgressStatusBarWidget.h"
#include "BaseActivityProgressWidget.h"
#include "ExpandingFrameWidget.h"

/// QT5
#include <QWidget>
#include <QToolButton>
#include <QDialog>

/// KF5
#include <KJob>
#include <KToolTipWidget>
#include <QTimer>

/// Ours
#include "ActivityProgressStatusBarWidget.h"
#include <concurrency/AMLMCompositeJob.h>
#include <gui/MainWindow.h>
#include "utils/DebugHelpers.h"

BaseActivityProgressWidget::BaseActivityProgressWidget(QWidget *parent) : BASE_CLASS(parent),
    m_parent(parent)
{
    m_widget = new BaseActivityProgressStatusBarWidget(nullptr, parent);

    // Expand jobs button.
    auto button_show_all_jobs = new QToolButton(parent);
    button_show_all_jobs->setPopupMode(QToolButton::InstantPopup);
    button_show_all_jobs->setArrowType(Qt::UpArrow); // Instead of a normal icon.
    button_show_all_jobs->setCheckable(true);

    m_widget->addButton(button_show_all_jobs);

    m_expanding_frame_widget = new ExpandingFrameWidget();
    m_expanding_frame_widget->hide();
//    m_kttw = new KToolTipWidget(MainWindow::instance());

    connect(button_show_all_jobs, &QToolButton::toggled, this, &BaseActivityProgressWidget::toggleSubjobDisplay);
}

BaseActivityProgressWidget::~BaseActivityProgressWidget()
{
    delete m_expanding_frame_widget;
    m_expanding_frame_widget = nullptr;
}

QWidget* BaseActivityProgressWidget::RootWidget()
{
    return m_widget;
}

void BaseActivityProgressWidget::registerJob(KJob *job)
{
    KAbstractWidgetJobTracker::registerJob(job);

    QTimer::singleShot(500, this, &BaseActivityProgressWidget::onShowProgressWidget);
}

void BaseActivityProgressWidget::unregisterJob(KJob *job)
{
    KAbstractWidgetJobTracker::unregisterJob(job);

    /// @todo removeAll(job) from queue.


}

void BaseActivityProgressWidget::registerJob(AMLMJobPtr job)
{
    // Create a new widget-based tracker for this job.
    /// @note In KWidgetJobTracker, this derives from QWidget.
    auto pw = ActivityProgressStatusBarWidget::make_tracker(job, this, m_parent);
    pw->m_is_job_registered = true;
//    pw->setAttribute(Qt::WA_DeleteOnClose);

    m_activities_to_widgets_map.insert(job, pw);

    /// @todo progressWidgetsToBeShown.enqueue(job);?

    /// @todo
    m_expanding_frame_widget->addWidget(m_activities_to_widgets_map[job]->widget(nullptr));
    m_expanding_frame_widget->reposition();

   registerJob(job.data());
}

void BaseActivityProgressWidget::unregisterJob(AMLMJobPtr job)
{
    unregisterJob(job.data());

    auto p_widget = m_activities_to_widgets_map.value(job, nullptr);
    if(!p_widget)
    {
        return;
    }

    p_widget->m_is_job_registered = false;
/// @todo:    p_widget->deref();

//    if(!m_activities_to_widgets_map[job]->m_being_deleted)
//    {
//        m_expanding_frame_widget->removeWidget(m_activities_to_widgets_map[job]->widget(nullptr));
//        delete m_activities_to_widgets_map[job];
//    }
    //    m_activities_to_widgets_map.remove(job);
}

void BaseActivityProgressWidget::toggleSubjobDisplay(bool checked)
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

void BaseActivityProgressWidget::onShowProgressWidget()
{
    // Called on a timer timeout after a new job is registered.

    /// @todo If queue is empty return.

    /// else dequeue job, look up qwidget, and show it.

}

void BaseActivityProgressWidget::showSubJobs()
{
    // Get the parent-relative geometry of the "root widget".
    auto rect = RootWidget()->frameGeometry();
    qDb() << "Root Frame Rect:" << rect << "parent:" << RootWidget()->parentWidget();

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
//    m_kttw->showBelow(rect, m_expanding_frame_widget, MainWindow::instance()->windowHandle());
}

void BaseActivityProgressWidget::hideSubJobs()
{
    m_expanding_frame_widget->hide();
//    m_kttw->hideLater();
}

void BaseActivityProgressWidget::subjobFinished(KJob *job)
{

}

QWidget *BaseActivityProgressWidget::widget(KJob *job)
{

}

QWidget *BaseActivityProgressWidget::widget(AMLMJobPtr amlmjob)
{
    /// Look up the widget for this job.
    auto asbw = m_activities_to_widgets_map.value(amlmjob, nullptr);
    if(asbw)
    {
        return asbw->widget(amlmjob.data());
    }
    else
    {
        return nullptr;
    }
}

#define M_WIDGET_OR_RETURN(the_job) auto widget = qobject_cast<ActivityProgressStatusBarWidget*>(m_activities_to_widgets_map.value(job, nullptr)); \
    if(!widget) { return; };



