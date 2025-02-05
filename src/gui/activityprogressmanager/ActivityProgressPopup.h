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

#ifndef SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSPOPUP_H_
#define SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSPOPUP_H_

/// Qt
// class QWidget;
// #include <QWidgetAction>
#include <QWidget>

/**
 * From https://community.kde.org/Frameworks/Porting_Notes#Application:
 * "KPassivePopup: standardView() now returns a QWidget* instead of a KVBox*. The QWidget has a QVBoxLayout,
 * i.e., additional widgets can be appended by calling widget->layout()->addWidget()."
 */

/*
 *
 */
class ActivityProgressPopup : public QWidget
{
public:
	explicit ActivityProgressPopup(QWidget* parent = nullptr);
};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_ACTIVITYPROGRESSPOPUP_H_ */
