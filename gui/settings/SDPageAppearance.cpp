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
#include <QString>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QFontDialog>
#include <QToolButton>

#include <utils/Theme.h>
#include <utils/DebugHelpers.h>


static void make_font_selector(QString label, const QFont& current_font, QFormLayout* layout)
{
    QHBoxLayout *rhs_layout = new QHBoxLayout;
    auto fontComboBox = new QFontComboBox();
    fontComboBox->setCurrentFont(current_font);

	auto fontSizeSpinbox = new QSpinBox;
    fontSizeSpinbox->setSuffix(" pt");
    fontSizeSpinbox->setRange(8,32);

    auto fontDialogButton = new QToolButton;
    fontDialogButton->setIcon(Theme::iconFromTheme("preferences-desktop-font"));
    QObject::connect(fontDialogButton, &QToolButton::clicked, [=](){
        auto font = QFontDialog::getFont(nullptr, fontComboBox->currentFont(), layout->parentWidget(), label);
        fontComboBox->setCurrentFont(font);
        fontSizeSpinbox->setValue(font.pointSize());
    });
M_WARNING("TODO Need to come up with this range better.")

	fontComboBox->setEditable(false);
    rhs_layout->addWidget(fontComboBox);
	rhs_layout->addWidget(fontSizeSpinbox);
    rhs_layout->addWidget(fontDialogButton);
    // Add the form entries.
    layout->addRow(label, rhs_layout);
}

SDPageAppearance::SDPageAppearance(QWidget *parent) : SettingsDialogPageBase(parent)
{
	// The font selection group box.
	QGroupBox *fontGroup = new QGroupBox(tr("Fonts"));
	auto fontFormLayout = new QFormLayout;
    make_font_selector(tr("Default Track font"), QFont(), fontFormLayout);
	make_font_selector(tr("Default Metadata Explorer font"), QFont(), fontFormLayout);
	fontGroup->setLayout(fontFormLayout);

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
	mainLayout->addWidget(fontGroup);
	mainLayout->addWidget(configGroup);
	mainLayout->addStretch(1);
	setLayout(mainLayout);

    //registerField("default_track_font", fontComboBox);
}

void SDPageAppearance::addContentsEntry(SettingsDialogSideWidget *contents_widget)
{
	contents_widget->addPageEntry("Appearance", Theme::iconFromTheme("preferences-desktop-color"),
	                                     "Appearance settings",
	                                     "View/Change appearance-related settings",
	                                     "This selection will allow you to view and/or change the appearance-related settings");
}
