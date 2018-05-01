/*
 * ActivityProgressManager.h
 *
 *  Created on: May 1, 2018
 *      Author: gary
 */

#ifndef SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSMANAGER_H_
#define SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSMANAGER_H_

#include <KJobTrackerInterface>

/*
 *
 */
class ActivityProgressManager: public KJobTrackerInterface
{
public:
	ActivityProgressManager();
};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSMANAGER_H_ */
