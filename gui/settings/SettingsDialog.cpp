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

#include <QApplication>


SettingsDialog::SettingsDialog(QWidget *parent, const Qt::WindowFlags &flags)
	: QWizard(parent, flags)
{
	// Make this Wizard look like a Settings Dialog.

	/// @todo Experiments.  Where do these strings, set on the Dialog itself, show up, if anywhere?
	setStatusTip("SettingsDialog StatusTip");
	setToolTip("SettingsDialog toolTip");
	setWhatsThis("SettingsDialog what'sThis");

	// Use the ModernStyle, to get rid of the big header graphic of AeroStyle, which just wastes space.
	setWizardStyle(QWizard::ModernStyle);
	setOptions(QWizard::HaveFinishButtonOnEarlyPages | QWizard::HaveHelpButton);

	// Turn the "Finish" button into an "Ok" button.
	setButtonText(QWizard::FinishButton, tr("OK"));

	// Explicitly set the button layout.
	// Note that the 'Commit" button is the Next button on a "Commit" page.  We'll use it here as an "Apply" button,
	// and make every page a "Commit" page.
	QList<QWizard::WizardButton> button_layout;
	button_layout << QWizard::HelpButton << QWizard::Stretch << QWizard::CommitButton << QWizard::CancelButton << QWizard::FinishButton;

	setButtonLayout(button_layout);

	setWindowTitle(tr("Settings"));

}
