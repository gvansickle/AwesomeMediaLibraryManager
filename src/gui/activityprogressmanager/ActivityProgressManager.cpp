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

#include <gui/activityprogressmanager/ActivityProgressMultiTracker.h>
#include "ActivityProgressManager.h"

/// QT5

/// KF5
#include <KJob>

/// Ours

ActivityProgressManager::ActivityProgressManager(QObject *parent) : BASE_CLASS(parent)
{

}

ActivityProgressManager::~ActivityProgressManager()
{

}

void ActivityProgressManager::registerJob(KJob *job)
{
    // Hook the job's signals up to the slots we need to listen to.
    KJobTrackerInterface::registerJob(job);

    // Tell any derived classes about the new job.
    jobRegistered(job);
}

void ActivityProgressManager::unregisterJob(KJob *job)
{
    // Disconnect any signals from this job.
    KJobTrackerInterface::unregisterJob(job);

    // Tell any derived classes that the job is unregistered.
    jobUnregistered(job);

    // Remove any widgets we might have registered for this job.
//    m_activities_to_widgets_map.remove(job);
}

