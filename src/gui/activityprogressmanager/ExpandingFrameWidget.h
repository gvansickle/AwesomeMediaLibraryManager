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

#ifndef SRC_GUI_ACTIVITYPROGRESSMANAGER_EXPANDINGFRAMEWIDGET_H_
#define SRC_GUI_ACTIVITYPROGRESSMANAGER_EXPANDINGFRAMEWIDGET_H_

#include <QWidget>
#include <QSize>
//#include <QFrame>
#include <QDialog>
#include <QPointer>


/*
 *
 */
class ExpandingFrameWidget : public QWidget
{
    Q_OBJECT

    using BASE_CLASS = QWidget;

Q_SIGNALS:
    /// We don't get this signal with widgets, only QWindows.
    void visibilityChanged(bool);

public:
    explicit ExpandingFrameWidget(QWidget* parent = nullptr);
     ~ExpandingFrameWidget() override;

    void setMainProgressWidget(QWidget* status_bar_widget);

    void setVisible(bool visible) override;

    void addWidget(QWidget* new_widget);

    void removeWidget(QWidget* new_widget);

    QSize sizeHint() const override;

    void reposition();

protected:
    void resizeEvent(QResizeEvent* ev) override;
    bool eventFilter( QObject* o, QEvent* e) override;

private:

    QPointer<QWidget> m_cumulative_status_bar_main_widget;

};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_EXPANDINGFRAMEWIDGET_H_ */
