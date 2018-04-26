/*
 * ActivityManger.cpp
 *
 *  Created on: Apr 25, 2018
 *      Author: gary
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

