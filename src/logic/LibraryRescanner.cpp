/*
 * Copyright 2017, 2018, 2019, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/** @file LibraryRescanner.cpp
 * Implementation of LibraryRescanner, an asynchronous helper for LibraryModel.
 */

#include "LibraryRescanner.h"

/// Std C++
#include <functional>
#include <memory>

/// Linux Callgrind
/// @link http://www.valgrind.org/docs/manual/manual-core-adv.html#manual-core-adv.clientreq :
/// "You are encouraged to copy the valgrind/*.h headers into your project's include directory, so your program doesn't
/// have a compile-time dependency on Valgrind being installed. The Valgrind headers, unlike most of the rest of the
/// code, are under a BSD-style license so you may include them without worrying about license incompatibility."
#include <valgrind/callgrind.h>

// Qt
#include <QThread>
#include <QVariant>
#include <QtConcurrentRun>

// KF
#include <KJobUiDelegate>
#include <KIO/DirectorySizeJob>

/// Ours, Qt/KF-related
#include <utils/TheSimplestThings.h>
#include <utils/RegisterQtMetatypes.h>
#include <utils/QtHelpers.h>
#include "SupportedMimeTypes.h"


/// Ours
#include <AMLMApp.h>
#include <Core.h>
#include <gui/MainWindow.h>
#include <logic/models/AbstractTreeModelItem.h>
#include <logic/models/ScanResultsTreeModel.h>
#include <logic/models/SRTMItemLibEntry.h>
#include <utils/DebugHelpers.h>
#include <utils/ext_iterators.h>

#include <concurrency/AsyncTaskManager.h>
#include <jobs/DirectoryScanJob.h>

#include <jobs/LibraryRescannerJob.h>
#include <gui/activityprogressmanager/ActivityProgressStatusBarTracker.h>
#include <logic/serialization/XmlSerializer.h>
#include <logic/serialization/SerializationExceptions.h>
#include <logic/serialization/SerializationHelpers.h>
#include <utils/Stopwatch.h>

#include "models/LibraryModel.h"


AMLM_QREG_CALLBACK([](){
	qIn() << "Registering LibraryRescanner types";
	// From #include <logic/LibraryRescanner.h>
	qRegisterMetaType<MetadataReturnVal>();
	qRegisterMetaType<QFuture<MetadataReturnVal>>();
	qRegisterMetaType<QVector<LibraryRescannerMapItem>>("VecLibRescannerMapItems");
    });


LibraryRescanner::LibraryRescanner(LibraryModel* parent) : QObject(parent)
{
	setObjectName("TheLibraryRescanner");

	// Somewhat redundant, but keep another pointer to the LibraryModel.
	m_current_libmodel = parent;
}

LibraryRescanner::~LibraryRescanner()
{
	//M_WARNING("TODO: THIS SHOULD CANCEL THE JOBS, OR THE JOBS SHOULDNT BE OWNED BY THIS");
}


