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

#include "concurrency/AMLMJob.h"
#include "DirectoryScanJob.h"

#include <QString>
#include <QUrl>
#include <QDirIterator>

/// Ours
#include "utils/TheSimplestThings.h"
#include <logic/DirScanResult.h>
#include <concurrency/ExtAsync.h>

DirectoryScannerAMLMJob::DirectoryScannerAMLMJob(QObject *parent, const QUrl& dir_url,
								   const QStringList &nameFilters,
								   const QDir::Filters filters,
								   const QDirIterator::IteratorFlags flags)
    : BASE_CLASS(parent), m_dir_url(dir_url), m_name_filters(nameFilters), m_dir_filters(filters), m_iterator_flags(flags)
{
    // Set our object name.
    setObjectName(uniqueQObjectName());

    setProgressUnit(KJob::Unit::Files);

    // Set our capabilities.
    setCapabilities(KJob::Capability::Killable | KJob::Capability::Suspendable);
}

DirectoryScannerAMLMJob::~DirectoryScannerAMLMJob()
{
//    qDbo() << "DirectoryScannerAMLMJob DELETED:" << this;
}

DirectoryScannerAMLMJobPtr DirectoryScannerAMLMJob::make_job(QObject *parent, const QUrl& dir_url,
                                                             const QStringList &nameFilters,
                                                             QDir::Filters filters,
                                                             QDirIterator::IteratorFlags flags)
{
    auto retval = new DirectoryScannerAMLMJob(parent, dir_url,
                                              nameFilters,
                                              filters,
                                              flags);
    return retval;
}

void DirScanFunction(ExtFuture<DirScanResult> ext_future, AMLMJob* /*amlmJob*/,
		const QUrl& dir_url, // The URL pointing at the directory to recursively scan.
		const QStringList &name_filters,
		const QDir::Filters dir_filters,
		const QDirIterator::IteratorFlags iterator_flags)
{

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
	ext_future.setProgressUnit(KJob::Unit::Files);


	int num_files_found_so_far = 0;
	int num_discovered_dirs = 0;
	uint64_t num_possible_files = 0;
	uint64_t total_discovered_file_size_bytes = 0;

	QString status_text = QObject::tr("Scanning for music files");

	ext_future.reportDescription(status_text,
								 QPair<QString,QString>(QObject::tr("Root URL"), dir_url.toString()),
								 QPair<QString,QString>(QObject::tr("Current file"), QObject::tr("")));

	ext_future.setProgressRange(0, 0);
	ext_future.setProgressValueAndText(0, status_text);

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
			/// @todo Unused, do we need this?
			// QDir dir = file_info.absoluteDir();
			num_discovered_dirs++;

			// Update the max range to be the number of files we know we've found so far plus the number
			// of files potentially in this directory.
			num_possible_files = num_files_found_so_far + file_info.dir().count();

//            setTotalAmountAndSize(KJob::Unit::Directories, num_discovered_dirs+1);
//            setProcessedAmountAndSize(KJob::Unit::Directories, num_discovered_dirs);
//            setTotalAmountAndSize(KJob::Unit::Files, num_possible_files+1);
			ext_future.setProgressRange(0, num_possible_files+1);
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

			ext_future.reportInfoMessage(QObject::tr("File: %1").arg(file_url.toString()), QObject::tr("File: %1").arg(file_url.toString()));

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
				ext_future.setProgressRange(0, num_possible_files);
			}

			/// @todo
//			amlmJob->setProcessedAmountAndSize(KJob::Unit::Files, num_files_found_so_far);
			/// NEW
			ext_future.setProgressValue(num_files_found_so_far);

			// Report the URL we found to the future.
			ext_future.reportResult(dir_scan_result);
		}

		// Have we been canceled?
		if(ext_future.HandlePauseResumeShouldICancel())
		{
			// We've been cancelled.
			qIn() << "CANCELLED";
			// It's already been handled, we'd be reporting it twice here.
//			ext_future.reportCanceled();
			break;
		}
	}

	// We've either completed our work or been cancelled.
	num_possible_files = num_files_found_so_far;
	if (!ext_future.isCanceled())
	{
		ext_future.setProgressRange(0, num_possible_files);
		ext_future.setProgressValueAndText(num_files_found_so_far, status_text);
	}

	ext_future.reportFinished();

	qDb() << "RETURNING, ExtFuture:" << ext_future; ///< STARTED only, last output of pool thread
	return;
}

ExtFuture<DirScanResult> DirectoryScannerAMLMJob::AsyncDirScan(const QUrl& dir_url,
															   const QStringList& name_filters,
															   const QDir::Filters dir_filters,
															   const QDirIterator::IteratorFlags iterator_flags)
{
	return ExtAsync::run(&DirScanFunction, /*AMLMJobT*/nullptr, dir_url, name_filters, dir_filters, iterator_flags);
}

void DirectoryScannerAMLMJob::runFunctor()
{
	DirScanFunction(m_ext_future, this,
				m_dir_url, m_name_filters, m_dir_filters, m_iterator_flags);
}

