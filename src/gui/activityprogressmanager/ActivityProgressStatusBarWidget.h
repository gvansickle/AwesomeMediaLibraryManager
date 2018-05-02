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


#include <QWidget>

#include "BaseActivityProgressWidget.h"


/*
 *
 */
class ActivityProgressStatusBarWidget: public QWidget
{
	Q_OBJECT

public:
    ActivityProgressStatusBarWidget(KJob *job, BaseActivityProgressWidget* object, QWidget *parent);
	virtual ~ActivityProgressStatusBarWidget();

    void init(KJob *job, QWidget *parent);

    BaseActivityProgressWidget *const q;
    KJob *const m_job;
    QWidget* m_widget;
    bool m_being_deleted;

public Q_SLOTS:
    virtual void description(const QString &title,
                             const QPair<QString, QString> &field1,
                             const QPair<QString, QString> &field2);
    virtual void totalAmount(KJob::Unit unit, qulonglong amount);
    virtual void percent(unsigned long percent);
    virtual void speed(unsigned long value);
    virtual void slotClean();

private Q_SLOTS:
    void killJob();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSSTATUSBARWIDGET_H_ */
