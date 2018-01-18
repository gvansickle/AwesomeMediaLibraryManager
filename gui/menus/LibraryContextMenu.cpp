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

	m_act_append_to_playlist = make_action(Theme::iconFromTheme("go-next"), tr("Append to Now Playing"), this);
	m_act_replace_playlist = make_action(Theme::iconFromTheme("go-next"), tr("Replace Now Playing with this"), this);

//	setTextandTips();
	
	addAction(m_act_append_to_playlist);
	addAction(m_act_replace_playlist);
	addSeparator();

	addAction("Song properties...");

	addSeparator();

	// Add cut/copy/paste to the context menu.
	mw->m_ab_cut_copy_paste_actions->appendToMenu(this);
}

/**
 * Context menu for a Library entry.
 */
LibraryContextMenu::LibraryContextMenu(const QString& title, const QPersistentModelIndexVec& selected_rows, QWidget* parent)
	: LibraryContextMenu(title, parent)
{
	// Convert to sourceModel coords.
	auto row_indexes = mapQPersistentModelIndexesToSource(selected_rows);
	bool is_multirow = false;
	
	if(row_indexes.size() > 0)
	{
		qDebug() << "row_indexes size():" << row_indexes.size();

		if(row_indexes.size() > 1)
		{
			// Multi-row selection.
			is_multirow = true;
		}
		
		auto model = qobject_cast<const LibraryModel*>(row_indexes[0].model());
		Q_ASSERT(model != nullptr);
		auto name_col = model->getColFromSection(SectionID::Title);

		auto row_index = row_indexes[0].sibling(row_indexes[0].row(), name_col);
		if(row_index.isValid())
		{
			qDebug() << "row_index valid:" << row_index;

			// Got a valid index, add the track-specific entries.

			auto model = row_index.model();

			auto track_name = model->data(row_index, Qt::DisplayRole);
			qDebug() << "track_name:" << track_name;

			QString send_to_now_playing_text;
			if(is_multirow)
			{
				send_to_now_playing_text = tr("Send selected items to 'Now Playing'");
			}
			else
			{
				send_to_now_playing_text = tr("Send '%1' to 'Now Playing'").arg(track_name.toString());
			}

			addAction(send_to_now_playing_text);
		}

		addSeparator();
		m_act_search_wikipedia = make_action(Theme::iconFromTheme("edit-web-search"), tr("Search Wikipedia for..."), this);
		m_act_search_wikipedia->setDisabled(true); /// @todo
		addAction(m_act_search_wikipedia);
	}
}