MetadataReturnVal LibraryRescanner::refresher_callback(const VecLibRescannerMapItems& mapitem)
{
    MetadataReturnVal retval;
	Q_ASSERT(0);
#if 0
	qDebug() << "Current thread:" << QThread::currentThread()->objectName();

	// If we have more than a single entry in the incoming list, we have a multi-track file to refresh.
	if(mapitem.size() == 1)
	{
		// Only one entry.

		// Get the LibraryEntry* to the existing entry.
M_WARNING("There's no locking here, there needs to be, or these need to be copies.");
		std::shared_ptr<LibraryEntry> item = mapitem[0].item;

		if(!item->isPopulated())
		{
			// Item's metadata has not been looked at.  We may have multiple tracks.

			// Only one pindex though.
			retval.m_original_pindexes.push_back(mapitem[0].pindex);

			auto vec_items = item->populate();
			for (auto i : vec_items)
			{
				if (!i->isPopulated())
				{
					qCritical() << "NOT POPULATED" << i.get();
				}
				retval.push_back(i);
			}
		}
		else if (item->isPopulated() && item->isSubtrack())
		{
			qCritical() << "TODO: FOUND SUBTRACK ITEM, SKIPPING:" << item->getUrl();
			Q_ASSERT(0);
		}
		else
		{
			//qDebug() << "Re-reading metatdata for item" << item->getUrl();
			std::shared_ptr<LibraryEntry> new_entry = item->refresh_metadata();

			if(new_entry == nullptr)
			{
				// Couldn't load the metadata from the file.
				// Only option here is to return the old item, which should now be marked with an error.
				qCritical() << "Couldn't load metadata for file" << item->getUrl();
				retval.m_original_pindexes.push_back(mapitem[0].pindex);
				retval.m_new_libentries.push_back(item);
				retval.m_num_tracks_found = 1;
			}
			else
			{
				// Repackage it and return.
				retval.m_original_pindexes.push_back(mapitem[0].pindex);
				retval.m_new_libentries.push_back(new_entry);
				retval.m_num_tracks_found = 1;
			}
		}
	}
	else if (mapitem.size() > 1)
	{
		// Multiple incoming tracks.
		std::shared_ptr<LibraryEntry> first_item = mapitem[0].item;
		auto subtracks = first_item->populate(true);
		if(subtracks.size() < mapitem.size())
		{
			// We got fewer back than we had before.
			if(subtracks.size() == 1)
			{
				// We only got one back.  This means we weren't able to read the file.
				return retval;
			}
			else
			{
				// Not sure what exactly we got back.
				qCritical() << "Unknown response: mapitem.size()==" << mapitem.size() << "subtracks.size()==" << subtracks.size();
				Q_ASSERT(0);
			}
		}
		for(int i=0; i<mapitem.size(); ++i)
		{
			Q_ASSERT(subtracks[i] != nullptr);
			retval.push_back(mapitem[i].pindex, subtracks[i]);
		}
	}
	else
	{
		qCritical() << "GOT EMPTY LIST OF LIBRARY ENTRIES TO RESCAN";
	}
#endif
    return retval;
}

void LibraryRescanner::SaveDatabase(std::shared_ptr<ScanResultsTreeModel> tree_model_ptr, const QString& database_filename)
{
	/// @todo Stub
	tree_model_ptr->SaveDatabase(database_filename);
}

void LibraryRescanner::LoadDatabase(std::shared_ptr<ScanResultsTreeModel> tree_model_ptr, const QString& database_filename)
{
	/// @todo Stub
	tree_model_ptr->LoadDatabase(database_filename);
}

