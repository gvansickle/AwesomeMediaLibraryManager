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
#include <QStandardPaths>
#include <QUrl>

// KF5

// Ours
#include <utils/TheSimplestThings.h>
#include <logic/SupportedMimeTypes.h>
#include <gui/Theme.h>
#include <logic/dbmodels/CollectionDatabaseModel.h>
#include <logic/PerfectDeleter.h>
#include <utils/RegisterQtMetatypes.h>


// Pointer to the singleton.
AMLMApp *AMLMApp::m_the_instance = nullptr;

AMLMApp::AMLMApp(int& argc, char** argv) : BASE_CLASS(argc, argv), m_perfect_deleter(this)
{
    Q_ASSERT(m_the_instance == nullptr);

    m_the_instance = this;

    setObjectName("TheAMLMApp");

	// Get the future cancel propagation infrastructure set up.
//	ExtAsync::ExtFuturePropagationHandler::InitStaticExtFutureState();
}

AMLMApp::~AMLMApp()
{
    /// @todo Shut down whatever still needs shutting down.

    delete m_mime_database;

	// No more singleton.
	m_the_instance = nullptr;

	qDb() << "AMLMApp SINGLETON DESTROYED";
}

void AMLMApp::Init(bool gtest_only)
{
	// Register our types with Qt.
	RegisterQtMetatypes();

    m_mime_database = new QMimeDatabase();

	/// @todo This is ugly, refactor this.
	if(gtest_only)
	{
		return;
	}

	/// @todo EXPERIMENTAL
//    QNetworkAccessManager* nam = new QNetworkAccessManager(this);
//    qIn() << "QNetworkAccessManager Supported Schemes:" << nam->supportedSchemes();

	// Create the singletons we'll need for any app invocation.
	/* QObject hierarchy will self-destruct this = */ new SupportedMimeTypes(this);

	/// @todo Experiments
	m_cdb_model = new CollectionDatabaseModel(this);

	/// @todo TEMP hardcoded db file name in home dir.
	auto db_dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
	QString db_file = db_dir + "/AMLMTestdb.sqlite3";

	// Create or open the database.
    /// @todo Removing.
//	m_cdb_model->InitDb(QUrl::fromLocalFile(db_file), "the_connection_name");

//	auto rel_table_model = m_cdb_model->make_reltable_model(this);
//	m_cdb_model->SLOT_addDirScanResult(QUrl("http://gbsfjdhg"));
//	m_cdb_model->SLOT_addDirScanResult(QUrl("http://the_next_one"), 1);

	QString str =
	"Getting Started				How to familiarize yourself with Qt Designer\n"
	" Launching Designer			Running the Qt Designer application\n"
	" The User Interface			How to interact with Qt Designer\n"

	"Designing a Component			Creating a GUI for your application\n"
	" Creating a Dialog			How to create a dialog\n"
	" Composing the Dialog		Putting widgets into the dialog example\n"
	" Creating a Layout			Arranging widgets on a form\n"
	" Signal and Slot Connections		Making widget communicate with each other\n"
			;


    /// @todo Move this somewhere.
//    m_cdb2_model_instance = new AbstractTreeModel({"DirProps", "MediaURL", "SidecarCueURL"}, str, this);

	// Create and set up the scan results model.
	m_srtm_instance = new ScanResultsTreeModel(str, this);
	QVector<QVariant> header_columns {"DirProps", "MediaURL", "SidecarCueURL"};
//	for(const QString& header : headers)
//	{
//		rootData << header;
//	}
	m_srtm_instance->setRootItem(m_srtm_instance->make_root_node(header_columns));


	/// @end Experiments

	/// @note This is a self-connection, not sure this will work as intended.
	connect_or_die(AMLMApp::instance(), &QCoreApplication::aboutToQuit, this, &AMLMApp::SLOT_onAboutToQuit);
}

AMLMApp *AMLMApp::instance()
{
    Q_ASSERT(m_the_instance != nullptr);
    return m_the_instance;
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

    qDbo() << "App about to quit, shutting down.";

    if(!m_shutting_down)
    {
        perform_controlled_shutdown();
    }

    qDbo() << "App shutdown complete.";
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
    Q_EMIT aboutToShutdown();

    if(!m_controlled_shutdown_complete)
    {
		// Do whatever shutdown tasks we need to in here.
#if 0
		ExtAsync::ExtFuturePropagationHandler::IExtFuturePropagationHandler()->close();
#endif 0
		// Cancel all asynchronous activities and wait for them to complete.
		AMLMApp::IPerfectDeleter()->cancel_and_wait_for_all();
    }

    m_controlled_shutdown_complete = true;

}
