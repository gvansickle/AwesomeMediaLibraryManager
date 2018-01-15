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
#include "gui/helpers/Tips.h"
#include "utils/ActionHelpers.h"
#include "ActionBundle.h"

#include "gui/MainWindow.h"

#include <logic/ModelUserRoles.h>

/**
 * Blank-area context menu.
 */
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
	auto act_search_wikipedia = make_action(Theme::iconFromTheme("edit-web-search"), tr("Search Wikipedia for..."), this);
	act_search_wikipedia->setDisabled(true); /// @todo
	addAction(act_search_wikipedia);
}

/**
 * Context menu for a Library entry.
 */
LibraryContextMenu::LibraryContextMenu(const QString& title, QPersistentModelIndex pmi, QWidget* parent)
	: LibraryContextMenu(title, parent)
{
	if(pmi.isValid())
	{
		qDebug() << "pmi valid:" << pmi;

		auto row_index = pmi.sibling(pmi.row(), 0);
		if(row_index.isValid())
		{
			qDebug() << "row_index valid:" << row_index;

			// Got a valid index, add the track-specific entries.
			auto model = row_index.model();
			auto item_ptr = model->data(row_index, ModelUserRoles::PointerToItemRole).value<std::shared_ptr<LibraryEntry>>();

//			auto track_name = item_ptr->get
		}
	}
}

