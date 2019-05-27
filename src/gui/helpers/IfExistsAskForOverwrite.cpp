/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/**
 * @file IfExistsAskForOverwrite.cpp
 */

#include "IfExistsAskForOverwrite.h"

// Qt5
#include <QFile>
#include <QMessageBox>

// Ours
#include <AMLMApp.h>
#include <gui/MainWindow.h>


IfExistsAskForOverwrite::IfExistsAskForOverwrite()
{
}

IfExistsAskForOverwrite::~IfExistsAskForOverwrite()
{
}

// Static
bool IfExistsAskForOverwrite::IfExistsAskForDelete(const QUrl &filename)
{
    QFile the_file(filename.toLocalFile());
    if(the_file.exists())
    {
		QMessageBox::StandardButton retval = QMessageBox::warning(AMLMApp::IMainWindow(), QObject::tr("File exists"),
        		QObject::tr("The file '%1' already exists.\n"
                            "Do you want to delete it?").arg(filename.toLocalFile()),
                                          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if(retval == QMessageBox::Yes)
        {
            return the_file.remove();
        }
        else
        {
            return false;
        }
    }
    // else file doesn't exist.
	return true;
}

