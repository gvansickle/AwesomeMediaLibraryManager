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

/** @file Interface for LibraryRescanner, an asynchronous helper for LibraryModel. */

#ifndef AWESOMEMEDIALIBRARYMANAGER_LIBRARYRESCANNER_H
#define AWESOMEMEDIALIBRARYMANAGER_LIBRARYRESCANNER_H

#include <memory>
#include <QtCore/QObject>
#include <QElapsedTimer>
#include <QtCore/QPersistentModelIndex>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <utils/concurrency/AsyncTaskManager.h>

#include "logic/LibraryEntry.h"

class LibraryModel;

struct LibraryRescannerMapItem
{
	QPersistentModelIndex pindex {QPersistentModelIndex()};
	std::shared_ptr<LibraryEntry> item {nullptr};
};

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

using VecLibRescannerMapItems = QVector<LibraryRescannerMapItem>;

class LibraryRescanner : public QObject
{
	Q_OBJECT

public:
	LibraryRescanner(LibraryModel* parent);
	~LibraryRescanner() override;

	void startAsyncRescan(QVector<VecLibRescannerMapItems> items_to_rescan);


	QFutureWatcher<MetadataReturnVal> m_rescan_future_watcher;
//	QFutureWatcher<QString> m_dir_traversal_future_watcher;

	QElapsedTimer m_timer;

signals:
	/// @name Signals forwarded from the QFutureWatcher's.
	///@{
	void progressRangeChanged(int minimum, int maximum);
	void progressValueChanged(int progressValue);
	void progressTextChanged(const QString &progressText);
	///@}

public slots:
	void startAsyncDirectoryTraversal(QUrl dir_url);

	void onDirTravResultReadyAt(int index);
	void onDirTravFinished();

	/// Slot called by m_rescan_future_watcher when it has a result available.
	void onResultReadyAt(int index, QFuture<MetadataReturnVal> f);

    void processReadyResults(MetadataReturnVal lritem_vec);

	/// Slot called by m_rescan_future_watcher when the rescan is complete.
	void onRescanFinished();

protected:
	/// The map function for rescanning the library to reload metadata from the files.
	/// Runs in an arbitrary thread context, so must be threadsafe.
	MetadataReturnVal refresher_callback(const VecLibRescannerMapItems& mapitem);

private:
	Q_DISABLE_COPY(LibraryRescanner)

	LibraryModel* m_current_libmodel;

	AsyncTaskManager m_async_task_manager;

    futureww<QString> m_futureww_dirscan;
    futureww<MetadataReturnVal> m_futureww;
};


#endif //AWESOMEMEDIALIBRARYMANAGER_LIBRARYRESCANNER_H
