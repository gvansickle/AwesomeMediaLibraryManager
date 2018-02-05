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

#include <functional>

#include <QThread>
#include <QtConcurrent>
#if 0//def USE_BUNDLED_ASYNCFUTURE
#include <asyncfuture.h>
#include <utils/concurrency/ExtendedDeferred.h>
#endif

// Simon Brunel's QtPromise.
// https://github.com/simonbrunel/qtpromise
#include <QtPromise>


#include <utils/concurrency/ExtFutureWatcher.h>

#include <utils/DebugHelpers.h>

#include "utils/AsyncDirScanner.h"
#include "utils/concurrency/AsyncTaskManager.h"
#include "utils/concurrency/ReportingRunner.h"
#include "logic/LibraryModel.h"


using std::placeholders::_1;


LibraryRescanner::LibraryRescanner(LibraryModel* parent) : QObject(parent), m_rescan_future_watcher(this),
//	m_dir_traversal_future_watcher(this),
	m_async_task_manager(this)
{
	setObjectName("TheLibraryRescanner");

	// Somewhat redundant, but keep another pointer to the LibraryModel.
	m_current_libmodel = parent;

	// Create a QFutureWatcher and connect up signals/slots.
	//m_rescan_future_watcher = new QFutureWatcher<VecLibRescannerMapItems>(this);

	/// Forward QFutureWatcher progress signals.
	connect(&m_rescan_future_watcher, SIGNAL(progressRangeChanged(int,int)), SIGNAL(progressRangeChanged(int,int)));
	connect(&m_rescan_future_watcher, SIGNAL(progressValueChanged(int)), SIGNAL(progressValueChanged(int)));
	connect(&m_rescan_future_watcher, SIGNAL(progressTextChanged(const QString &)), SIGNAL(progressTextChanged(const QString &)));
M_WARNING("QObject::connect: No such slot LibraryRescanner::onResultReadyAt(int) in /home/gary/src/AwesomeMediaLibraryManager/logic/LibraryRescanner.cpp:51");
	connect(&m_rescan_future_watcher, SIGNAL(resultReadyAt(int)), SLOT(onResultReadyAt(int)));
	connect(&m_rescan_future_watcher, SIGNAL(finished()), SLOT(onRescanFinished()));

	//m_dir_traversal_future_watcher = new QFutureWatcher<QString>(this);

//	connect(&m_dir_traversal_future_watcher, SIGNAL(progressRangeChanged(int,int)), SIGNAL(progressRangeChanged(int,int)));
//	connect(&m_dir_traversal_future_watcher, SIGNAL(progressValueChanged(int)), SIGNAL(progressValueChanged(int)));
//	connect(&m_dir_traversal_future_watcher, SIGNAL(progressTextChanged(const QString &)), SIGNAL(progressTextChanged(const QString &)));
//	connect(&m_dir_traversal_future_watcher, SIGNAL(resultReadyAt(int)), SLOT(onDirTravResultReadyAt(int)));
//	connect(&m_dir_traversal_future_watcher, SIGNAL(finished()), SLOT(onDirTravFinished()));

    // On Windows at least, these don't seem to throttle automatically as well as on Linux.
    m_rescan_future_watcher.setPendingResultsLimit(1);
//    m_dir_traversal_future_watcher.setPendingResultsLimit(1);
}

LibraryRescanner::~LibraryRescanner()
{
	// Make sure we don't have any async tasks running when we get destroyed.
	m_rescan_future_watcher.cancel();
//	m_dir_traversal_future_watcher.cancel();
	m_rescan_future_watcher.waitForFinished();
//	m_dir_traversal_future_watcher.waitForFinished();
}


MetadataReturnVal LibraryRescanner::refresher_callback(const VecLibRescannerMapItems& mapitem)
{
	qDebug() << "Current thread:" << QThread::currentThread()->objectName();

	MetadataReturnVal retval;

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

	return retval;
}

