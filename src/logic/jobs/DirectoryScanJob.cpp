/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "DirectoryScanJob.h"

// Qt
#include <QString>
#include <QUrl>
#include <QDirIterator>
#include <QPromise>

// Ours
#include <utils/TheSimplestThings.h>
#include <logic/DirScanResult.h>
#include <utils/Stopwatch.h>


void DirScanFunction(QPromise<DirScanResult>& promise,
                     const QUrl& dir_url, // The URL pointing at the directory to recursively scan.
                     const QStringList &name_filters,
		             const QDir::Filters dir_filters,
		             const QDirIterator::IteratorFlags iterator_flags)
{
	Stopwatch sw;
	sw.start("DirScanning");

	if(!dir_url.isLocalFile())
	{
		throw QException();//, "NOT IMPLEMENTED", "dir_url is not a local file");
	}

	// Create the QDirIterator.
	QDirIterator dir_iterator(dir_url.toLocalFile(), name_filters, dir_filters, iterator_flags);

	// Check for errors.
	QFileInfo file_info(dir_url.toLocalFile());
	if(!(file_info.exists() && file_info.isReadable() && file_info.isDir()))
	{
		qWr() << "UNABLE TO READ TOP-LEVEL DIRECTORY:" << dir_url;
		qWr() << file_info << file_info.exists() << file_info.isReadable() << file_info.isDir();
		/// @todo Need to report something here.  Or maybe throw?
		Q_UNIMPLEMENTED();
//        setSuccessFlag(false);
		return;
	}

	// Count progress in terms of files found.
    // M_TODO("FIX THIS FOR QT6/KF6")
//    ext_future.setProgressUnit(KJob::Unit::Files);


	int num_files_found_so_far = 0;
	int num_discovered_dirs = 0;
	uint64_t num_possible_files = 0;
	uint64_t total_discovered_file_size_bytes = 0;

	QString status_text = QObject::tr("Scanning for music files");

    // M_TODO("FIX THIS FOR QT6/KF6")
    // promise.reportDescription(status_text,
    //                           QPair<QString,QString>(QObject::tr("Root URL"), dir_url.toString()),
    //                           QPair<QString,QString>(QObject::tr("Current file"), QObject::tr("")));

	promise.setProgressRange(0, 0);
	promise.setProgressValueAndText(0, status_text);

	// Iterate through the directory tree.
	while(dir_iterator.hasNext())
	{
		// Go to the next entry and return the path to it.
		QString entry_path = dir_iterator.next();
		// Get the QFileInfo for this entry.
		QFileInfo file_info = dir_iterator.fileInfo();

		// First check that we have a valid file or dir: Currently exists and is readable by current user.
		if(!(file_info.exists() && file_info.isReadable()))
		{
			qWr() << "UNREADABLE/NON-EXISTENT FILE:" << file_info.absoluteFilePath();
			/// @todo Collect errors
		}
		else if(file_info.isDir())
		{
			num_discovered_dirs++;

			// Update the max range to be the number of files we know we've found so far plus the number
			// of files potentially in this directory.
			num_possible_files = num_files_found_so_far + file_info.dir().count();

//            setTotalAmountAndSize(KJob::Unit::Directories, num_discovered_dirs+1);
//            setProcessedAmountAndSize(KJob::Unit::Directories, num_discovered_dirs);
//            setTotalAmountAndSize(KJob::Unit::Files, num_possible_files+1);
			promise.setProgressRange(0, num_possible_files + 1);
			promise.setProgressValue(num_files_found_so_far);
		}
		else if(file_info.isFile())
		{
			// It's a file.
			num_files_found_so_far++;

			// How big is it?
            auto file_size = file_info.size();
            total_discovered_file_size_bytes += file_size;

			QUrl file_url = QUrl::fromLocalFile(entry_path);

			/// @todo
			DirScanResult dir_scan_result(file_url, file_info);
//			qDb() << "DIRSCANRESULT:" << dir_scan_result;

            promise.setProgressValueAndText(num_files_found_so_far, QObject::tr("File: %1").arg(file_url.toString()));

			// Update progress.
			/// @note Bytes is being used for "Size" == progress by the system.
			/// No real need to accumulate that here anyway.
			/// Well, really there is, we could report this as summary info.  Ah well, for tomorrow.
			/// @todo XXXXXXXXXXXXXXXXXXXXXXXXX
//			amlmJob->setTotalAmountAndSize(KJob::Unit::Bytes, total_discovered_file_size_bytes+1);
//			setProcessedAmountAndSize(KJob::Unit::Bytes, total_discovered_file_size_bytes);
			if(num_possible_files <= num_files_found_so_far)
			{
				// Keep progress range at least one more than the number of files we've found.
				num_possible_files = num_files_found_so_far+1;
//                setTotalAmountAndSize(KJob::Unit::Files, num_possible_files);
				promise.setProgressRange(0, num_possible_files);
			}
			promise.setProgressValue(num_files_found_so_far);

			// Report the URL we found to the future.
            promise.addResult(dir_scan_result);

            qDb() << "NUM FILES:" << num_files_found_so_far; // << ", per promise:" << promise.resultCount();
		}

		// Have we been canceled?
        promise.suspendIfRequested();
        if(promise.isCanceled())
		{
			// We've been cancelled.
			qIn() << "CANCELLED";
			break;
		}
	}

	// We've either completed our work or been cancelled.
	num_possible_files = num_files_found_so_far;
	if (!promise.isCanceled())
	{
		promise.setProgressRange(0, num_possible_files);
		promise.setProgressValueAndText(num_files_found_so_far, status_text);
	}

	sw.stop();
	sw.print_results();

	// QtConcurrent::run() does the finish for us, so we just return here.
	return;
}

