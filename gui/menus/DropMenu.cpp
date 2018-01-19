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

#include <gui/menus/DropMenu.h>
#include "utils/Theme.h"

DropMenu::DropMenu(const QString &title, QWidget *parent) : QMenu(title, parent)
{
	m_act_copy = addAction(Theme::iconFromTheme("edit-copy"), tr("Copy"));
	m_act_copy->setData(Qt::CopyAction);
	m_act_move = addAction(Theme::iconFromTheme("go-jump"), tr("Move"));
	m_act_move->setData(Qt::MoveAction);
	addSeparator();
	m_act_cancel = addAction(Theme::iconFromTheme("window-close"), tr("Cancel"));
	m_act_cancel->setData(Qt::IgnoreAction);
	// Set the safest choice as the default.
	setDefaultAction(m_act_cancel);
}

Qt::DropAction DropMenu::whichAction(QPoint p)
{
	QAction *selected_action = exec(p);

	if(selected_action == 0)
	{
		// User hit escape.
		return Qt::IgnoreAction;
	}

	return selected_action->data().value<Qt::DropAction>();
}


