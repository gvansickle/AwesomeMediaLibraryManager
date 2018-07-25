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

/** @file Interface for LibraryRescanner, an asynchronous helper for LibraryModel. */

#ifndef AWESOMEMEDIALIBRARYMANAGER_LIBRARYRESCANNER_H
#define AWESOMEMEDIALIBRARYMANAGER_LIBRARYRESCANNER_H

/// Std C++
#include <memory>

/// Qt5
#include <QObject>
#include <QElapsedTimer>
#include <QPersistentModelIndex>
#include <QFuture>
#include <QFutureWatcher>
#include <QVector>

/// Ours
#include <concurrency/ExtAsync.h>
#include <concurrency/AsyncTaskManager.h> ///< For futureww

#include "LibraryRescannerMapItem.h"

class LibraryModel;
class LibraryEntry;


struct MetadataReturnVal
{
	QVector<QPersistentModelIndex> m_original_pindexes;
	QVector<std::shared_ptr<LibraryEntry>> m_new_libentries;
	int m_num_tracks_found {0};

	void push_back(QPersistentModelIndex pmi, std::shared_ptr<LibraryEntry> le)
	{
		m_original_pindexes.push_back(pmi);
		m_new_libentries.push_back(le);
		m_num_tracks_found++;
	}

	void push_back(std::shared_ptr<LibraryEntry> le)
	{
		m_new_libentries.push_back(le);
		m_num_tracks_found++;
	}
};

Q_DECLARE_METATYPE(MetadataReturnVal)
Q_DECLARE_METATYPE(QFuture<MetadataReturnVal>)
Q_DECLARE_METATYPE(ExtFuture<MetadataReturnVal>)

using VecLibRescannerMapItems = QVector<LibraryRescannerMapItem>;

class LibraryRescanner : public QObject
{
	Q_OBJECT

Q_SIGNALS:

public:
	LibraryRescanner(LibraryModel* parent);
	~LibraryRescanner() override;

	void startAsyncRescan(QVector<VecLibRescannerMapItems> items_to_rescan);

	QElapsedTimer m_timer;
	qint64 m_last_elapsed_time_dirscan {0};

public Q_SLOTS:
	void startAsyncDirectoryTraversal(QUrl dir_url);
	void cancelAsyncDirectoryTraversal();

	/// @todo EXPERIMENTAL
//	ExtFuture<QString> AsyncDirectoryTraversal(QUrl dir_url);
//	void SyncDirectoryTraversal(ExtFuture<QString>& future, QUrl dir_url);

//	void onDirTravFinished();

    void processReadyResults(MetadataReturnVal lritem_vec);

	/// Slot called by m_rescan_future_watcher when the rescan is complete.
//    void onRescanFinished();

protected:
	/// The map function for rescanning the library to reload metadata from the files.
	/// Runs in an arbitrary thread context, so must be threadsafe.
	MetadataReturnVal refresher_callback(const VecLibRescannerMapItems& mapitem);

private:
	Q_DISABLE_COPY(LibraryRescanner)

	LibraryModel* m_current_libmodel;

	ExtFuture<QString> m_dirtrav_future;

    futureww<MetadataReturnVal> m_futureww;
};


#endif //AWESOMEMEDIALIBRARYMANAGER_LIBRARYRESCANNER_H
