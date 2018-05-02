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

#ifndef SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSSTATUSBARWIDGET_H_
#define SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSSTATUSBARWIDGET_H_

class QWidget;
#include <QTime>

class KJob;
#include <KAbstractWidgetJobTracker>

#include "BaseActivityProgressWidget.h"


/*
 *
 */
class ActivityProgressStatusBarWidget: public KAbstractWidgetJobTracker
{
	Q_OBJECT

    using BASE_CLASS = KAbstractWidgetJobTracker;

public:
    ActivityProgressStatusBarWidget(KJob *job, BaseActivityProgressWidget* object, QWidget *parent);
	virtual ~ActivityProgressStatusBarWidget();

    void init(KJob *job, QWidget *parent);

    QWidget *widget(KJob *job) override;


    BaseActivityProgressWidget *const q;
    KJob *const m_job;
    QWidget* m_widget;
    bool m_being_deleted;

protected Q_SLOTS:

    void totalAmount(KJob *job, KJob::Unit unit, qulonglong amount) override;
    void processedAmount(KJob *job, KJob::Unit unit, qulonglong amount) override;
    void percent(KJob *job, unsigned long percent) override;
//    void speed(KJob *job, unsigned long value) override;

private:
    qulonglong m_processedSize;
    bool m_totalSizeKnown;
    qulonglong m_totalSize;

    QTime startTime;

};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSSTATUSBARWIDGET_H_ */
