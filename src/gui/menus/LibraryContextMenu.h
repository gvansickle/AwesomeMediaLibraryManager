/*
 * Copyright 2017, 2018
 * Gary R. Van Sickle (grvs@users.sourceforge.net).
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
#include <QItemSelection>

#include "utils/DebugHelpers.h"
#include "logic/proxymodels/ModelHelpers.h"

class QString;
class ActionBundle;


class LibraryContextMenu : public QMenu
{
	Q_OBJECT

	using BASE_CLASS = QMenu;

public:
	/**
	 * Constructor for blank area context menu.
	 */
	explicit LibraryContextMenu(const QString &title, QWidget *parent = Q_NULLPTR);

	/**
	 * Constructor for a context menu for a selection of one or more items.
	 * @a selection is in View-model index space, so depending on the use, it may need to be
	 * translated to source-model space.
	 */
	explicit LibraryContextMenu(const QString &title, const QPersistentModelIndexVec& selected_rows, QWidget *parent = Q_NULLPTR);

protected:

	/**
	 * Return a QString
	 */
	QStringList getSongsAsTooltips(const QPersistentModelIndexVec& row_indexes);

private:
	Q_DISABLE_COPY(LibraryContextMenu)

/// @todo M_WARNING("TODO: These shouldn't be public.  Not sure what would be a better way to do this atm.");
public:
	ActionBundle* m_ab_to_now_playing;
	QAction* m_act_append_to_now_playing;
	QAction* m_act_replace_playlist;
	QAction* m_act_search_wikipedia;
};

#endif /* LIBRARYCONTEXTMENU_H */

