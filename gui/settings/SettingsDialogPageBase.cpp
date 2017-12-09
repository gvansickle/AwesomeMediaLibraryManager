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

#include "SettingsDialogPageBase.h"
SettingsDialogPageBase::SettingsDialogPageBase(QWidget *parent)
	: QWizardPage(parent)
{
	// Every page is a "Commit" page.  As such, when you hit the "Apply" button (which in the normal QWizard setup is
	// the "Next" button with the text "Commit" on it), the changes are applied and you can't undo them with Cancel.
	setCommitPage(true);
}
