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

#include <utils/DebugHelpers.h>
#include "SettingsDialogPageBase.h"

#include "SettingsDialog.h"

SettingsDialogPageBase::SettingsDialogPageBase(SettingsDialog *parent)
	: QWidget(parent)
{

}

SettingsDialog *SettingsDialogPageBase::getSettingsDialog()
{
M_WARNING("TODO This parent-of-parent is adequate, but we should have a better way.")
    return dynamic_cast<SettingsDialog *>(this->parentWidget()->parentWidget());
}

void SettingsDialogPageBase::registerField(const QString &name, QWidget *widget, const char *property,
                                           const char *changedSignal)
{
    getSettingsDialog()->registerField(name, widget, property, changedSignal);
}



