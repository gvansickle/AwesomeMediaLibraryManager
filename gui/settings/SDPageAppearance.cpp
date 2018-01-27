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


static QFontComboBox* make_font_selector(QString label, const QFont& current_font, QFormLayout* layout)
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
        auto current_combo_font = fontComboBox->currentFont();
        current_combo_font.setPointSize(fontSizeSpinbox->value());
        auto font = QFontDialog::getFont(nullptr, current_combo_font, layout->parentWidget(), label);
        fontComboBox->setCurrentFont(font);
        fontSizeSpinbox->setValue(font.pointSize());
    });

	fontComboBox->setEditable(false);
    rhs_layout->addWidget(fontComboBox);
	rhs_layout->addWidget(fontSizeSpinbox);
    rhs_layout->addWidget(fontDialogButton);
    // Add the form entries.
    layout->addRow(label, rhs_layout);

    return fontComboBox;
}

SDPageAppearance::SDPageAppearance(SettingsDialogBase *settings_dialog_parent, QWidget *parent)
        : SettingsDialogPageBase(settings_dialog_parent, parent)
{
	// The font selection group box.
	QGroupBox *fontGroup = new QGroupBox(tr("Fonts"));
	auto fontFormLayout = new QFormLayout;
    m_track_font_selector = make_font_selector(tr("Default Track font"), QFont(), fontFormLayout);
	make_font_selector(tr("Default Metadata Explorer font"), QFont(), fontFormLayout);
	fontGroup->setLayout(fontFormLayout);


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

	QGroupBox *configGroup = new QGroupBox(tr("Appearance"));
	QVBoxLayout *config_layout = new QVBoxLayout;
	config_layout->addLayout(dummy_layout);
	configGroup->setLayout(config_layout);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(fontGroup);
	mainLayout->addWidget(configGroup);
	mainLayout->addStretch(1);
	setLayout(mainLayout);

//    registerField("default_track_font", m_track_font_selector);
}

void SDPageAppearance::addContentsEntry(SettingsDialogSideWidget *contents_widget)
{
	contents_widget->addPageEntry("Appearance", Theme::iconFromTheme("preferences-desktop-color"),
	                                     "Appearance settings",
	                                     "View/Change appearance-related settings",
								  "This selection will allow you to view and/or change the appearance-related settings");
}

void SDPageAppearance::onApply()
{

}
