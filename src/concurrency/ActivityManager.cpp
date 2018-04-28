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

#include "ActivityManager.h"

#include <concurrency/AMLMJob.h>
#include <ThreadWeaver/QObjectDecorator>

#include <utils/DebugHelpers.h>

/// The singleton.
ActivityManager ActivityManager::m_the_activity_manager;


ActivityManager::ActivityManager()
{
    qDb() << "SINGLETON CREATED";
}

ActivityManager::~ActivityManager()
{

}

void ActivityManager::addActivity(ThreadWeaver::QObjectDecorator* activity)
{
    qDb() << "ACTIVITY ADDED:" << activity;

    m_tw_activities.push_back(activity);

    connect(activity, &ThreadWeaver::QObjectDecorator::done,
            this, qOverload<ThreadWeaver::JobPointer>(&ActivityManager::onActivityFinished));
}

void ActivityManager::addActivity(AMLMJob *activity)
{
    qDb() << "ACTIVITY ADDED, AMLMJob:" << activity;

    m_amlm_activities.push_back(activity);

    connect(activity, &AMLMJob::finished, [this](KJob* job){
        auto as_amlmjob = qobject_cast<AMLMJob*>(job);
        Q_CHECK_PTR(as_amlmjob);
        onActivityFinished(as_amlmjob);
    });
}

void ActivityManager::onActivityFinished(ThreadWeaver::JobPointer activity)
{
    // Slot that indicates an activity is complete and should be removed from the list.
    qDb() << "ACTIVITY FINISHED:" << activity;
}

void ActivityManager::onActivityFinished(AMLMJob* activity)
{
    qDb() << "ACTIVITYFINISHED/AMLMJob" << activity;
}

