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

#ifndef SRC_AMLMAPP_H_
#define SRC_AMLMAPP_H_

#include <config.h>

// Qt5
#include <QApplication>

// KF5
#include <KConfigGroup>
#include <QMimeDatabase>

// Ours
#include <logic/dbmodels/CollectionDatabaseModel.h>

#include <logic/models/AbstractTreeModel.h>
#include <logic/models/ScanResultsTreeModel.h>


/**
 * qApp-alike macro for getting a pointer to the AMLMApp singleton.
 */
#define amlmApp AMLMApp::instance()

/**
 * The AMLM Application singleton.
 */
class AMLMApp: public QApplication
{
	Q_OBJECT

    using BASE_CLASS = QApplication;

Q_SIGNALS:

    /// @name Inherited signals
    /// @{

    /**
     * "Emitted when the application is about to quit the main event loop."
     * "Note that no user interaction is possible in this state"
     */
    // void QCoreApplication::aboutToQuit()

    /// @}

    /// Emitted upon reception of aboutToQuit() signal.
    /// Per @link http://doc.qt.io/qt-5/qcoreapplication.html#exec:
    /// "We recommend that you connect clean-up code to the aboutToQuit() signal, instead of putting it
    /// in your application's main() function because on some platforms the exec() call may not return."
	void aboutToShutdown();

public:
    /**
     * Ordinarily would be a protected member for a singleton.
     * Passed in params may be modified by QApplication.
     *
     * @param argc  argc passed into main().
     * @param argv  argv passed into main().
     */
    explicit AMLMApp(int& argc, char **argv);
	~AMLMApp() override;

	/**
	 * Post-constructor initialization.
	 */
	void Init(bool gtest_only = false);

    /**
     * @returns the AMLMApp singleton.
     */
    static AMLMApp* instance();

    /// @name "Controllers", per KDevelop ICore terminology.
    /// Basically just a bunch of instance()'s for singletons used by the app.
    /// @{

//    ActivityProgressStatusBarTracker == see MainWindow, this currently needs a parent widget.


    CollectionDatabaseModel* cdb_instance() { return m_cdb_model; }
	AbstractTreeModel* cdb2_model_instance() { return m_cdb2_model_instance; }

    ScanResultsTreeModel* scan_results_tree_model_instance() { return m_srtm_instance; };

    QMimeDatabase* mime_db() { return m_mime_database; };

    /// @}

    /**
     * @return true if this app is in the process of shutting down.
     */
    bool shuttingDown() const;

    void KDEOrForceBreeze(KConfigGroup group);

public Q_SLOTS:

    /// @name Inherited slots
    /// @{

    /**
     * "Tells the application to exit with return code 0 (success). Equivalent to calling QCoreApplication::exit(0)."
     * "always connect signals to this slot using a QueuedConnection. If a signal connected (non-queued) to this slot
     * is emitted before control enters the main event loop (such as before "int main" calls exec()), the slot has no
     * effect and the application never exits."
     */
    // void QCoreApplication::quit()

    /// @}

    /**
     * Connected to this app's aboutToQuit() signal.
     */
    void SLOT_onAboutToQuit();

protected:

    /**
     * Called from the SLOT_onAboutToQuit() slot to handle the shutdown of app subcomponents.
     * This is ~Kdev's Core::cleanup() public member function.
     */
    void perform_controlled_shutdown();

private:
    Q_DISABLE_COPY(AMLMApp)

    /// The AMLMApp singleton.
    static AMLMApp* m_the_instance;

    CollectionDatabaseModel* m_cdb_model;

	AbstractTreeModel* m_cdb2_model_instance;

    ScanResultsTreeModel* m_srtm_instance;

    QMimeDatabase* m_mime_database;

    std::atomic_bool m_shutting_down {false};
    std::atomic_bool m_controlled_shutdown_complete {false};
};

#endif /* SRC_AMLMAPP_H_ */
