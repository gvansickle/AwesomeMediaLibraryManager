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

#ifndef GUI_MENUS_PLAYLISTCONTEXTMENU_H_
#define GUI_MENUS_PLAYLISTCONTEXTMENU_H_

#include <QMenu>

class QString;
class QWidget;

/*
 *
 */
class PlaylistContextMenu: public QMenu
{
	Q_OBJECT

public:
	explicit PlaylistContextMenu(const QString &title, QWidget *parent = Q_NULLPTR);

private:
	Q_DISABLE_COPY(PlaylistContextMenu)
};

#endif /* GUI_MENUS_PLAYLISTCONTEXTMENU_H_ */
