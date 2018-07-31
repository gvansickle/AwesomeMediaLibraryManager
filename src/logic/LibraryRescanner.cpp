/*
 * Copyright 2017, 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/** @file Implementation of LibraryRescanner, an asynchronous helper for LibraryModel. */

#include "LibraryRescanner.h"
#include "SupportedMimeTypes.h"

/// Std C++
#include <functional>

/// Qt5
#include <QThread>
#include <QtConcurrent>

/// KF5
#include <KJobUiDelegate>
#include <KIO/DirectorySizeJob>

/// Ours
#include <AMLMApp.h>
#include <gui/MainWindow.h>
#include <utils/DebugHelpers.h>

/// Ours, Qt5/KF5-related
#include <utils/TheSimplestThings.h>
#include <utils/RegisterQtMetatypes.h>

#include "utils/AsyncDirScanner.h"
#include <concurrency/ExtAsync.h>
#include <concurrency/ReportingRunner.h>
#include <concurrency/AsyncTaskManager.h>
#include <concurrency/DirectoryScanJob.h>

#include "LibraryRescannerJob.h"

#include <gui/activityprogressmanager/ActivityProgressStatusBarTracker.h>

#include "logic/LibraryModel.h"


AMLM_QREG_CALLBACK([](){
	qIn() << "Registering LibraryRescanner types";
	// From #include <logic/LibraryRescanner.h>
	qRegisterMetaType<MetadataReturnVal>();
	qRegisterMetaType<QFuture<MetadataReturnVal>>();
	qRegisterMetaType<VecLibRescannerMapItems>();
    });


LibraryRescanner::LibraryRescanner(LibraryModel* parent) : QObject(parent)
{
	setObjectName("TheLibraryRescanner");

	// Somewhat redundant, but keep another pointer to the LibraryModel.
	m_current_libmodel = parent;
}

LibraryRescanner::~LibraryRescanner()
{
M_WARNING("TODO: THIS SHOULD CANCEL THE JOBS, OR THE JOBS SHOULDNT BE OWNED BY THIS");
}


