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

#ifndef LIBRARYCONTEXTMENU_H
#define LIBRARYCONTEXTMENU_H

#include <QObject>
#include <QMenu>
#include <QAction>
#include <QPersistentModelIndex>

class QString;


class LibraryContextMenu : public QMenu
{
	Q_OBJECT
	
	using BASE_CLASS = QMenu;

public:
	/**
	 * Constructor for blank area context menu.
	 */
	explicit LibraryContextMenu(const QString &title, QWidget *parent = Q_NULLPTR);

	explicit LibraryContextMenu(const QString &title, QList<QPersistentModelIndex> row_indexes, QWidget *parent = Q_NULLPTR);

private:
	Q_DISABLE_COPY(LibraryContextMenu)

	QAction* m_act_append_to_playlist;
	QAction* m_act_replace_playlist;
	QAction* m_act_search_wikipedia;
};

#endif /* LIBRARYCONTEXTMENU_H */

