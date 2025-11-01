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

/**
 * @file LibraryRescanner.cpp
 * Implementation of LibraryRescanner, an asynchronous helper for LibraryModel.
 */

#include "LibraryRescanner.h"

// Std C++
#include <functional>
#include <memory>

// Linux Callgrind
// @link http://www.valgrind.org/docs/manual/manual-core-adv.html#manual-core-adv.clientreq :
// "You are encouraged to copy the valgrind/*.h headers into your project's include directory, so your program doesn't
// have a compile-time dependency on Valgrind being installed. The Valgrind headers, unlike most of the rest of the
// code, are under a BSD-style license so you may include them without worrying about license incompatibility."
#include <valgrind/callgrind.h>

// Qt
#include <QThread>
#include <QVariant>
#include <QtConcurrentRun>

// KF
#include <KJobUiDelegate>
#include <KIO/DirectorySizeJob>

// Ours, Qt/KF-related
#include <utils/TheSimplestThings.h>
#include <utils/RegisterQtMetatypes.h>
#include <utils/QtHelpers.h>
#include "SupportedMimeTypes.h"


// Ours
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

    connect_or_die(this, &LibraryRescanner::SIGNAL_onIncomingPopulateRowWithItems_Multiple,
                   m_current_libmodel, &LibraryModel::SLOT_onIncomingPopulateRowWithItems_Multiple);
    connect_or_die(this, &LibraryRescanner::SIGNAL_setData,
    				m_current_libmodel, &LibraryModel::setData);
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

	// We use a blocking queue here to ensure we don't start metadata scanning before the dir scanning is completed.
	// This is overly conservative; we only need the total population of the model to be finished before proceeding,
	// not every individual filename entry.  But it's easy, and at the time of writing it doesn't seem to have a
	// meaningful performance penalty (prob. swamped by disk I/O).
	connect_or_die(this, &LibraryRescanner::SIGNAL_FileUrlQString,
		m_current_libmodel, &LibraryModel::SLOT_onIncomingFilename,
		Qt::BlockingQueuedConnection);

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

	// The promise/future that we'll use to move the LibraryRescannerMapItems to the library_metadata_rescan_task().
    QPromise<VecLibRescannerMapItems> rescan_items_in_promise;
    QFuture<VecLibRescannerMapItems> rescan_items_in_future = rescan_items_in_promise.future();

    //
    // Start the library_metadata_rescan_task.
    //
	ExtFuture<MetadataReturnVal> lib_rescan_future = QtConcurrent::run(library_metadata_rescan_task,
																							 rescan_items_in_future
																							 );
	// Make a new AMLMJobT for the metadata rescan.
	AMLMJobT<ExtFuture<MetadataReturnVal>>* lib_rescan_job = make_async_AMLMJobT(lib_rescan_future, "LibRescanJob", AMLMApp::instance());

	m_timer.lap("End setup, start continuation attachments");

	streaming_then(dirresults_future, [this](QFuture<DirScanResult> sthen_future, int begin, int end) -> Unit {
		// Start of the dirtrav streaming_then callback.  This should be a non-main thread.
		// This will be called multiple times by streaming_then() as DirScanResult's become available.

		AMLM_ASSERT_NOT_IN_GUITHREAD();

		if(begin == 0)
		{
            expect_and_set(1, 2);
		}

		// Create a new container instance we'll use to pass the incoming values to the GUI thread below.
		// Note The container here is really of type vector<shared_ptr<ScanResultsTreeModelItem>>
		SharedItemContType new_items = std::make_shared<ItemContType>();

		int original_end = end;
		for(int i=begin; i<end; i++)
		{
			DirScanResult dsr = sthen_future.resultAt(i);

			// Add another entry to the vector we'll send to the model.
			/// @todo It looks like this can be simplified, no longer a need for ScanResultsTreeModelItem in here.
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
			qWr() << "sthen_callback saw canceled";
			return unit;
		}
		if(sthen_future.isFinished())
		{
			qIn() << "sthen_callback saw finished";
			if(new_items->empty())
			{
                qWr() << "sthen_callback saw finished/empty new_items";
				return unit;
			}
            qIn() << "sthen_callback saw finished, but with" << new_items->size() << "outstanding results.";
		}

		// Shouldn't get here with no incoming items.
        Q_ASSERT_X(!new_items->empty(), "DIRTRAV CALLBACK", "NO NEW ITEMS BUT HIT STHEN CALLBACK");

		// Got all the ready results, send them to the model(s).
        // Because of Qt's model/view system not being threadsafeable, we have to do at least the final
		// model item entry from the GUI thread.

		for(std::shared_ptr<AbstractTreeModelItem> entry : *new_items)
		{
			// Send the URL ~dsr.getMediaExtUrl().m_url.toString()) to the LibraryModel.
			Q_EMIT SIGNAL_FileUrlQString(entry->data(1).toString());
		}

		Q_ASSERT(new_items->size() == 1);

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
	// Finish the output future.
    .then([this](ExtFuture<Unit> future) mutable {

    	expect_and_set(2, 3);
		AMLM_ASSERT_NOT_IN_GUITHREAD();

        future.waitForFinished();

        return unit;
    })
	/// .then() ############################################
	.then(qApp, [this,
			rescan_items_in_promise = std::move(rescan_items_in_promise)
			](ExtFuture<Unit> future_unit) mutable {
		AMLM_ASSERT_IN_GUITHREAD();

		m_timer.lap("GUI Thread dirtrav over start.");

		expect_and_set(3, 4);

		qDb() << "DIRTRAV COMPLETE, NOW IN GUI THREAD";

		// Succeeded, but we may still have outgoing filenames in flight.
		/// @todo Do we need to do something for potential in-flight filenames?
		qIn() << "DIRTRAV SUCCEEDED";
		m_timer.lap("DirTrav succeeded");
		qIn() << "Directory scan time params:";
		m_timer.print_results();

        m_model_ready_to_save_to_db = true; /// @todo REMOVE?
		Q_ASSERT(m_model_ready_to_save_to_db == true);
		m_model_ready_to_save_to_db = false;

		m_timer.lap("dirtrav over partial, starting metadata rescan.");

		// Directory traversal complete, start rescan.

		QVector<VecLibRescannerMapItems> rescan_items;

		qDb() << "GETTING RESCAN ITEMS";
		/// @todo What do we need to do with potential in-flight filenames that haven't landed in the model yet?
		rescan_items = m_current_libmodel->getLibRescanItems();

		qDb() << M_ID_VAL(rescan_items.size());

		if(rescan_items.empty())
		{
			qIn() << "Model has no items to rescan:" << m_current_libmodel;
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

    streaming_then(lib_rescan_future, [this](QFuture<MetadataReturnVal> lib_rescan_future, int begin, int end){
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

	m_timer.lap("Leaving startAsyncDirTrav");
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
        Q_EMIT SIGNAL_onIncomingPopulateRowWithItems_Multiple(lritem_vec.m_original_pindexes[0], lritem_vec.m_new_libentries);
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
            Q_EMIT SIGNAL_setData(initial_row_index, QVariant::fromValue(lritem_vec.m_new_libentries[i]));
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
