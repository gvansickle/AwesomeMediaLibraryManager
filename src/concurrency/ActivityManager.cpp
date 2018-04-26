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

#include <src/concurrency/ActivityManager.h>

#include <ThreadWeaver/QObjectDecorator>

#include <utils/DebugHelpers.h>

///
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
            this, &ActivityManager::onActivityFinished);//this, &ActivityManager::onActivityFinished);
}

void ActivityManager::onActivityFinished(ThreadWeaver::JobPointer activity)
{
    // Slot that indicates an activity is complete and should be removed from the list.
    qDb() << "ACTIVITY FINISHED:" << activity;
}

