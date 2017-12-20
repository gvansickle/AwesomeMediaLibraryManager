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

#include <QObject>
#include <QDialog>
#include <QPointer>
#include <QDialogButtonBox>
#include <QStackedWidget>

#include "SettingsDialogSideWidget.h"
#include "SettingsDialogPageBase.h"
#include "RegisteredField.h"
#include "SettingsDialogBase.h"

class SettingsDialog : public SettingsDialogBase
{
    Q_OBJECT

public:
    SettingsDialog(QWidget *parent = nullptr, const Qt::WindowFlags &flags = 0);
};


#endif //AWESOMEMEDIALIBRARYMANAGER_SETTINGSDIALOG_H

