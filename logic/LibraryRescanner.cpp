/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include <functional>

#include <QThread>
#include <QtConcurrent>

#include "utils/AsyncDirScanner.h"
#include "utils/concurrency/ReportingRunner.h"
#include "logic/LibraryModel.h"


using std::placeholders::_1;


LibraryRescanner::LibraryRescanner(LibraryModel* parent) : QObject(parent), m_rescan_future_watcher(this), m_dir_traversal_future_watcher(this)
{
	// Somewhat redundant, but keep another pointer to the LibraryModel.
	m_current_libmodel = parent;

	// Create a QFutureWatcher and connect up signals/slots.
	//m_rescan_future_watcher = new QFutureWatcher<VecLibRescannerMapItems>(this);

	/// Forward QFutureWatcher progress signals.
	connect(&m_rescan_future_watcher, SIGNAL(progressRangeChanged(int,int)), SIGNAL(progressRangeChanged(int,int)));
	connect(&m_rescan_future_watcher, SIGNAL(progressValueChanged(int)), SIGNAL(progressValueChanged(int)));
	connect(&m_rescan_future_watcher, SIGNAL(progressTextChanged(const QString &)), SIGNAL(progressTextChanged(const QString &)));
	connect(&m_rescan_future_watcher, SIGNAL(resultReadyAt(int)), SLOT(onResultReadyAt(int)));
	connect(&m_rescan_future_watcher, SIGNAL(finished()), SLOT(onRescanFinished()));

	//m_dir_traversal_future_watcher = new QFutureWatcher<QString>(this);

	connect(&m_dir_traversal_future_watcher, SIGNAL(progressRangeChanged(int,int)), SIGNAL(progressRangeChanged(int,int)));
	connect(&m_dir_traversal_future_watcher, SIGNAL(progressValueChanged(int)), SIGNAL(progressValueChanged(int)));
	connect(&m_dir_traversal_future_watcher, SIGNAL(progressTextChanged(const QString &)), SIGNAL(progressTextChanged(const QString &)));
	connect(&m_dir_traversal_future_watcher, SIGNAL(resultReadyAt(int)), SLOT(onDirTravResultReadyAt(int)));
	connect(&m_dir_traversal_future_watcher, SIGNAL(finished()), SLOT(onDirTravFinished()));

    // On Windows at least, these don't seem to throttle automatically as well as on Linux.
    m_rescan_future_watcher.setPendingResultsLimit(10);
    m_dir_traversal_future_watcher.setPendingResultsLimit(10);
}

LibraryRescanner::~LibraryRescanner()
{
	// Make sure we don't have any async tasks running when we get destroyed.
	m_rescan_future_watcher.cancel();
	m_dir_traversal_future_watcher.cancel();
	m_rescan_future_watcher.waitForFinished();
	m_dir_traversal_future_watcher.waitForFinished();
}


MetadataReturnVal LibraryRescanner::refresher_callback(const VecLibRescannerMapItems& mapitem)
{
	qDebug() << "Enter";
	qDebug() << "Current thread:" << QThread::currentThread()->objectName();

	MetadataReturnVal retval;

	// If we have more than a single entry in the incoming list, we have a multi-track file to refresh.
	if(mapitem.size() == 1)
	{
		// Only one entry.

		// Get the LibraryEntry* to the existing entry.
		/// @todo There's no locking here, there needs to be, or these need to be copies.
		LibraryEntry *item = mapitem[0].item;

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
					qCritical() << "NOT POPULATED" << i;
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
			LibraryEntry *new_entry = item->refresh_metadata();

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
		LibraryEntry* first_item = mapitem[0].item;
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

	qDebug() << "Exit";

	return retval;
}


void LibraryRescanner::startAsyncDirectoryTraversal(QUrl dir_url)
{
	emit progressTextChanged("Scanning directory tree");

	QFuture<QString> fut = ReportingRunner::run(new AsyncDirScanner(dir_url,
	                                                                QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
	                                                                QDir::NoFilter, QDirIterator::Subdirectories));
	m_dir_traversal_future_watcher.setFuture(fut);
}

void LibraryRescanner::startAsyncRescan(QVector<VecLibRescannerMapItems> items_to_rescan)
{
	// Send out progress text.
	emit progressTextChanged("Rereading metadata");

	// Start the mapped operation, set the future watcher to the returned future, and we're scanning.
	m_rescan_future_watcher.setFuture(QtConcurrent::mapped(
			items_to_rescan,
			std::bind(&LibraryRescanner::refresher_callback, this, _1)));
}

void LibraryRescanner::onResultReadyAt(int index)
{
	//qDebug() << "Async Rescan reports result ready at" << index;

	MetadataReturnVal lritem_vec = m_rescan_future_watcher.resultAt(index);

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
		m_current_libmodel->onIncomingPopulateRowWithItems_Multiple(lritem_vec.m_original_pindexes[0], lritem_vec.m_new_libentries);
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
		                                                      << lritem_vec.m_new_libentries.size()
		                                                      << lritem_vec.m_new_libentries;
		Q_ASSERT_X(0, "Scanning", "Not sure what we got");
	}
}

void LibraryRescanner::onRescanFinished()
{
	qDebug() << "Async Rescan reports fisished.";
	// Send out progress text.
	emit progressTextChanged("Idle");
}

void LibraryRescanner::onDirTravResultReadyAt(int index)
{
	qDebug() << "Async Dir Trav reports result ready at" << index << "==" << m_dir_traversal_future_watcher.resultAt(index);

	m_current_libmodel->onIncomingFilename(m_dir_traversal_future_watcher.resultAt(index));
}

void LibraryRescanner::onDirTravFinished()
{
	qDebug() << "Async Dir Trav reports fisished.";

	// Send out progress text.
	emit progressTextChanged("Idle");

	/// @todo Should be a lambda.
	///m_current_libmodel->onIncomingFilenamesComplete();
	m_current_libmodel->startRescan();
}