void LibraryRescanner::startAsyncDirectoryTraversal(QUrl dir_url)
{
	qDebug() << M_THREADNAME();
	qDebug() << "START:" << dir_url;

	// Time how long it takes.
	m_timer.start();

	// Create the ControlledTask which will scan the directory tree for files.

#if 1 // USE_BUNDLED_SB_QTPROMISE Simon Brunel's qtpromise.
	using namespace QtPromise;

	QFutureInterface<QString> future_interface = ReportingRunner::runFI(new AsyncDirScanner(dir_url,
												  QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
												  QDir::NoFilter, QDirIterator::Subdirectories));

	QPromise<QString> promise = qPromise(future_interface.future());

	ExtFutureWatcher<QString>* fw = new ExtFutureWatcher<QString>();

	fw->onProgressChange([=](int min, int val, int max, QString text) -> void {
					qDebug() << M_THREADNAME() << "PROGRESS+TEXT SIGNAL: " << min << val << max << text;
					Q_EMIT progressChanged(min, val, max, text);
					;})
				.onReportResult([=](QString s, int index) {
					Q_UNUSED(index);
					/// @note This lambda is called in an arbitrary thread context.
//					qDebug() << M_THREADNAME() << "RESULT:" << s << index;

					// This is not threadsafe:
					/// WRONG: this->m_current_libmodel->onIncomingFilename(s);

					// This is threadsafe.
					QMetaObject::invokeMethod(this->m_current_libmodel, "onIncomingFilename", Q_ARG(QString, s));
				})
				.setFuture(future_interface);

	qDebug() << "future is finished:" << fw->future().isFinished() << "isPending/Fulfilled:" << promise.isPending() << promise.isFulfilled();

	promise.then([&](QString res){
		qDebug() << M_THREADNAME() << "Directory scan complete.";
		m_last_elapsed_time_dirscan = m_timer.elapsed();
		qInfo() << "Directory scan took" << m_last_elapsed_time_dirscan << "ms";
		// Directory traversal complete.
		onDirTravFinished();
	}); //.wait();

	qDebug() << "future is finished:" << fw->future().isFinished() <<"isPending/Fulfilled:" << promise.isPending() << promise.isFulfilled();

#elif 0 /// USE_PROMISE

	/// This is key:
	/// https://stackoverflow.com/a/22205495
	/// "The future() call is a factory method for creating a QFuture bound to your QFutureInterface."
	/// It looks like it is creating a new one each time.
	Promise<QString> promise;

	qDebug() << promise.state();

	QFutureWatcher<QString> fw(this);
	connect(&fw, &QFutureWatcher<QString>::progressValueChanged, [=](int b) -> void {
		qDebug() << M_THREADNAME();
		qDebug() << "PROGRESS" << b; // << ":" << future.resultAt(b);
	});

	fw.setFuture(promise.future());

	auto new_future = ReportingRunner(async_dir_scanner, promise);

	qDebug() << "COUNT:" << new_future.resultCount();

//	auto result = promise.future().result();
//	qDebug() << promise.state();
//	qDebug() << "RESULT:" << result;

	qDebug() << promise.state();

	new_future.waitForFinished();

	qDebug() << "AFTER WAIT:" << promise.state();


#elif 0 /// USE_FUTUREWW

	QFuture<QString> fut = ReportingRunner::run(new AsyncDirScanner(dir_url,
	                                                                QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
	                                                                QDir::NoFilter, QDirIterator::Subdirectories));
    m_futureww_dirscan.on_result([this](auto a){
		qDebug() << M_THREADNAME() << "INCOMING FILENAME:" << a;
        this->m_current_libmodel->onIncomingFilename(a);
    })
    .on_progress([this](int min, int max, int val){
		qDebug() << M_THREADNAME() << "PROGRESS";
        emit progressRangeChanged(min, max);
        emit progressValueChanged(val);
    })
	.then([=](){
		qDebug() << M_THREADNAME() << "DIRTRAV RESCAN FINISHED";
        onDirTravFinished();
    });

	Promise<QString> promise;

	qDebug() << promise.state();
	promise.then([=](Promise<QString>& p) -> Promise<QString>& {
		qDebug() << M_THREADNAME() << "THEN1";
		p.reportStarted();
		p.reportFinished();
		return p;});

	qDebug() << "AFTER";
#if 0
			.tap([=](QFutureInterface<QString>& future){
		qDebug() << M_THREADNAME() << "TAP";
		return future;
			;})
#endif

    m_futureww_dirscan = fut;

#endif // Which future/promise to use.

	qDebug() << "END:" << dir_url;
}

