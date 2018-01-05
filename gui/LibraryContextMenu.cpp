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

#include "LibraryContextMenu.h"

#include <QDebug>

#include "utils/Theme.h"
#include "utils/ActionHelpers.h"
#include "helpers/Tips.h"
#include "menus/ActionBundle.h"

#include "gui/MainWindow.h"

LibraryContextMenu::LibraryContextMenu(const QString &title, QWidget *parent) : QMenu(title, parent)
{
	setTitle(title);

	qDebug() << "Parent:" << parent;

	auto mw = MainWindow::getInstance();

	auto act_append_to_playlist = make_action(Theme::iconFromTheme("go-next"), tr("Append to Now Playing"), this);
	auto act_replace_playlist = make_action(Theme::iconFromTheme("go-next"), tr("Replace Now Playing with this"), this);

//	setTextandTips();
	
	addAction(act_append_to_playlist);
	addAction(act_replace_playlist);
	addSeparator();

	addAction("Song properties...");

	addSeparator();

	// Add cut/copy/paste to the context menu.
	mw->m_ab_cut_copy_paste_actions->appendToMenu(this);


	addSeparator();
	addAction("Search Wikipedia for...");
}

