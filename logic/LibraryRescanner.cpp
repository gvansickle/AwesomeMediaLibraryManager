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

// Simon Brunel's QtPromise.
// https://github.com/simonbrunel/qtpromise
#include <QtPromise>
#endif

//#include <utils/concurrency/ExtFutureWatcher.h>
#include <utils/concurrency/ExtAsync.h>
//#include <utils/concurrency/ExtFuture.h>

#include <utils/concurrency/runextensions.h>

#include <utils/DebugHelpers.h>

#include "utils/AsyncDirScanner.h"
#include "utils/concurrency/AsyncTaskManager.h"
#include "utils/concurrency/ReportingRunner.h"
#include "logic/LibraryModel.h"


using std::placeholders::_1;


LibraryRescanner::LibraryRescanner(LibraryModel* parent) : QObject(parent), m_async_task_manager(this)
{
	setObjectName("TheLibraryRescanner");

	// Somewhat redundant, but keep another pointer to the LibraryModel.
	m_current_libmodel = parent;
}

LibraryRescanner::~LibraryRescanner()
{

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

using namespace QtPromise;

QPromise<QString> traverse_dirs(LibraryRescanner *thiz, const QUrl& dir_url, LibraryModel* lib_model)
{
	qDebug() << "ENTER traverse_dirs";
	return QPromise<QString>([&](const QPromiseResolve<QString>& resolve, const QPromiseReject<QString>& reject) {

		QFutureInterface<QString> future_interface = ReportingRunner::runFI(new AsyncDirScanner(dir_url,
													  QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
													  QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories));

		ExtFutureWatcher<QString>* fw = new ExtFutureWatcher<QString>();

		fw->onProgressChange([=](int min, int val, int max, QString text) -> void {
						//qDebug() << M_THREADNAME() << "PROGRESS+TEXT SIGNAL: " << min << val << max << text;
						Q_EMIT thiz->progressChanged(min, val, max, text);
						;})
					.onReportResult([=](QString s, int index) {
						Q_UNUSED(index);
						/// @note This lambda is called in an arbitrary thread context.
						//qDebug() << M_THREADNAME() << "RESULT:" << s << index;

						// This is not threadsafe:
						/// WRONG: this->m_current_libmodel->onIncomingFilename(s);

						// This is threadsafe.
						QMetaObject::invokeMethod(lib_model, "onIncomingFilename", Q_ARG(QString, s));

						/// EXPERIMENTAL
//						static int fail_counter = 0;
//						fail_counter++;
//						if(fail_counter > 5)
//						{
////							future_interface.cancel();
//							//throw QException();
//							qDebug() << "ERROR";
//							return QPromise<QString>::reject("ERROR");
//						}


					})
					.setFuture(future_interface);

		qDebug() << "future is finished:" << fw->future().isFinished() << fw->future().isCanceled();

		QObject::connect(fw, &ExtFutureWatcher<QString>::finished, [=](){
			if (fw->isFinished() && !fw->isCanceled())
			{
				qDebug() << "RESOLVED";
				resolve(fw->future().result());
			}
			else
			{
				qDebug() << "REJECTED";
				reject("Error");
			}

			fw->deleteLater();
		});
	});
	qDebug() << "LEAVE traverse_dirs";
}

void LibraryRescanner::startAsyncDirectoryTraversal(QUrl dir_url)
{
	qDebug() << M_THREADNAME();
	qDebug() << "START:" << dir_url;

	// Time how long it takes.
	m_timer.start();

	// Create the ControlledTask which will scan the directory tree for files.

#if 0 // USE_BUNDLED_SB_QTPROMISE Simon Brunel's qtpromise.
	using namespace QtPromise;

	QFutureInterface<QString> future_interface = ReportingRunner::runFI(new AsyncDirScanner(dir_url,
												  QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
												  QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories));

	QPromise<QString> promise = qPromise(future_interface.future());

	ExtFutureWatcher<QString>* fw = new ExtFutureWatcher<QString>();

	fw->onProgressChange([=](int min, int val, int max, QString text) -> void {
					qDebug() << M_THREADNAME() << "PROGRESS+TEXT SIGNAL: " << min << val << max << text;
					Q_EMIT progressChanged(min, val, max, text);
					;})
				.onReportResult([this, &future_interface](QString s, int index) {
					Q_UNUSED(index);
					/// @note This lambda is called in an arbitrary thread context.
//					qDebug() << M_THREADNAME() << "RESULT:" << s << index;

					// This is not threadsafe:
					/// WRONG: this->m_current_libmodel->onIncomingFilename(s);

//					/// EXPERIMENTAL
//					static int fail_counter = 0;
//					fail_counter++;
//					if(fail_counter > 5)
//					{
////						future_interface.cancel();
//						throw QException();
//					}

					// This is threadsafe.
					QMetaObject::invokeMethod(this->m_current_libmodel, "onIncomingFilename", Q_ARG(QString, s));
				})
				.setFuture(future_interface);

	qDebug() << "future is finished:" << fw->future().isFinished() << "isPending/Fulfilled:" << promise.isPending() << promise.isFulfilled();

	promise.then([&](){
		// promise was fulfilled.
		qDebug() << M_THREADNAME() << "Directory scan complete.";
		m_last_elapsed_time_dirscan = m_timer.elapsed();
		qInfo() << "Directory scan took" << m_last_elapsed_time_dirscan << "ms";
		// Directory traversal complete, start rescan.
		onDirTravFinished();
	}).fail([](const QException& e){
		// dir scanning threw an exception.
		qWarning() << "EXCEPTION:" << e.what();
	}).fail([](){
		// Catch-all fail.
		qWarning() << "FAIL";
		;}); //.wait();

	qDebug() << "future is finished:" << fw->future().isFinished() <<"isPending/Fulfilled:" << promise.isPending() << promise.isFulfilled();

#elif 0 /// QtPromise 2

	traverse_dirs(this, dir_url, this->m_current_libmodel).then([&](){
		// promise was fulfilled.
		qDebug() << M_THREADNAME() << "Directory scan complete.";
		m_last_elapsed_time_dirscan = m_timer.elapsed();
		qInfo() << "Directory scan took" << m_last_elapsed_time_dirscan << "ms";
		// Directory traversal complete, start rescan.
		onDirTravFinished();
	}).fail([](const QException& e){
		// dir scanning threw an exception.
		qWarning() << "EXCEPTION:" << e.what();
	}).fail([](){
		// Catch-all fail.
		qWarning() << "FAIL";
		;});
#elif 1 /// ExtAsync

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

	qDb() << "END:" << dir_url;
}

void LibraryRescanner::cancelAsyncDirectoryTraversal()
{
	m_dirtrav_future.cancel();
}

ExtFuture<QString> LibraryRescanner::AsyncDirectoryTraversal(QUrl dir_url)
{
	qDb() << "START ASYNC";

	ExtFuture<QString> result = ExtAsync::run(this, &LibraryRescanner::SyncDirectoryTraversal, dir_url);

//	m_dirtrav_future = ExtAsync::run(this, &LibraryRescanner::SyncDirectoryTraversal, dir_url);

	qDb() << "RETURNED FROM ExtAsync:" << result;

	result.tap(this, [=](QString str){
		qDb() << "FROM TAP:" << str;
		qDb() << "IN onResultReady CALLBACK:" << result;
		runInObjectEventLoop([=](){ m_current_libmodel->onIncomingFilename(str);}, m_current_libmodel);
	})
	.tap(this, [=](ExtAsyncProgress prog) {
		Q_EMIT this->progressChanged(prog.min, prog.val, prog.max, prog.text);

	;})
	.then(this, [=](ExtFuture<QString> dummy){
		qDb() << "FROM THEN:" << dummy;
		return QString("anotherdummy");
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

		qDb() << "PATH:" << entry_path << "FILEINFO Dir/File:" << file_info.isDir() << file_info.isFile();

		if(file_info.isDir())
		{
			QDir dir = file_info.absoluteDir();
			qDb() << "FOUND DIRECTORY" << dir << " WITH COUNT:" << dir.count();

			// Update the max range to be the number of files we know we've found so far plus the number
			// of files potentially in this directory.
			num_possible_files = num_files_found_so_far + file_info.dir().count();

			future.setProgressRange(0, num_possible_files);
		}
		else if(file_info.isFile())
		{
			// It's a file.
			num_files_found_so_far++;

			qDb() << "ITS A FILE";

			QUrl file_url = QUrl::fromLocalFile(entry_path);

			// Send this path to the future.
			future.reportResult(file_url.toString());

			qDb() << "resultCount:" << future.resultCount();
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
															  // lritem_vec.m_new_libentries;
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
	qDb() << "Async Dir Trav reports fisished.";

	// Send out progress text.
	emit progressChanged(0, 0, 0, "Idle");

	/// @todo Should be a lambda.
	///m_current_libmodel->onIncomingFilenamesComplete();
	m_current_libmodel->startRescan();
}


