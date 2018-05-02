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

#include "BaseActivityProgressWidget.h"

/// QT5
#include <QWidget>

/// KF5
#include <KJob>

/// Ours
#include "ActivityProgressStatusBarWidget.h"

BaseActivityProgressWidget::BaseActivityProgressWidget(QWidget *parent) : BASE_CLASS(parent),
    m_parent(parent)
{

}

BaseActivityProgressWidget::~BaseActivityProgressWidget()
{
}

void BaseActivityProgressWidget::registerJob(KJob *job)
{
    KAbstractWidgetJobTracker::registerJob(job);

    if(m_activities_to_widgets_map.contains(job))
    {
        return;
    }

    // Create a new widget for this job.
    auto pw = new ActivityProgressStatusBarWidget(job, this, m_parent);
    m_activities_to_widgets_map.insert(job, pw);
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
        delete m_activities_to_widgets_map[job];
    }
    m_activities_to_widgets_map.remove(job);
}

QWidget *BaseActivityProgressWidget::widget(KJob *job)
{
    if(!m_activities_to_widgets_map.contains(job))
    {
        return nullptr;
    }

    return m_activities_to_widgets_map[job];
}