MetadataReturnVal LibraryRescanner::refresher_callback(const VecLibRescannerMapItems& mapitem)
{
    MetadataReturnVal retval;
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

void LibraryRescanner::startAsyncDirectoryTraversal(QUrl dir_url)
{
    qDb() << "START:" << dir_url;

	// Time how long it takes.
	m_timer.start();

#if 0
	// Create the ControlledTask which will scan the directory tree for files.

//	ExtFuture<QString> future = AsyncDirectoryTraversal(dir_url);
	m_dirtrav_future = AsyncDirectoryTraversal(dir_url);
	qDb() << "ExtFuture<> RETURNED FROM ASYNCDIRTRAV:" << m_dirtrav_future;
	qDb() << "ATTACHING THEN, future:" << m_dirtrav_future;
	m_dirtrav_future.then([=](ExtFuture<QString> the_future) -> QString {
		qDb() << "then() Async Scan Complete, future:" << m_dirtrav_future;
		qDb() << "Directory scan complete.";
		m_last_elapsed_time_dirscan = m_timer.elapsed();
		qIn() << "Directory scan took" << m_last_elapsed_time_dirscan << "ms";
		// Directory traversal complete, start rescan.
		onDirTravFinished();
		return QString("THEN DONE");
	});
	qDb() << "THEN ATTACHED, future:" << m_dirtrav_future;

//	qDb() << "ATTACHING WAIT, future state:" << future;
//	future.wait();
//	qDb() << "WAIT ATTACHED, future state:" << future;

//	future.waitForFinished();
//	/// EXPERIMENTAL
//	future.cancel();
//	//throw QException();
//	qDebug() << "ERROR";
#else
    auto master_job_tracker = MainWindow::master_tracker_instance();
    Q_CHECK_PTR(master_job_tracker);

    auto extensions = SupportedMimeTypes::instance().supportedAudioMimeTypesAsSuffixStringList();

    DirectoryScannerAMLMJobPtr dirtrav_job = DirectoryScannerAMLMJob::make_job(this, dir_url, extensions,
									QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    LibraryRescannerJobPtr lib_rescan_job = LibraryRescannerJob::make_job(this);

	/// @todo EXPERIMENTAL: Also send it to the SQLITE DB model.
	auto dbmodel = AMLMApp::instance()->cdb_instance();
//	connect_or_die(dirtrav_job, &DirectoryScannerAMLMJob::entries, dbmodel, &CollectionDatabaseModel::SLOT_addDirScanResult);
	connect_or_die(dirtrav_job, &DirectoryScannerAMLMJob::entries, dbmodel, [=](auto dsr, auto kjob)
	{
		qIno() << "DBSTART";
#if 0
		auto db_conn = dbmodel->OpenDatabaseConnection("the_connection_name", true);

//		auto prepped_insert_query = new QSqlQuery(db_conn);
		QSqlQuery prepped_insert_query(db_conn);

		bool status = prepped_insert_query.prepare(QLatin1String(
			"INSERT INTO DirScanResults(media_url, sidecar_cuesheet_url, dirscanrelease) values (?, ?, ?)"));
		Q_ASSERT(status);

		dbmodel->addDirScanResult(prepped_insert_query, dsr);
#else
		dbmodel->SLOT_addDirScanResult(dsr);
#endif
		qIno() << "DBEND";
	});


	connect_or_die(dirtrav_job, &DirectoryScannerAMLMJob::entries, m_current_libmodel, [=](DirScanResult the_find, KJob* kjob)  {
        // Found a file matching the criteria.  Send it to the model.
		m_current_libmodel->SLOT_onIncomingFilename(the_find.getMediaExtUrl().m_url.toString());

        ;});

    dirtrav_job->then(this, [=](DirectoryScannerAMLMJob* kjob){
        qDb() << "DIRTRAV COMPLETE";
        if(kjob->error())
        {
            qWr() << "DIRTRAV FAILED:" << kjob->error() << ":" << kjob->errorText() << ":" << kjob->errorString();
            auto uidelegate = kjob->uiDelegate();
            Q_CHECK_PTR(uidelegate);
            uidelegate->showErrorMessage();
        }
        else
        {
            // Succeeded, but we may still have outgoing filenames in flight.
            qIn() << "DIRTRAV SUCCEEDED";
            m_last_elapsed_time_dirscan = m_timer.elapsed();
            qIn() << "Directory scan took" << m_last_elapsed_time_dirscan << "ms";
#if 0
            // Directory traversal complete, start rescan.

            QVector<VecLibRescannerMapItems> rescan_items;

            qDb() << "GETTING RESCAN ITEMS";

            rescan_items = m_current_libmodel->getLibRescanItems();

            qDb() << "rescan_items:" << rescan_items.size();
            Q_ASSERT(rescan_items.size() > 0);

            lib_rescan_job->setDataToMap(rescan_items, m_current_libmodel);

            // Start the metadata scan.
            qDb() << "STARTING RESCAN";
            lib_rescan_job->start();
#endif
        }
    });

    master_job_tracker->registerJob(dirtrav_job);
    master_job_tracker->setAutoDelete(dirtrav_job, false);
    master_job_tracker->setStopOnClose(dirtrav_job, true);
    master_job_tracker->registerJob(lib_rescan_job);
    master_job_tracker->setAutoDelete(lib_rescan_job, false);
    master_job_tracker->setStopOnClose(lib_rescan_job, true);

    // Start the asynchronous ball rolling.
    dirtrav_job->start();

#endif

	qDb() << "END:" << dir_url;
}

void LibraryRescanner::cancelAsyncDirectoryTraversal()
{
	m_dirtrav_future.cancel();
}

#if 0
ExtFuture<QString> LibraryRescanner::AsyncDirectoryTraversal(QUrl dir_url)
{
	qDb() << "START ASYNC";

	ExtFuture<QString> result = ExtAsync::run(this, &LibraryRescanner::SyncDirectoryTraversal, dir_url);

//	m_dirtrav_future = ExtAsync::run(this, &LibraryRescanner::SyncDirectoryTraversal, dir_url);

	qDb() << "RETURNED FROM ExtAsync:" << result;

	result.tap(this, [=](QString str){
//		qDb() << "FROM TAP:" << str;
//		qDb() << "IN onResultReady CALLBACK:" << result;
		runInObjectEventLoop([=](){ m_current_libmodel->onIncomingFilename(str);}, m_current_libmodel);
	})
	.tap(this, [=](ExtAsyncProgress prog) {
		Q_EMIT this->progressChanged(prog.min, prog.val, prog.max, prog.text);
	;})
	.then(this, [=](ExtFuture<QString> dummy){
		qDb() << "FROM THEN:" << dummy;
		return unit;
	;});


//	qDb() << "WAIT ASYNC";
//	result.wait();
//	qDb() << "END ASYNC";

	return result;
}

void LibraryRescanner::SyncDirectoryTraversal(ExtFuture<QString>& future, QUrl dir_url)
{
	qDb() << "ENTER SyncDirectoryTraversal";
	future.setProgressRange(0, 100);

	QDirIterator m_dir_iterator(dir_url.toLocalFile(),
								QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
								QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

	int num_files_found_so_far = 0;
	uint num_possible_files = 0;
	QString status_text = QObject::tr("Scanning for music files");

	future.setProgressRange(0, 0);
	future.setProgressValueAndText(0, status_text);

	while(m_dir_iterator.hasNext())
	{
		if(future.isCanceled())
		{
			// We've been cancelled.
			break;
		}
		if(future.isPaused())
		{
			// We're paused, wait for a resume signal.
			future.waitForResume();
		}

		// Go to the next entry and return the path to it.
		QString entry_path = m_dir_iterator.next();
		auto file_info = m_dir_iterator.fileInfo();

		if(file_info.isDir())
		{
			QDir dir = file_info.absoluteDir();

			// Update the max range to be the number of files we know we've found so far plus the number
			// of files potentially in this directory.
			num_possible_files = num_files_found_so_far + file_info.dir().count();

			future.setProgressRange(0, num_possible_files);
		}
		else if(file_info.isFile())
		{
			// It's a file.
			num_files_found_so_far++;

			QUrl file_url = QUrl::fromLocalFile(entry_path);

			// Send this path to the future.
			future.reportResult(file_url.toString());

			// Update progress.
			future.setProgressValueAndText(num_files_found_so_far, status_text);
		}
	}

	// We're done.  One last thing to clean up: We need to send the now-known max range out.
	// Then we need to send out the final progress value again, because it might have been throttled away
	// by Qt.
	num_possible_files = num_files_found_so_far;
	if (!future.isCanceled())
	{
		future.setProgressRange(0, num_possible_files);
		future.setProgressValueAndText(num_files_found_so_far, status_text);
	}

	if (future.isCanceled())
	{
		// Report that we were canceled.
		future.reportCanceled();
	}
	else
	{
		future.reportFinished();
	}

	qDebug() << "LEAVE SyncDirectoryTraversal";
}
#endif

#if 0

void LibraryRescanner::startAsyncRescan(QVector<VecLibRescannerMapItems> items_to_rescan)
{
	// Send out progress text.
	QString progtext = tr("Rereading metadata");

    m_timer.start();

	ExtFuture<MetadataReturnVal> future = QtConcurrent::mapped(items_to_rescan,
	                                    std::bind(&LibraryRescanner::refresher_callback, this, _1));

	future.tap(this, [this](MetadataReturnVal a) {
		// The result is ready tap.
		this->processReadyResults(a);
	})
	.tap(this, [=](ExtAsyncProgress prog) {
		// Progress update tap.
		Q_EMIT this->progressChanged(prog.min, prog.val, prog.max, progtext);
	;})
	.finally([this](){
		qDb() << "METADATA RESCAN COMPLETE";
		onRescanFinished();
	});
}

#endif

void LibraryRescanner::processReadyResults(MetadataReturnVal lritem_vec)
{
	// We got one of ??? things back:
	// - A single pindex and associated LibraryEntry*, maybe new, maybe a rescan..
	// - A single pindex and more than one LibraryEntry*, the result of the first scan after the file was found.
	// - Multiple pindexs and LibraryEntry*'s.  The result of a multi-track file rescan.

	if(lritem_vec.m_num_tracks_found == 0)
	{
		qCritical() << "RESULT WAS EMPTY";
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
