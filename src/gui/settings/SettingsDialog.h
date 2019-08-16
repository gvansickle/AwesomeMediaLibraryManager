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

#ifndef AWESOMEMEDIALIBRARYMANAGER_SETTINGSDIALOG_H
#define AWESOMEMEDIALIBRARYMANAGER_SETTINGSDIALOG_H

#include <KConfigDialog>
#include <KConfigSkeleton>
#include <KWindowConfig>

class KComboBox;
class QComboBox;


/**
 * @todo How to use enums with KConfig, from /usr/share/config.kcfg/structviewpreferences.kcfg
 *  Okteta: https://github.com/KDE/okteta/blob/master/kasten/controllers/view/structures/settings/structureviewpreferences.kcfg
 *
 *<?xml version="1.0" encoding="UTF-8"?>
  <kcfg xmlns="http://www.kde.org/standards/kcfg/1.0" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xsi:schemaLocation="http://www.kde.org/standards/kcfg/1.0 http://www.kde.org/standards/kcfg/1.0/kcfg.xsd">
  <include>QSysInfo</include>
  <include>view/structures/datatypes/datainformationbase.h</include>
  <kcfgfile name="oktetastructuresrc" />
  ////
 * <group name="StructureSettings">
    <entry name="ByteOrder" type="Enum">
      <label context="@label:listbox">Byte order</label>
      <choices name="QSysInfo::Endian">
        <choice name="BigEndian"><label context="@item:inlistbox">Big endian</label></choice>
        <choice name="LittleEndian"><label context="@item:inlistbox">Little endian</label></choice>
      </choices>
      <default>QSysInfo::LittleEndian</default>
    </entry>
    <entry name="LoadedStructures" type="StringList">
      <default></default>
    </entry>
  </group>
 *
 * But apparently you still have to populate the combo box with items matching the label text (not choice name).
 */

class SettingsDialog : public KConfigDialog
{
    Q_OBJECT

    using BASE_CLASS = KConfigDialog;

public:
	SettingsDialog(QWidget *parent, const char* name, KConfigSkeleton *config);
    ~SettingsDialog() override;

// KConfigDialog signals:
	// void 	settingsChanged (const QString &dialogName)
	// void 	widgetModified ()

protected Q_SLOTS:

    /**
     * Update the settings from the dialog.
     * Example use: User clicks Ok or Apply button in a configure dialog.
     */
    void updateSettings() override;

    /**
     * Update the dialog based on the settings, before the dialog is shown.
     *
     * Example use: Initialisation of dialog.
     * Example use: User clicks Reset button in a configure dialog.
     */
    void updateWidgets() override;

    /**
     * Update the dialog based on the default settings.
     * Virtual function for custom additions.
     *
     * Example use: User clicks Defaults button in a configure dialog.
     */
    void updateWidgetsDefault() override;

	void onSettingsChanged();

protected:

	void parseWidgetsThatKDEForgotAbout();

	void setupWidget(KComboBox* box, KConfigSkeletonItem *item);

};


#endif //AWESOMEMEDIALIBRARYMANAGER_SETTINGSDIALOG_H

