/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
 * @file DatabaseScanJob.cpp
 */
#include <jobs/DatabaseScanJob.h>

// Qt5
#include <QUrl>

// Ours
#include <utils/DebugHelpers.h>
#include <models/ScanResultsTreeModel.h>
#include <Core.h>
#include <gui/MainWindow.h>
#include <logic/SupportedMimeTypes.h>
#include <AMLMApp.h>
#include "DirectoryScanJob.h"

/**
 * (Re-)Load the given tree model with a scan of the contents of @a dir_url.
 * @param dir_url
 * @param srtm
 */
void DatabaseScanJob::SLOT_startAsyncDirectoryTraversal_ForDB(const QUrl& dir_url, std::shared_ptr<ScanResultsTreeModel> srtm)
{
	qDb() << "START:" << dir_url;

//	expect_and_set(0, 1);

	// Time how long all this takes.
	m_timer.start("############ startAsyncDirectoryTraversal()");

	// Get a shared pointer to the Scan Results Tree model.
	std::shared_ptr<ScanResultsTreeModel> tree_model = srtm; ///AMLM::Core::self()->getScanResultsTreeModel();
	Q_CHECK_PTR(tree_model);

	// Clear it out.
	tree_model->clear();

	// Set the root URL of the scan results model.
	/// @todo Should this really be done here, or somewhere else?
	tree_model->setBaseDirectory(dir_url);
	// Set the default columnspecs.
	/// @todo I don't know why I hate this so much.  So very very much.
	tree_model->setColumnSpecs(AMLM::Core::self()->getDefaultColumnSpecs());

	auto master_job_tracker = MainWindow::master_tracker_instance();
	Q_CHECK_PTR(master_job_tracker);

	// Get a list of the file extensions we're looking for.
	auto extensions = SupportedMimeTypes::instance().supportedAudioMimeTypesAsSuffixStringList();

	// Run the directory scan in another thread.
	ExtFuture<DirScanResult> dirresults_future = ExtAsync::qthread_async_with_cnr_future(DirScanFunction, nullptr,
	                                                                                     dir_url,
	                                                                                     extensions,
	                                                                                     QDir::Filters(QDir::Files |
	                                                                                                   QDir::AllDirs |
	                                                                                                   QDir::NoDotAndDotDot),
	                                                                                     QDirIterator::Subdirectories);
	// Create/Attach an AMLMJobT to the dirscan future.
	QPointer<AMLMJobT<ExtFuture<DirScanResult>>> dirtrav_job = make_async_AMLMJobT(dirresults_future, "DirResultsAMLMJob", AMLMApp::instance());

}
