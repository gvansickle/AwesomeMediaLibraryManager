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

#include "AMLMApp.h"

/// Qt5
//#include <QNetworkAccessManager>

/// KF5
#include <KJob>
#include <ThreadWeaver/Queue>

/// Ours
#include <utils/TheSimplestThings.h>
#include <logic/SupportedMimeTypes.h>


AMLMApp::AMLMApp(int& argc, char** argv) : BASE_CLASS(argc, argv)
{
    qRegisterMetaType<KJob::Unit>();

    /// @todo EXPERIMENTAL
//    QNetworkAccessManager* nam = new QNetworkAccessManager(this);
//    qIn() << "QNetworkAccessManager Supported Schemes:" << nam->supportedSchemes();

    // Create the singletons we'll need for any app invocation.
    /* QObject hierarchy will self-destruct this = */ new SupportedMimeTypes(this);
}

AMLMApp::~AMLMApp()
{
    // Shut down ThreadWeaver.
    ThreadWeaver::Queue::instance()->requestAbort();
    ThreadWeaver::Queue::instance()->finish();
    ThreadWeaver::Queue::instance()->shutDown();
}
