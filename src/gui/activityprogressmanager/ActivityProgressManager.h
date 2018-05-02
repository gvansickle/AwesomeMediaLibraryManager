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

#ifndef SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSMANAGER_H_
#define SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSMANAGER_H_

/// Qt5
#include <QMap>

/// KF5
#include <KJobTrackerInterface>
class KJob;

/// Ours
class BaseActivityProgressWidget;

/**
 *
 */
class ActivityProgressManager: public KJobTrackerInterface
{
    Q_OBJECT

    using BASE_CLASS = KJobTrackerInterface;

public:
    explicit ActivityProgressManager(QObject* parent = nullptr);
    ~ActivityProgressManager() override;

    /**
     * @note KJobTrackerInterface has implementations of registerJob()/unregisterJob(),
     *      but not much else.  In particular, none of the slots these connect to
     *      except for finished()->unregisterJob() do anything.
     *      unregisterJob() does this: job->disconnect(this);
     */

    void registerJob(KJob* job) override;
    void unregisterJob(KJob* job) override;

    virtual void jobRegistered(KJob* job) {}
    virtual void jobUnregistered(KJob* job) {}


protected:

    using ActiveActivitiesMap = QMap<KJob*, BaseActivityProgressWidget*>;

    ActiveActivitiesMap m_activities_to_widgets_map;

};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSMANAGER_H_ */
