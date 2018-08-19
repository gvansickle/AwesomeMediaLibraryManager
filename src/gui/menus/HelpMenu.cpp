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


#include <gui/actions/ActionHelpers.h>
#include "HelpMenu.h"

#include <KHelpMenu>
#include <QMenu>
#include <QAction>
#include <QApplication>

#include <utils/ConnectHelpers.h>

HelpMenu::HelpMenu(QWidget* parent, const KAboutData& aboutData, bool showWhatsThis) : KHelpMenu(parent, aboutData, showWhatsThis)
{
	setObjectName("main_help_menu");

	// Hide the switch languages entry.
	auto lang_act = action(KHelpMenu::menuSwitchLanguage);
	if(lang_act)
	{
		lang_act->setVisible(false);
	}

	// Grey out the "Handbook" section until we actually have help.
	auto help_act = action(KHelpMenu::menuHelpContents);
	if(help_act)
	{
		help_act->setDisabled(true);
	}

	// Add the "About QT" dialog between "About app" and "About KDE".
	QAction* act_about_kde = action(KHelpMenu::menuAboutKDE);
	QAction* act_aboutQt = make_action(QIcon::fromTheme("help-about-qt"), tr("About &Qt"), parent,
	                           QKeySequence(),
	                           "Show the Qt library's About box");
	if(act_about_kde && act_aboutQt)
	{
		connect_trig(act_aboutQt, qApp, &QApplication::aboutQt);
		menu()->insertAction(act_about_kde, act_aboutQt);
	}

}

HelpMenu::~HelpMenu()
{

}
