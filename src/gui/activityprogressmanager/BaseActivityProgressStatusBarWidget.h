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

#ifndef SRC_GUI_ACTIVITYPROGRESSMANAGER_BASEACTIVITYPROGRESSSTATUSBARWIDGET_H_
#define SRC_GUI_ACTIVITYPROGRESSMANAGER_BASEACTIVITYPROGRESSSTATUSBARWIDGET_H_

/// Qt5
#include <QObject>
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

    /// Private constructor to get us a fully-constructed vtable so we can
    /// call virtual functions in the non-default constructor.
    BaseActivityProgressStatusBarWidget(QWidget *parent);

public:
    explicit BaseActivityProgressStatusBarWidget(KJob *job, QWidget *parent);
    ~BaseActivityProgressStatusBarWidget() override;

    virtual void setDescription(const QString& title,
                        const QPair<QString, QString> &field1,
                        const QPair<QString, QString> &field2);
    virtual void setInfoMessage(const QString &text);
    virtual void setWarning(const QString &text);

public /*Q_SLOTS*/:
    virtual void setRange(int min, int max);
    virtual void setValue(int val);

protected:

    /// Create the widget.
    /// Called by the public constructor.
    virtual void init(KJob *job, QWidget *parent);

private:
    Q_DISABLE_COPY(BaseActivityProgressStatusBarWidget)

    QLabel* m_current_activity_label {nullptr};
    QLabel* m_text_status_label {nullptr};
    QProgressBar* m_progress_bar {nullptr};
    // Cancel Operation button.
    QToolButton *m_cancel_button {nullptr};
    // Pause/Resume button.
    QToolButton *m_pause_resume_button {nullptr};
};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_BASEACTIVITYPROGRESSSTATUSBARWIDGET_H_ */
