/*
 * ActivityManager.h
 *
 *  Created on: Apr 25, 2018
 *      Author: gary
 */

#ifndef SRC_CONCURRENCY_ActivityManager_H_
#define SRC_CONCURRENCY_ActivityManager_H_

#include <QObject>

namespace ThreadWeaver
{
	class QObjectDecorator;
}

struct ActivityStatus
{
    QString m_desc;

    int m_min;
    int m_val;
    int m_max;
};

/**
 *
 */
class ActivityManager: public QObject
{
	Q_OBJECT

Q_SIGNALS:
    void totalStatusChanged(ActivityStatus total_status);


public:
    ActivityManager();
    ~ActivityManager() override;

    /**
     * Add a decorated ThreadWeaver Job/Queue/Weaver to the collection of activities.
     */
    void addActivity(ThreadWeaver::QObjectDecorator *activity);

public Q_SLOTS:


protected:

protected Q_SLOTS:

    void onActivityFinished(ThreadWeaver::QObjectDecorator *activity);

private:

    QVector<ThreadWeaver::QObjectDecorator*> m_tw_activities;
};

#endif /* SRC_CONCURRENCY_ActivityManager_H_ */
