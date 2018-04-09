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

#ifndef AWESOMEMEDIALIBRARYMANAGER_SETTINGSDIALOGPAGEBASE_H
#define AWESOMEMEDIALIBRARYMANAGER_SETTINGSDIALOGPAGEBASE_H

#include <QObject>
#include <QWidget>
#include <QString>

#include "SettingsDialogSideWidget.h"

class SettingsDialogBase;

class QLabel;

class SettingsDialogPageBase : public QWidget
{
    Q_OBJECT

public:
	// Create the page, and hook up the page's widgets to @a settings_dialog_base's settings model.
	SettingsDialogPageBase(SettingsDialogBase *settings_dialog_base, QWidget *parent = nullptr);

	/// @todo Private/Friend this to SettingsDialogBase?
	virtual void addContentsEntry(SettingsDialogSideWidget* contents_widget) = 0;

	virtual void onApply() = 0;

protected:

    /**
     * This is me copying the functionality of QWizardPage's registerField() members.
     * I am sure I'll find out that there's a simple way to use QWizard/QWizardPage for a
     * settings dialog that I couldn't figure out a few seconds after I have this mostly implemented.
     */
//    void registerField(const QString &name, QWidget *widget, const char *property = Q_NULLPTR, const char *changedSignal = Q_NULLPTR);

    SettingsDialogBase *m_settings_dialog_base = nullptr;
};


#endif //AWESOMEMEDIALIBRARYMANAGER_SETTINGSDIALOGPAGEBASE_H
