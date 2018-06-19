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

#include <KCoreConfigSkeleton>

#include <utils/DebugHelpers.h>
#include "../MainWindow.h"

#include <AMLMSettings.h>
#include <gui/Theme.h>

#include <KComboBox>
#include <QComboBox>
#include <QRegularExpression>

SettingsDialog::SettingsDialog(QWidget *parent, const char* name, KConfigSkeleton *config)
	: KConfigDialog( parent, name, config )
{
	// Create and add the pages.
	setFaceType(KPageDialog::List);
    addPage(new SettingsPageGeneral(this), tr("General"), "preferences-desktop-sound");
    addPage(new SettingsPageCollection(this), tr("Collection"), "applications-multimedia");
	addPage(new SettingsPageAppearance(this), tr("Appearance"), "preferences-desktop-color");
//	addPage(new SettingsPageLibrary(this), tr("Music Library") );
	/// ...

	connect(this, &KConfigDialog::settingsChanged, this, &SettingsDialog::onSettingsChanged);

///// @todo experiment
//    QRegExp re("^kcfg_.*");
//    auto children = findChildren<QWidget*>(re, Qt::FindChildrenRecursively);
//    qDebug() << "CHILDREN:" << children;

////    auto fmcombo = findChild<QComboBox*>("kcfg_toolbarTextIconModeCombo");//"kcfg_fileDialogModeComboBox");
//    auto fmcombo = findChild<QComboBox*>("kcfg_fileDialogModeComboBox");
//    qDebug() << "FMCOMBO:" << fmcombo->count();

//    auto item = AMLMSettings::self()->fileDialogModeComboBoxItem();
//    auto ch = item->choices();
//    qDebug() << "Choices:" << ch.count() << "Label:" << item->label() << "Group:" << item->group();
//    for(auto i = 0; i< ch.count(); i++)
//    {
//        auto txt = ch[i].label;
//        qDebug() << "Choice:" << txt;
//        fmcombo->addItem(txt);
//    }

    parseWidgetsThatKDEForgotAbout();
}

SettingsDialog::~SettingsDialog()
{

}

void SettingsDialog::updateSettings()
{
    // Save our own window settings.
    /// @todo Do we actually need to do this with KF5?
    /// @todo Load this in the constructor.
    KConfigGroup settings_dlg_group(AMLMSettings::self()->config(), "AMLMSettingsDialog");
    KWindowConfig::saveWindowSize(windowHandle(), settings_dlg_group);

    BASE_CLASS::updateSettings();
}

void SettingsDialog::updateWidgets()
{
//    parseWidgetsThatKDEForgotAbout();
}

void SettingsDialog::updateWidgetsDefault()
{
//    parseWidgetsThatKDEForgotAbout();
}

void SettingsDialog::onSettingsChanged()
{
    setFaceType(AMLMSettings::settingsDialogFace());
}

void SettingsDialog::parseWidgetsThatKDEForgotAbout()
{
    // K/QComboBoxes
    // If there's a way to make combo boxes work with Enums without doing the setup by hand, I don't see it.
    QRegExp re("^kcfg_.*");
    auto children = findChildren<KComboBox*>(re, Qt::FindChildrenRecursively);
    if(children.size() == 0)
    {
        qDb() << "Found no KComboBoxes";
    }
    else
    {
        qDb() << "CHILDREN:" << children;

        for(auto combo : children)
        {
            // Get the name we need to look up the corresponding KConfigSkeletonItem.
            QString kskel_item_name = combo->objectName().mid(5);
            KConfigSkeletonItem* kskel_item = AMLMSettings::self()->findItem(kskel_item_name);
            AMLMSettings::ItemEnum* the_enum = dynamic_cast<AMLMSettings::ItemEnum*>(kskel_item);
            if(kskel_item == nullptr || the_enum == nullptr)
            {
                qWr() << "NO KCoreConfigSkeleton::ItemEnum FOR WIDGET:" << combo->objectName() << "kskel_name:" << kskel_item_name;
            }
            else
            {
                auto ch = the_enum->choices();
                qDebug() << "Choices:" << ch.count() << "Label:" << the_enum->label() << "Group:" << the_enum->group();
                for(auto i = 0; i< ch.count(); i++)
                {
                    auto txt = ch[i].label;
                    qDebug() << "Choice:" << txt;
                    combo->addItem(txt);
                }
            }
        }
    }

}

void SettingsDialog::setupWidget(KComboBox *box, KConfigSkeletonItem *item)
{
    if(box && item)
    {

    }
}








