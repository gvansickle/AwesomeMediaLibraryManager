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

#include "ActionBundle.h"

#include <QMenu>
#include <QToolBar>

ActionBundle::ActionBundle(QObject *parent) : QActionGroup (parent)
{
	// Default to non-exclusive.
	setExclusive(false);
}

QAction* ActionBundle::addSection(const QString& text)
{
	auto sec = new QAction(text, this);
	sec->setSeparator(true);
	return sec;
}

QAction* ActionBundle::addSection(const QIcon& icon, const QString& text)
{
	auto sec = new QAction(icon, text, this);
	sec->setSeparator(true);
	return sec;
}

void ActionBundle::appendToMenu(QMenu* menu, bool elide_separators)
{
	for(const auto& action : actions())
	{
		if(!elide_separators || action->isSeparator() == false)
		{
			menu->addAction(action);
		}
	}
}

void ActionBundle::prependToMenu(QMenu* menu, bool elide_separators)
{
	// We insert our contained actions in reverse order to the top of @a menu.
    auto menu_actions = menu->actions();
    auto first_action_in_menu = menu_actions[0];
    for(auto i = 0; i < menu_actions.size(); ++i)
	{
        auto action = menu_actions[menu_actions.size()-i-1];
		if(!elide_separators || action->isSeparator() == false)
		{
			menu->insertAction(first_action_in_menu, action);
			first_action_in_menu = action;
		}
	}
}

void ActionBundle::appendToToolBar(QToolBar* toolbar, bool elide_separators)
{
	for(const auto& action : actions())
	{
		if(!elide_separators || action->isSeparator() == false)
		{
			toolbar->addAction(action);
		}
	}
}

