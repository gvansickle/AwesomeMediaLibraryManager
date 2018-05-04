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

/// KF5
#include <KJob>

/// Ours
#include "ActivityProgressStatusBarWidget.h"
#include <concurrency/AMLMCompositeJob.h>

BaseActivityProgressWidget::BaseActivityProgressWidget(QWidget *parent) : BASE_CLASS(parent),
    m_parent(parent), m_composite_job(new AMLMCompositeJob(this))
{
    m_widget = new BaseActivityProgressStatusBarWidget(m_composite_job, parent);

    // Expand jobs button.
//    auto menu_jobs = new QMenu(this);
    auto button_jobs = new QToolButton(parent);
    button_jobs->setPopupMode(QToolButton::InstantPopup);
    button_jobs->setArrowType(Qt::UpArrow); // Instead of a normal icon.
    button_jobs->setCheckable(true);
//    button_jobs->setMenu(menu_jobs);
//    auto button_jobs_popup = new ActivityProgressPopup(this);
//    button_jobs->addAction(button_jobs_popup);
//    auto button_jobs = new QToolButton(this);
//    button_jobs->setArrowType(Qt::UpArrow); // Instead of a normal icon.
    m_widget->addButton(button_jobs);

    m_expanding_frame_widget = new ExpandingFrameWidget(m_widget);

    connect(button_jobs, &QToolButton::toggled, this, &BaseActivityProgressWidget::toggleSubjobDisplay);
}

BaseActivityProgressWidget::~BaseActivityProgressWidget()
{
}

QWidget* BaseActivityProgressWidget::RootWidget()
{
    return m_widget;
}

void BaseActivityProgressWidget::registerJob(KJob *job)
{
    if(m_activities_to_widgets_map.contains(job))
    {
        return;
    }

    // Create a new widget-based tracker for this job.
    auto pw = new ActivityProgressStatusBarWidget(job, this, m_parent);
    m_activities_to_widgets_map.insert(job, pw);

    m_expanding_frame_widget->addWidget(m_activities_to_widgets_map[job]->widget(nullptr));

    KAbstractWidgetJobTracker::registerJob(job);
}

void BaseActivityProgressWidget::unregisterJob(KJob *job)
{
    KAbstractWidgetJobTracker::unregisterJob(job);

    if(!m_activities_to_widgets_map.contains(job))
    {
        return;
    }

    if(!m_activities_to_widgets_map[job]->m_being_deleted)
    {
        m_expanding_frame_widget->removeWidget(m_activities_to_widgets_map[job]->widget(nullptr));
        delete m_activities_to_widgets_map[job];
    }
    m_activities_to_widgets_map.remove(job);
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

void BaseActivityProgressWidget::showSubJobs()
{
    m_expanding_frame_widget->raise();
    m_expanding_frame_widget->show();
}

void BaseActivityProgressWidget::hideSubJobs()
{
    m_expanding_frame_widget->hide();
}

void BaseActivityProgressWidget::subjobFinished(KJob *job)
{

}

QWidget *BaseActivityProgressWidget::widget(KJob *job)
{
    auto asbw = m_activities_to_widgets_map.value(job, nullptr);
    if(asbw)
    {
        return asbw->widget(job);
    }
    else
    {
        return nullptr;
    }
}

#define M_WIDGET_OR_RETURN(the_job) auto widget = qobject_cast<ActivityProgressStatusBarWidget*>(m_activities_to_widgets_map.value(job, nullptr)); \
    if(!widget) { return; };



