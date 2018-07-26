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

#include <logic/dbmodels/CollectionDatabaseModel.h>


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
    /// Emitted upon reception of aboutToQuit() signal.
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

    static AMLMApp* instance();

    CollectionDatabaseModel* cdb_instance() { return m_cdb_model; }

    /**
     * @return true if this app is in the process of shutting down.
     */
    bool shuttingDown() const;

    void KDEOrForceBreeze(KConfigGroup group);

public Q_SLOTS:
    /**
     * Connected to this app's aboutToQuit() signal.
     */
    void SLOT_onAboutToQuit();

protected:

    /**
     * Called from the SLOT_onAboutToQuit() slot to handle the shutdown of app subcomponents.
     */
    void perform_controlled_shutdown();

private:
    Q_DISABLE_COPY(AMLMApp)

    /// The AMLMApp singleton.
    static AMLMApp* m_the_instance;

    CollectionDatabaseModel* m_cdb_model;

    bool m_shutting_down {false};
    bool m_controlled_shutdown_complete {false};
};

#endif /* SRC_AMLMAPP_H_ */
