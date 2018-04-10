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

#ifndef DROPMENU_H
#define DROPMENU_H

#include <QObject>
#include <QMenu>

class QString;
class QAction;

/**
 * Create a popup menu for a right-drop to allow user to select copy or move.
 */
class DropMenu : public QMenu
{
    Q_OBJECT

public:
    /**
     * @todo write docs
     */
	explicit DropMenu(const QString &title = tr("Copy or Move?"), QWidget *parent = Q_NULLPTR);

	Qt::DropAction whichAction(QPoint p);

private:
	Q_DISABLE_COPY(DropMenu)

    QAction *m_act_cancel;
    QAction *m_act_move;
    QAction *m_act_copy;
};

#endif // DROPMENU_H
