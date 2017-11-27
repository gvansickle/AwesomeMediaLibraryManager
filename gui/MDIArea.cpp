/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "MDIArea.h"

MDIArea::MDIArea(QWidget *parent) : QMdiArea(parent)
{
    // Set the tabbed view look.
    setViewMode(QMdiArea::TabbedView);

    // From the Qt5 docs:
    // "This property holds whether or not the tab bar is rendered in a mode suitable for the main window.
    // This property is used as a hint for styles to draw the tabs in a different way then they would normally look
    // in a tab widget. On macOS this will look similar to the tabs in Safari or Sierra's Terminal.app."
    setDocumentMode(true);

    // Tabs can be closed, and add a close button to the tabs.
    setTabsClosable(true);

    // Tabs can be moved.
    setTabsMovable(true);
}

MDIArea::~MDIArea()
{

}
