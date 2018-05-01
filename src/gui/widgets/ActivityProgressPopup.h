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

#ifndef SRC_GUI_WIDGETS_ACTIVITYPROGRESSPOPUP_H_
#define SRC_GUI_WIDGETS_ACTIVITYPROGRESSPOPUP_H_

/// Qt5
class QWidget;
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
