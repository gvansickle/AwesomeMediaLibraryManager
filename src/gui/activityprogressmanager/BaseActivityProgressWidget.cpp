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
    if(m_activities_to_widgets_map.contains(job))
    {
        return;
    }

    // Create a new widget for this job.
    auto pw = new ActivityProgressStatusBarWidget(job, this, m_parent);
    m_activities_to_widgets_map.insert(job, pw);

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
        delete m_activities_to_widgets_map[job];
    }
    m_activities_to_widgets_map.remove(job);
}

QWidget *BaseActivityProgressWidget::widget(KJob *job)
{
    return m_activities_to_widgets_map.value(job, nullptr);
}

#define M_WIDGET_OR_RETURN(the_job) auto widget = qobject_cast<ActivityProgressStatusBarWidget*>(m_activities_to_widgets_map.value(job, nullptr)); \
    if(!widget) { return; };


void BaseActivityProgressWidget::description(KJob *job, const QString &title, const QPair<QString, QString> &field1, const QPair<QString, QString> &field2)
{
    M_WIDGET_OR_RETURN(job);

    widget->description(title, field1, field2);
}
