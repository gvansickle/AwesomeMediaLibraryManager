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

#include <config.h>

#include "AMLMApp.h"

// Qt5
#include <QProcessEnvironment>

// KF5
//#include <KJob>

// Ours
#include <utils/TheSimplestThings.h>
#include <logic/SupportedMimeTypes.h>
#include <gui/Theme.h>


AMLMApp::AMLMApp(int& argc, char** argv) : BASE_CLASS(argc, argv)
{
    /// @todo EXPERIMENTAL
//    QNetworkAccessManager* nam = new QNetworkAccessManager(this);
//    qIn() << "QNetworkAccessManager Supported Schemes:" << nam->supportedSchemes();

    // Create the singletons we'll need for any app invocation.
    /* QObject hierarchy will self-destruct this = */ new SupportedMimeTypes(this);
}

AMLMApp::~AMLMApp()
{
    /// @todo Shut down whatever still needs shutting down.
}

void AMLMApp::KDEOrForceBreeze(KConfigGroup group)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains(QStringLiteral("XDG_CURRENT_DESKTOP")) && env.value(QStringLiteral("XDG_CURRENT_DESKTOP")).toLower() == QLatin1String("kde"))
    {
        qDb() << "KDE Desktop detected, using system icons";
    }
    else
    {
        // We are not on a KDE desktop, force breeze icon theme
        group.writeEntry("force_breeze", true);
        qDb() << "Non KDE Desktop detected, forcing Breeze icon theme";
    }
}