void LibraryRescanner::startAsyncDirectoryTraversal(const QUrl& dir_url)
{
/// throwif<SerializationException>(!status, "########## COULDN'T OPEN FILE");

	expect_and_set(0, 1);

	// Time how long all this takes.
	m_timer.start("############ startAsyncDirectoryTraversal()");

    auto master_job_tracker = MainWindow::master_tracker_instance();
    Q_CHECK_PTR(master_job_tracker);

    // Get a list of the file extensions we're looking for.
    auto extensions = SupportedMimeTypes::instance().supportedAudioMimeTypesAsSuffixStringList();

    // Set up the directory scan to run in another thread.
    QFuture<DirScanResult> dirresults_future = QtConcurrent::run(DirScanFunction,
                                                                     dir_url,
                                                                     extensions,
                                                                     QDir::Filters(QDir::Files |
                                                                                   QDir::AllDirs |
                                                                                   QDir::NoDotAndDotDot),
                                                                     QDirIterator::Subdirectories);
	// Create/Attach an AMLMJobT to the dirscan future.
	QPointer<AMLMJobT<ExtFuture<DirScanResult>>> dirtrav_job = make_async_AMLMJobT(dirresults_future, "DirResultsJob", AMLMApp::instance());

	// The future that we'll use to move the LibraryRescannerMapItems to the library_metadata_rescan_task().
    QPromise<VecLibRescannerMapItems> rescan_items_in_promise;
    QFuture<VecLibRescannerMapItems> rescan_items_in_future = rescan_items_in_promise.future();

    ///
    /// Start the library_metadata_rescan_task.
    ///
	ExtFuture<MetadataReturnVal> lib_rescan_future = QtConcurrent::run(library_metadata_rescan_task,
																							 nullptr, rescan_items_in_future,
																							 nullptr /*m_current_libmodel*/);
	// Make a new AMLMJobT for the metadata rescan.
	AMLMJobT<ExtFuture<MetadataReturnVal>>* lib_rescan_job = make_async_AMLMJobT(lib_rescan_future, "LibRescanJob", AMLMApp::instance());

	// Create a future so we can attach a watcher to get the QUrl results to the main thread.
	/// @todo Obsoleting.
	auto qurl_promise = std::make_shared<QPromise<QString>>();
	QFuture<QString> qurl_future = qurl_promise->future();

	// Create a future so we can attach a continuation to get the results to the main thread.
	auto tree_model_item_promise = std::make_shared<QPromise<SharedItemContType>>();
	QFuture<SharedItemContType> tree_model_item_future = tree_model_item_promise->future();

	m_timer.lap("End setup, start continuation attachments");

	qurl_promise->start();
	tree_model_item_promise->start();

	// Get a pointer to the single ScanResultsTreeModel (view is the "ExperimentalKDE1" tab).
	std::shared_ptr<ScanResultsTreeModel> tree_model_sptr = AMLM::Core::self()->getScanResultsTreeModel();
    Q_CHECK_PTR(tree_model_sptr);
	// Clear the model, it may already have been populated.
	tree_model_sptr->clear(false);
	Q_ASSERT(tree_model_sptr->columnCount(QModelIndex()) == 0);
	auto num_rows = tree_model_sptr->rowCount(QModelIndex());
	if (tree_model_sptr->columnCount(QModelIndex()) == 0)
	{
		// Set up the columns to defaults.
		// Have to do this before any children are added.
		auto col_specs = AMLM::Core::self()->getDefaultColumnSpecs();
		tree_model_sptr->setColumnSpecs(col_specs);
	}
	qDb() << "START rowCount: " << num_rows << "checkConsistency" << tree_model_sptr->checkConsistency();
	// Set the root URL of the scan results model.
	tree_model_sptr->setBaseDirectory(dir_url);


std::shared_ptr<AbstractTreeModelItem> root = tree_model_sptr->getRootItem();

connect_or_die(this, &LibraryRescanner::SIGNAL_appendChildToRoot,
				tree_model_sptr.get(), &AbstractTreeModel::SLOT_appendChildToRoot);
connect_or_die(this, &LibraryRescanner::SIGNAL_appendChild, tree_model_sptr.get(), &AbstractTreeModel::SLOT_appendChild);

	streaming_then(dirresults_future, [this, tree_model_sptr, qurl_promise, tree_model_item_promise](QFuture<DirScanResult> sthen_future, int begin, int end) -> Unit {
		// Start of the dirtrav streaming_then callback.  This should be a non-main thread.
		// This will be called multiple times by streaming_then() as DirScanResult's become available.

		AMLM_ASSERT_NOT_IN_GUITHREAD();

		Q_ASSERT(tree_model_sptr);

		if(begin == 0)
		{
            expect_and_set(1, 2);
		}

		// Create a new container instance we'll use to pass the incoming values to the GUI thread below.
		/// @note The container here is really of type vector<shared_ptr<ScanResultsTreeModelItem>>
		SharedItemContType new_items = std::make_shared<ItemContType>();

		int original_end = end;
		for(int i=begin; i<end; i++)
		{
			DirScanResult dsr = sthen_future.resultAt(i);

			// Add another entry to the vector we'll send to the model.
			Q_ASSERT(tree_model_sptr);
            std::shared_ptr<ScanResultsTreeModelItem> new_item = ScanResultsTreeModelItem::create(dsr);
            new_items->push_back(new_item);

			if(i >= end)
			{
				// We're about to leave the loop.  Check if we have more ready results now than was originally reported.
				if(sthen_future.isResultReadyAt(end+1) || sthen_future.resultCount() > (end+1))
				{
					// There are more results now than originally reported.
					qIn() << "##### MORE RESULTS:" << M_NAME_VAL(original_end) << M_NAME_VAL(end) << M_NAME_VAL(sthen_future.resultCount());
					end = end+1;
				}
			}
		}
		// Broke out of loop, check for problems.
		if(sthen_future.isCanceled())
		{
			// We've been canceled.
			/// @todo Not sure if we should see that in here or not, probably not.
			qWr() << "tap_callback saw canceled";
			return unit;
		}
		if(sthen_future.isFinished())
		{
			qIn() << "tap_callback saw finished";
			if(new_items->empty())
			{
                qWr() << "sthen_callback saw finished/empty new_items";

//M_TODO("This needs to reportFinished before the next steps below which save the DB, NOT WORKING HERE");
//tree_model_item_future.reportFinished();

				return unit;
			}
            qIn() << "sthen_callback saw finished, but with" << new_items->size() << "outstanding results.";
		}

		// Shouldn't get here with no incoming items.
        Q_ASSERT_X(!new_items->empty(), "DIRTRAV CALLBACK", "NO NEW ITEMS BUT HIT STHEN CALLBACK");

		// Got all the ready results, send them to the model(s).
        // Because of Qt's model/view system not being threadsafeable, we have to do at least the final
		// model item entry from the GUI thread.
//		qIn() << "Sending" << new_items->size() << "scan results to models";
#if 1 // DEBUG!!!
        /// @todo Obsoleting... very... slowly.
		for(std::shared_ptr<AbstractTreeModelItem> entry : *new_items)
		{
			// Send the URL ~dsr.getMediaExtUrl().m_url.toString()) to the LibraryModel.
            qurl_promise->addResult(entry->data(1).toString());
		}

		/// @note This could also be a signal emit.
		Q_ASSERT(new_items->size() == 1);
        tree_model_item_promise->addResult(new_items);
#endif
    	return unit;
    })
/// .then() ############################################
	.then([](ExtFuture<void> fut_ignored) -> Unit {
                  fut_ignored.waitForFinished();
		// The dirscan is complete.
		qDb() << "DIRSCAN COMPLETE .then()";

		return unit;
	})
/// .then() ############################################
	// Finish the two output futures.
    .then([this, tree_model_item_future, tree_model_item_promise = std::move(tree_model_item_promise),
    			qurl_future, qurl_promise=std::move(qurl_promise)](ExtFuture<Unit> future) mutable {

        future.waitForFinished();

		// Finish a couple futures we started in this, and since this is done, there should be no more
		// results coming for them.

		expect_and_set(2, 3);
		AMLM_ASSERT_NOT_IN_GUITHREAD();

		qDb() << "FINISHING TREE MODEL FUTURE:" << M_ID_VAL(tree_model_item_future); // == (Running|Started)
        tree_model_item_promise->finish();
		qDb() << "FINISHED TREE MODEL FUTURE:" << M_ID_VAL(tree_model_item_future); // == (Started|Finished)
		m_timer.lap("Finished tree_model_item_future");

		qDb() << "FINISHING:" << M_ID_VAL(qurl_future);
        qurl_promise->finish();
		qDb() << "FINISHED:" << M_ID_VAL(qurl_future);
		m_timer.lap("Finished qurl_future");

        return unit;
    });

#if 1

	///
	/// Convert SharedItemContainerType's into TreeModelItems, then Append TreeModelItems to the ScanResultsTreeModel tree_model.
	///
    Q_ASSERT(tree_model_sptr);
/// streaming_then() ############################################
    streaming_then(tree_model_item_future,
                              [this, tree_model_sptr](ExtFuture<SharedItemContType> new_items_future, int begin, int end){
		AMLM_ASSERT_NOT_IN_GUITHREAD();

Stopwatch sw("Populate LibraryEntry");

		qDb() << "START: tree_model_item_future.stap(), new_items_future count:" << new_items_future.resultCount();

		// For each QList<SharedItemContType> entry.
		for(int index = begin; index < end; ++index)
		{
			auto result = new_items_future.resultAt(index);
			const SharedItemContType& new_items_vector_ptr = result;

			// Append ScanResultsTreeModelItem entries to the ScanResultsTreeModel.
			for(std::shared_ptr<AbstractTreeModelItem>& entry : *new_items_vector_ptr)
			{
                // AbstractTreeModelItem's need to have a model at all times?????
                entry->setModel(tree_model_sptr);
				// Make sure the entry wasn't moved from.
				Q_ASSERT(bool(entry) == true);
//				Q_ASSERT(entry->isInModel());
				auto entry_dp = std::dynamic_pointer_cast<ScanResultsTreeModelItem>(entry);
				Q_ASSERT(entry_dp);

				// Here we're only dealing with the per-file LibraryEntry's.

				// Populate the LibraryEntry in this non-GUI thread.
sw.start("LibEntry");
				std::shared_ptr<LibraryEntry> lib_entry = LibraryEntry::fromUrl(entry_dp->data(1).toString());
				lib_entry->populate(true);
sw.lap("Populate Complete");
                std::shared_ptr<SRTMItem_LibEntry> new_child = SRTMItem_LibEntry::create(lib_entry, tree_model_sptr);
				Q_ASSERT(new_child);
sw.lap("SRTMItem_LibEntry created");
				/// NEW: Give the incoming ScanResultTreeModelItem entry a parent.

//				entry_dp->changeParent(tree_model_sptr->getRootItem());
				// tree_model_sptr->getRootItem()->appendChild(entry_dp);
				// entry_dp->appendChild(new_child);
				Q_EMIT SIGNAL_appendChildToRoot(entry_dp);
				Q_EMIT SIGNAL_appendChild(new_child, entry_dp->getId());
//				tree_model_sptr->getRootItem()->appendChild(entry_dp);
			}

			// Finally, move the new model items to their new home.
			Q_ASSERT(new_items_vector_ptr->at(0));

//M_WARNING("PUT THIS BACK??");
//			tree_model_sptr->appendItems(*new_items_vector_ptr);
//			for(auto it : new_items_vector_ptr)
//			{
//				std::shared_ptr<AbstractTreeModelItem> new_child = the_etm->insertChild();
//			}
            /// @temp
            // qDb() << "Checking tree consistency after";
            // bool ok = tree_model_sptr->checkConsistency();
            // Q_ASSERT(ok);
            // qDb() << "ok";
            // sw.lap("TreeModel checkConsistency");
sw.stop();
sw.print_results();
		}
	})
#endif
	/// .then() ############################################
    .then(qApp, [&, tree_model_sptr](ExtFuture<void> f)-> Unit {
		Q_ASSERT(f.isFinished());
		Q_ASSERT(m_model_ready_to_save_to_db == false);
		m_model_ready_to_save_to_db = true;

        qDb() << "Checking tree consistency after";
        bool ok = tree_model_sptr->checkConsistency();
        Q_ASSERT(ok);
        qDb() << "ok";

        m_timer.lap("TreeModelItems sthen() finished.");
		return unit;
	})
	/// .then() ############################################
	.then(qApp, [this,
			rescan_items_in_promise = std::move(rescan_items_in_promise),
            tree_model_sptr
			](ExtFuture<Unit> future_unit) mutable {
		AMLM_ASSERT_IN_GUITHREAD();

		/// @note The time between this and the immediately previous "Finished qurl_future" takes all the time
		///       (about 271 secs atm).
		m_timer.lap("GUI Thread dirtrav over start.");

		expect_and_set(3, 4);

		qDb() << "DIRTRAV COMPLETE, NOW IN GUI THREAD";

		// Succeeded, but we may still have outgoing filenames in flight.
		qIn() << "DIRTRAV SUCCEEDED";
		m_timer.lap("DirTrav succeeded");
		qIn() << "Directory scan time params:";
		m_timer.print_results();

		// Save the database out to XML.
		QString database_filename = QDir::homePath() + "/AMLMDatabase.xml";

		Q_ASSERT(m_model_ready_to_save_to_db == true);
		m_model_ready_to_save_to_db = false;

		m_timer.lap("Start of SaveDatabase");

		tree_model_sptr->SaveDatabase(database_filename);
		m_timer.lap("End of SaveDatabase");

#if 1 /// ScanResultsTreeModel round-trip test.

		/// Try to load it back in and round-trip it.

		//				std::shared_ptr<ScanResultsTreeModel> load_tree = ScanResultsTreeModel::construct({ColumnSpec(SectionID(0), "DirProps"), {SectionID{0}, "MediaURL"}, {SectionID{0}, "SidecarCueURL"}});
		std::shared_ptr<ScanResultsTreeModel> load_tree	= ScanResultsTreeModel::create({});//temp_initlist);

		load_tree->LoadDatabase(database_filename);
		//				load_tree->clear();
		//				dump_map(load_tree);
		load_tree->SaveDatabase(QDir::homePath() + "/AMLMDatabaseRT.xml");
#endif /////

		/// @todo EXPERIMENTAL

		m_timer.lap("dirtrav over partial, starting metadata rescan.");

		// Directory traversal complete, start rescan.

		QVector<VecLibRescannerMapItems> rescan_items;

		qDb() << "GETTING RESCAN ITEMS";

		rescan_items = m_current_libmodel->getLibRescanItems();

		qDb() << M_ID_VAL(rescan_items.size());

		if(rescan_items.empty())
		{
			qDb() << "Model has no items to rescan:" << m_current_libmodel;
		}
		else
		{
			// Push all the LibraryRescannerMapItems into the promise and start the metadata rescan.
			rescan_items_in_promise.start();
			rescan_items_in_promise.addResults(rescan_items);
			rescan_items_in_promise.finish();

			m_timer.lap("GUI Thread dirtrav over partial, metadata rescan complete.");

			// Start the metadata scan.
			qDb() << "STARTING RESCAN";
			// lib_rescan_job->start();
		}
		//            lib_rescan_job->start();
    });

	if(dirtrav_job.isNull())
	{
		Q_ASSERT_X(0, __func__, "dirtrav is null");
	}

    master_job_tracker->registerJob(dirtrav_job);
//	master_job_tracker->setAutoDelete(dirtrav_job, false);
//  master_job_tracker->setStopOnClose(dirtrav_job, true);
	master_job_tracker->registerJob(lib_rescan_job);
//	master_job_tracker->setAutoDelete(lib_rescan_job, false);
//	master_job_tracker->setStopOnClose(lib_rescan_job, true);


	//
	// Hook up future watchers.
	//

	//
	// qurl_promise ->  ... -> LibraryModel::SLOT_onIncomingFilename [create and appendRow() as new unpopulated LibraryEntry]
	//
	auto* qurl_future_watcher = new QFutureWatcher<QString>();
	connect_or_die(qurl_future_watcher, &QFutureWatcher<QString>::resultReadyAt, qurl_future_watcher, [this, qurl_future](int i){
			/// @todo Maybe coming in out of order.
			QString url_str = qurl_future.resultAt(i);
			Q_EMIT SIGNAL_FileUrlQString(url_str);
	});
	connect_or_die(this, &LibraryRescanner::SIGNAL_FileUrlQString, m_current_libmodel, &LibraryModel::SLOT_onIncomingFilename);
	// Set self-destruct.
	connect_or_die(qurl_future_watcher, &QFutureWatcher<QString>::finished, qurl_future_watcher, &QFutureWatcher<QString>::deleteLater);
	qurl_future_watcher->setFuture(qurl_future);

    streaming_then(lib_rescan_future, [this](QFuture<MetadataReturnVal> lib_rescan_future, int begin, int end){
    	// Q_ASSERT(lib_rescan_future.isFinished());
		for(int i = begin; i<end; ++i)
		{
			qDb() << "lib_rescan_future sthen:" << i;
			this->SLOT_processReadyResults(lib_rescan_future.resultAt(i));
		}
	})
	.then([]()
	{
		qDb() << "lib_rescan_future sthen complete.";
	});

	// Make sure the above job gets canceled and deleted.
    // AMLMApp::IPerfectDeleter().addQFuture(QFuture<void>(tail_future));

// M_TODO("???? I think we're already started");
	// dirtrav_job->start();

	m_timer.lap("Leaving startAsyncDirTrav");

	qDb() << "LEAVING" << __func__ << ":" << dir_url;
}

