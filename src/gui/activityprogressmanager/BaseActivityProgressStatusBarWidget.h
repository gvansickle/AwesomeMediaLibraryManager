/*
 * BaseActivityProgressStatusBarWidget.h
 *
 *  Created on: May 4, 2018
 *      Author: gary
 */

#ifndef SRC_GUI_ACTIVITYPROGRESSMANAGER_BASEACTIVITYPROGRESSSTATUSBARWIDGET_H_
#define SRC_GUI_ACTIVITYPROGRESSMANAGER_BASEACTIVITYPROGRESSSTATUSBARWIDGET_H_

/// Qt5
#include <QFrame>
class QLabel;
class QProgressBar;
class QToolButton;

/// KF5
class KJob;

/*
 *
 */
class BaseActivityProgressStatusBarWidget: public QFrame
{
    Q_OBJECT

    using BASE_CLASS = QFrame;

public:
    BaseActivityProgressStatusBarWidget(QWidget* parent);
	virtual ~BaseActivityProgressStatusBarWidget();

protected:

    /// Create the widget.
    /// Called by the constructor.
    void init(KJob *job, QWidget *parent);

private:

    QWidget* m_widget;
    QLabel* m_current_activity_label;
    QLabel* m_text_status_label;
    QProgressBar* m_progress_bar;
    // Cancel Operation button.
    QToolButton *m_cancel_button;
    // Pause/Resume button.
    QToolButton *m_pause_resume_button;
};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_BASEACTIVITYPROGRESSSTATUSBARWIDGET_H_ */
