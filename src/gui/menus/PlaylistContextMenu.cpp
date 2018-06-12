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

#include <gui/Theme.h>
#include "PlaylistContextMenu.h"

#include <QDebug>

#include "utils/ActionHelpers.h"
#include "gui/helpers/Tips.h"
#include "ActionBundle.h"

#include "gui/MainWindow.h"

PlaylistContextMenu::PlaylistContextMenu(const QString &title, QWidget *parent) : QMenu(title, parent)
{
	setTitle(title);

    auto mw = MainWindow::instance();

	// Add cut/copy/paste to the context menu.
	mw->m_ab_cut_copy_paste_actions->appendToMenu(this);
}

