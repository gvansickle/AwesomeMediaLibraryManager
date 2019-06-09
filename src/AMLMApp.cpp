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

// Std C++
#include <memory>

// Qt5
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QUrl>

// KF5

// Ours
#include <utils/TheSimplestThings.h>
#include <logic/SupportedMimeTypes.h>
#include <gui/Theme.h>
//#include <logic/dbmodels/CollectionDatabaseModel.h>
#include <logic/PerfectDeleter.h>
#include <utils/RegisterQtMetatypes.h>
#include <gui/MainWindow.h>



// Pointer to the AMLMApp singleton.
AMLMApp *AMLMApp::m_the_instance = nullptr;

AMLMApp::AMLMApp(int& argc, char** argv) : BASE_CLASS(argc, argv)
{
    Q_ASSERT(m_the_instance == nullptr);

    m_the_instance = this;

    setObjectName("TheAMLMApp");
}

AMLMApp::~AMLMApp()
{
    /// @todo Shut down whatever still needs shutting down.

	// No more singleton.
	m_the_instance = nullptr;

	qDb() << "AMLMApp SINGLETON DESTROYED";
}

void AMLMApp::Init(bool gtest_only)
{
	// Get the PerfectDeleter instance up as early as possible.
	PerfectDeleter::instance(this);

	// Register our types with Qt.
	RegisterQtMetatypes();

	/// @todo This is ugly, refactor this.
	if(gtest_only)
	{
		return;
	}

	/// @todo EXPERIMENTAL
//    QNetworkAccessManager* nam = new QNetworkAccessManager(this);
//    qIn() << "QNetworkAccessManager Supported Schemes:" << nam->supportedSchemes();

	// Create the singletons we'll need for any app invocation.
	m_the_supported_mime_types = &SupportedMimeTypes::instance(this);

	/// @todo Experiments
//	m_cdb_model = new CollectionDatabaseModel(this);

	/// @todo TEMP hardcoded db file name in home dir.
//	auto db_dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

	// Create and set up the scan results tree model.
	m_srtm_instance = std::make_shared<ScanResultsTreeModel>(this);
	// Create and set the root item / headers
M_TODO("Needs to be ColumnSpecs");
	m_srtm_instance->setColumnSpecs({"DirProps", "MediaURL", "SidecarCueURL"});
	// Let's add two more columns
	m_srtm_instance->insertColumns(3, 2);


	/// @end Experiments

	// Connect to aboutToQuit() signal to perform app-end cleanup.
	// Per @link https://doc.qt.io/qt-5/qapplication.html#exec
	// "We recommend that you connect clean-up code to the aboutToQuit() signal, instead of putting it in your
	// application's main() function. This is because, on some platforms [Windows on user logoff] the QApplication::exec() call may not return."
	connect_or_die(AMLMApp::instance(), &QApplication::aboutToQuit, this, &AMLMApp::SLOT_onAboutToQuit);
}

void AMLMApp::MAIN_ONLY_setMainWindow(MainWindow* the_main_window)
{
	m_the_main_window = the_main_window;
}

AMLMApp* AMLMApp::instance()
{
    Q_ASSERT(m_the_instance != nullptr);
    return m_the_instance;
}

// Static
std::shared_ptr<ScanResultsTreeModel> AMLMApp::IScanResultsTreeModel()
{
	return amlmApp->m_srtm_instance;
};


QMimeDatabase& AMLMApp::mime_db()
{
	static QMimeDatabase* m_mime_database = new QMimeDatabase();
	return *m_mime_database;
}

bool AMLMApp::shuttingDown() const
{
	return true;
}

void AMLMApp::KDEOrForceBreeze(KConfigGroup group)
{
M_WARNING("TODO: Do something with this Breeze forcing.");
return;
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

void AMLMApp::SLOT_onAboutToQuit()
{
    // This slot is ~KDevelop's &Core::shutdown() slot, which is invoked by:
    // - the QCoreApplication::aboutToQuit() signal.
    // - Called directly by the MainWindow() destructor.

    qWr() << "##### App about to quit, shutting down event loop.";

    if(!m_shutting_down)
    {
        perform_controlled_shutdown();
    }

	qDbo() << "#### App shutdown complete.";

	if(!AMLMApp::IPerfectDeleter().empty())
	{
		qWro() << "PerfectDeleter still has undeleted objects:";
		qWro() << AMLMApp::IPerfectDeleter().stats();
	}
}

void AMLMApp::perform_controlled_shutdown()
{
    // This is ~Kdev's Core::cleanup() public member function.

    // We received a signal to ourselves that we're in the process of shutting down.
    m_shutting_down = true;

    // Signal to the world that we're in the process of shutting down.
	// This is triggered from the Qt5 aboutToQuit() signal:
	// @note From the aboutToQuit() docs:
	// "Emitted when the application is about to quit the main event loop."
	// "Note that no user interaction is possible in this state"
	/// @todo ... should we even be emitting it here?
	Q_EMIT SIGNAL_aboutToShutdown();

    if(!m_controlled_shutdown_complete)
    {
		// Do whatever shutdown tasks we need to in here.

		m_the_supported_mime_types->deleteLater();

		// Cancel all asynchronous activities and wait for them to complete.
		AMLMApp::IPerfectDeleter().cancel_and_wait_for_all();
    }
	else
	{
		qWr() << "REENTRY OF perform_controlled_shutdown";
	}

    m_controlled_shutdown_complete = true;

}