void LibraryRescanner::cancelAsyncDirectoryTraversal()
{
//	m_dirtrav_future.cancel();
}

void LibraryRescanner::SLOT_processReadyResults(MetadataReturnVal lritem_vec)
{
	// We got one of ??? things back:
	// - A single pindex and associated LibraryEntry*, maybe new, maybe a rescan..
	// - A single pindex and more than one LibraryEntry*, the result of the first scan after the file was found.
	// - Multiple pindexs and LibraryEntry*'s.  The result of a multi-track file rescan.

	if(lritem_vec.m_num_tracks_found == 0)
	{
		qWr() << "RESULT WAS EMPTY";
		return;
	}

	if(lritem_vec.m_num_tracks_found > 1
	   && lritem_vec.m_original_pindexes.size() == 1
			&& lritem_vec.m_new_libentries.size() == lritem_vec.m_num_tracks_found)
	{
		// It's a valid, new, multi-track entry.
		m_current_libmodel->SLOT_onIncomingPopulateRowWithItems_Multiple(lritem_vec.m_original_pindexes[0], lritem_vec.m_new_libentries);
	}
	else if(lritem_vec.m_new_libentries.size() == lritem_vec.m_num_tracks_found
			&& lritem_vec.m_original_pindexes.size() == lritem_vec.m_num_tracks_found)
	{
		// It's a matching set of pindexes and libentries.

		for(int i=0; i<lritem_vec.m_num_tracks_found; ++i)
		{
			if (!lritem_vec.m_original_pindexes[i].isValid())
			{
				qWarning() << "Invalid persistent index, ignoring update";
				return;
			}

			// None of the returned entries should be null.
			Q_ASSERT(lritem_vec.m_new_libentries[i] != nullptr);

			qDebug() << "Replacing entry"; // << item->getUrl();
			// item is a single song which has its metadata populated.
			// Reconstruct the QModelIndex we sent out.
			auto initial_row_index = QModelIndex(lritem_vec.m_original_pindexes[i]);
			auto row = initial_row_index.row();
			qDebug() << QString("incoming single item, row %1").arg(row);
			// Metadata's been populated.
			m_current_libmodel->setData(initial_row_index, QVariant::fromValue(lritem_vec.m_new_libentries[i]));
		}
	}
	else
	{
		// Not sure what we got.
		qCritical() << "pindexes/libentries/num_new_entries:" << lritem_vec.m_original_pindexes.size()
															  << lritem_vec.m_new_libentries.size();
															  // lritem_vec.m_new_libentries;
		Q_ASSERT_X(0, "Scanning", "Not sure what we got");
	}
}

bool LibraryRescanner::expect_and_set(int expect, int set)
{
	Q_ASSERT(expect == m_main_sequence_monitor);
	m_main_sequence_monitor = set;
	return true;
}
/// END @todo MORE EXERIMENTS, QIODevice.

#if 0
void LibraryRescanner::onRescanFinished()
{
    auto elapsed = m_timer.elapsed();
	qDebug() << "Async Rescan reports finished.";
	qInfo() << "Directory scan took" << m_last_elapsed_time_dirscan << "ms";
	qInfo() << "Metadata rescan took" << elapsed << "ms";
	// Send out progress text.
	Q_EMIT progressChanged(0, 0, 0, "Idle");
}

void LibraryRescanner::onDirTravFinished()
{
	qDb() << "Async Dir Trav reports fisished.";

	// Send out progress text.
	Q_EMIT progressChanged(0, 0, 0, "Idle");

	/// @todo Should be a lambda.
	///m_current_libmodel->onIncomingFilenamesComplete();
	m_current_libmodel->startRescan();
}

#endif
