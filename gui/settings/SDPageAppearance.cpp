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

#include "SDPageAppearance.h"
#include <QApplication>
#include <QLabel>
#include <QHBoxLayout>
#include <utils/Theme.h>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QComboBox>

SDPageAppearance::SDPageAppearance(QWidget *parent) : SettingsDialogPageBase(parent)
{
	QGroupBox *configGroup = new QGroupBox(tr("Appearance"));

	QLabel *dummy_select_label = new QLabel(tr("Select one:"));
	QComboBox *dummy_combo = new QComboBox;
	dummy_combo->addItem(tr("Dummy Entry 1"));
	dummy_combo->addItem(tr("Dummy Entry 2"));
	dummy_combo->addItem(tr("Dummy Entry 3"));
	dummy_combo->addItem(tr("Dummy Entry 4"));
	dummy_combo->addItem(tr("Dummy Entry 5"));

	QHBoxLayout *dummy_layout = new QHBoxLayout;
	dummy_layout->addWidget(dummy_select_label);
	dummy_layout->addWidget(dummy_combo);

	QVBoxLayout *config_layout = new QVBoxLayout;
	config_layout->addLayout(dummy_layout);
	configGroup->setLayout(config_layout);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(configGroup);
	mainLayout->addStretch(1);
	setLayout(mainLayout);
}

void SDPageAppearance::addContentsEntry(SettingsDialogSideWidget *contents_widget)
{
	contents_widget->addPageEntry("Appearance", Theme::iconFromTheme("preferences-desktop-color"),
	                                     "Appearance settings",
	                                     "View/Change appearance-related settings",
	                                     "This selection will allow you to view and/or change the appearance-related settings");
}
