/*
 * ActivityProgressPopup.h
 *
 *  Created on: Apr 30, 2018
 *      Author: gary
 */

#ifndef SRC_GUI_WIDGETS_ACTIVITYPROGRESSPOPUP_H_
#define SRC_GUI_WIDGETS_ACTIVITYPROGRESSPOPUP_H_

/// Qt5

#include <QWidgetAction>

/// KF5

class KMessageWidget;




class ActivityEntry : public QWidgetAction
{
public:
    ActivityEntry(QObject *parent = nullptr);
    ~ActivityEntry();

protected:

    QWidget* createWidget(QWidget *parent) override;
    void deleteWidget(QWidget* widget) override;

    KMessageWidget* m_message_widget;
};


/*
 *
 */
class ActivityProgressPopup : public QWidget
{
public:
    ActivityProgressPopup(QWidget* parent = nullptr);
};

#endif /* SRC_GUI_WIDGETS_ACTIVITYPROGRESSPOPUP_H_ */
