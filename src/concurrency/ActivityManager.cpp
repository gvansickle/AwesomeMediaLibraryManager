/*
 * ActivityManger.cpp
 *
 *  Created on: Apr 25, 2018
 *      Author: gary
 */

#include <src/concurrency/ActivityManager.h>

ActivityManager::ActivityManager()
{

}

ActivityManager::~ActivityManager()
{

}

void ActivityManager::addActivity(ThreadWeaver::QObjectDecorator *activity)
{
    qDb() << "ACTIVITY ADDED:" << activity;

    m_tw_activities.push_back(activity);
}

void ActivityManager::onActivityFinished(ThreadWeaver::QObjectDecorator *activity)
{
    // Slot that indicates an activity is complete and should be removed from the list.
    qDb() << "ACTIVITY FINISHED:" << activity;
}

