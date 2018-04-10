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

#include "SettingsDialog.h"

// Pages
#include "SettingsPageGeneral.h"
#include "SettingsPageCollection.h"
#include "SettingsPageAppearance.h"
#include "SettingsPageLibrary.h"

#include <QApplication>
#include <QLayout>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QMessageBox>
#include <QDataWidgetMapper>
#include <QDebug>
#include <QStandardItem>

#include <utils/Theme.h>

#include "../MainWindow.h"

#include <AMLMSettings.h>

SettingsDialog::SettingsDialog(QWidget *parent, const char* name, KConfigSkeleton *config)
	: KConfigDialog( parent, name, config )
{
	// Create and add the pages.
	setFaceType(KPageDialog::List);
    addPage(new SettingsPageGeneral(this), tr("General"), "preferences-desktop-sound");
    addPage(new SettingsPageCollection(this), tr("Collection"), "applications-multimedia");
	addPage(new SettingsPageAppearance(this), tr("Appearance"), "preferences-desktop-color");
	addPage(new SettingsPageLibrary(this), tr("Music Library") );
	/// ...

	connect(this, &KConfigDialog::settingsChanged, this, &SettingsDialog::onSettingsChanged);

	show();
}

SettingsDialog::~SettingsDialog()
{

}

void SettingsDialog::onSettingsChanged()
{
	setFaceType(AMLMSettings::settingsDialogFace());
}








