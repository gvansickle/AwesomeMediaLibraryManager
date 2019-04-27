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
#include <vector>

/// Qt5
#include <QObject>
#include <QElapsedTimer>
#include <QPersistentModelIndex>
#include <QFuture>
#include <QFutureWatcher>
#include <QVector>

/// Ours
#include <src/concurrency/ExtAsync.h>
#include "LibraryRescannerMapItem.h"
#include <src/logic/models/AbstractTreeModelItem.h>

class LibraryModel;
class LibraryEntry;
class ScanResultsTreeModel;
class SharedItemContType;


struct MetadataReturnVal
{
	std::vector<QPersistentModelIndex> m_original_pindexes;
	std::vector<std::shared_ptr<LibraryEntry>> m_new_libentries;
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

// Typedef registered in cpp file.
using VecLibRescannerMapItems = QVector<LibraryRescannerMapItem>;


/**
 * Object which needs to be refactored badly.  Parent is a LibraryModel (which deleteLater()s it),
 * this object (re-)populates the model by scanning a directory tree in one phase, then loading metadata
 * from the files in the next phase.
 */
class LibraryRescanner : public QObject
{
	Q_OBJECT

Q_SIGNALS:

public:
	explicit LibraryRescanner(LibraryModel* parent);
	~LibraryRescanner() override;

	void startAsyncRescan(QVector<VecLibRescannerMapItems> items_to_rescan);

	QElapsedTimer m_timer;
	qint64 m_last_elapsed_time_dirscan {0};

public Q_SLOTS:
	void startAsyncDirectoryTraversal(const QUrl& dir_url);
	void cancelAsyncDirectoryTraversal();

//	void onDirTravFinished();
	/**
	 * Slot which accepts the incoming metadata.
	 */
    void SLOT_processReadyResults(MetadataReturnVal lritem_vec);

	/// Slot called by m_rescan_future_watcher when the rescan is complete.
//    void onRescanFinished();

protected:
	/// The map function for rescanning the library to reload metadata from the files.
	/// Runs in an arbitrary thread context, so must be threadsafe.
	MetadataReturnVal refresher_callback(const VecLibRescannerMapItems& mapitem);

	/// Experimental: Run XQuery in a separate thread.
	void ExpRunXQuery1(const QString& database_filename, const QString& in_filename);

	void SaveDatabase(ScanResultsTreeModel* tree_model_ptr, const QString& database_filename);

        
private:
	Q_DISABLE_COPY(LibraryRescanner)

	std::atomic_int m_main_sequence_monitor {0};
	std::atomic_bool m_model_ready_to_save_to_db {false};

	bool expect_and_set(int expect, int set);

	LibraryModel* m_current_libmodel;

	QFutureWatcher<QString> m_extfuture_watcher_dirtrav;
	/// @todo
	using ItemContType = std::vector<std::unique_ptr<AbstractTreeModelItem>>;
	using SharedItemContType = std::shared_ptr<ItemContType>;
	QFutureWatcher<SharedItemContType> m_efwatcher_tree_model_append;
	QFutureWatcher<MetadataReturnVal> m_extfuture_watcher_metadata;
};


#endif //AWESOMEMEDIALIBRARYMANAGER_LIBRARYRESCANNER_H
