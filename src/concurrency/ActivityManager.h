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

#ifndef SRC_CONCURRENCY_ActivityManager_H_
#define SRC_CONCURRENCY_ActivityManager_H_

#include "AMLMJob.h"

#include <QObject>
#include <QVector>

namespace ThreadWeaver
{
	class QObjectDecorator;
}
#include <ThreadWeaver/JobPointer>

class AMLMJob;

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

private:
    /// Singleton, hide the constructor.
    explicit ActivityManager(QObject* parent = nullptr);

public:
    ~ActivityManager() override;

    /**
     * Important safety tip per the Qt5 docs (http://doc.qt.io/qt-5/threads-qobject.html):
     * "In general, creating QObjects before the QApplication is not supported and can lead to weird crashes on exit,
     *  depending on the platform. This means static instances of QObject are also not supported. A properly
     *  structured single or multi-threaded application should make the QApplication be the first created, and
     *  last destroyed QObject."
     */
    static ActivityManager* instance();

    /// Destruct the singleton.
    static void destroy();

    /**
     * Add a decorated ThreadWeaver Job/Queue/Weaver to the collection of activities.
     */
//    void addActivity(ThreadWeaver::QObjectDecorator *activity);

    void addActivity(AMLMJob* activity);


public Q_SLOTS:


protected:

protected Q_SLOTS:

    void onActivityFinished(ThreadWeaver::JobPointer activity);

    void onActivityFinished(AMLMJob* activity);

private:

    /// The singleton.
    /// This cannot be a static value member because of Qt5 (surprise).  This
    /// is a QObject and QObects need to be created after the QApplication is created.
    static ActivityManager* m_the_activity_manager;

    QVector<ThreadWeaver::QObjectDecorator *> m_tw_activities;

    QVector<AMLMJob*> m_amlm_activities;
};

#endif /* SRC_CONCURRENCY_ActivityManager_H_ */