void LibraryRescanner::startAsyncRescan(QVector<VecLibRescannerMapItems> items_to_rescan)
{
	// Send out progress text.
	QString progtext = tr("Rereading metadata");

    m_timer.start();

M_WARNING("EXPERIMENTAL");
#if 0
	m_async_task_manager.addFuture(QtConcurrent::mapped(items_to_rescan,
									   std::bind(&LibraryRescanner::refresher_callback, this, _1)),
								   [this](int index, QFuture<MetadataReturnVal> f){
		qDebug() << "RESULT.  this:" << this << "int:" << index;
		qDebug() << "Current thread:" << QThread::currentThread()->objectName();
		this->onResultReadyAt(index, f);
	},
									[](){
		qDebug() << "FINISHED";
		qDebug() << "Current thread:" << QThread::currentThread()->objectName();
		},
									[](){ qDebug() << "CANCELLED"; }
	);

#elif 0
	// Start the mapped operation, set the future watcher to the returned future, and we're scanning.
	m_rescan_future_watcher.setFuture(QtConcurrent::mapped(
			items_to_rescan,
			std::bind(&LibraryRescanner::refresher_callback, this, _1)));
#elif 1

    m_futureww
//	.on_resultat([](int at){
//        qDebug() << "RESULT AT:" << at << "THREAD:" << QThread::currentThread()->objectName();
//    })
	.on_result([this](auto a){
    	this->processReadyResults(a);
    })
	.on_progress([=](int min, int max, int val){
		emit progressChanged(min, val, max, progtext);
    })
    .then([=](){
    	qDebug() << "METADATA RESCAN FINISHED, THREAD:" << QThread::currentThread()->objectName();
    	onRescanFinished();
    });
    m_futureww = QtConcurrent::mapped(items_to_rescan,
                                    std::bind(&LibraryRescanner::refresher_callback, this, _1));
#endif
}

void LibraryRescanner::onResultReadyAt(int index, QFuture<MetadataReturnVal> f)
{
    //qDebug() << "Async Rescan reports result ready at" << index;

    MetadataReturnVal lritem_vec = m_rescan_future_watcher.resultAt(index);

    processReadyResults(lritem_vec);
}

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
															  << lritem_vec.m_new_libentries.size();
															  //<< lritem_vec.m_new_libentries;
		Q_ASSERT_X(0, "Scanning", "Not sure what we got");
	}
}

void LibraryRescanner::onRescanFinished()
{
    auto elapsed = m_timer.elapsed();
	qDebug() << "Async Rescan reports finished.";
	qInfo() << "Directory scan took" << m_last_elapsed_time_dirscan << "ms";
	qInfo() << "Metadata rescan took" << elapsed << "ms";
	// Send out progress text.
	emit progressChanged(0, 0, 0, "Idle");
}

void LibraryRescanner::onDirTravFinished()
{
	qDebug() << "Async Dir Trav reports fisished.";

	// Send out progress text.
	emit progressChanged(0, 0, 0, "Idle");

	/// @todo Should be a lambda.
	///m_current_libmodel->onIncomingFilenamesComplete();
	m_current_libmodel->startRescan();
}


