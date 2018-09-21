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

#include "AMLMJob.h"
#include "DirectoryScanJob.h"

#include <QString>
#include <QUrl>
#include <QDirIterator>

/// Ours
#include "utils/TheSimplestThings.h"
#include <logic/DirScanResult.h>
#include <concurrency/ExtAsync.h>

DirectoryScannerAMLMJob::DirectoryScannerAMLMJob(QObject *parent, QUrl dir_url,
                                   const QStringList &nameFilters,
                                   QDir::Filters filters,
                                   QDirIterator::IteratorFlags flags)
    : BASE_CLASS(parent), m_dir_url(dir_url), m_name_filters(nameFilters), m_dir_filters(filters), m_iterator_flags(flags)
{
    // Set our object name.
    setObjectName(uniqueQObjectName());

    setProgressUnit(KJob::Unit::Files);

    // Set our capabilities.
    setCapabilities(KJob::Capability::Killable | KJob::Capability::Suspendable);

    /// @todo Temp, delete.
//    connect_or_die(this, &DirectoryScannerAMLMJob::SIGNAL_resultsReadyAt, this, [=](int begin, int end){
//        qDbo() << "Got signal:" << begin << end;
//    });
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

void DirectoryScannerAMLMJob::DirScanFunction(ExtFuture<DirScanResult> ext_future, AMLMJob* amlmJob,
		const QUrl& dir_url, // The URL pointing at the directory to recursively scan.
		const QStringList &name_filters,
		QDir::Filters dir_filters,
		QDirIterator::IteratorFlags iterator_flags)
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
	if(amlmJob != nullptr)
	{
		amlmJob->setProgressUnit(KJob::Unit::Files);
	}

	int num_files_found_so_far = 0;
	int num_discovered_dirs = 0;
	uint num_possible_files = 0;

	QString status_text = QObject::tr("Scanning for music files");

//	if(amlmJob != nullptr)
//	{
//		Q_EMIT amlmJob->description(amlmJob, status_text,
//								QPair<QString,QString>(QObject::tr("Root URL"), dir_url.toString()),
//								QPair<QString,QString>(QObject::tr("Current file"), QObject::tr("")));
//	}
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
			qWr() << "UNREADABLE FILE:" << file_info.absoluteFilePath();
			/// @todo Collect errors
		}
		else if(file_info.isDir())
		{
			QDir dir = file_info.absoluteDir();
			num_discovered_dirs++;

			// Update the max range to be the number of files we know we've found so far plus the number
			// of files potentially in this directory.
			num_possible_files = num_files_found_so_far + file_info.dir().count();

//            setTotalAmountAndSize(KJob::Unit::Directories, num_discovered_dirs+1);
//            setProcessedAmountAndSize(KJob::Unit::Directories, num_discovered_dirs);
//            setTotalAmountAndSize(KJob::Unit::Files, num_possible_files+1);
			/// NEW
			ext_future.setProgressRange(0, num_possible_files+1);
		}
		else if(file_info.isFile())
		{
			// It's a file.
			num_files_found_so_far++;

			// How big is it?
//            auto file_size = file_info.size();
//            total_discovered_file_size_bytes += file_size;

			QUrl file_url = QUrl::fromLocalFile(entry_path);

			/// @todo
			DirScanResult dir_scan_result(file_url, file_info);
//            qDbo() << "DIRSCANRESULT:" << dir_scan_result;

			ext_future.reportInfoMessage(QObject::tr("File: %1").arg(file_url.toString()), tr("File: %1").arg(file_url.toString()));
//			if(amlmJob != nullptr)
//			{
//				Q_EMIT amlmJob->infoMessage(amlmJob, QObject::tr("File: %1").arg(file_url.toString()), tr("File: %1").arg(file_url.toString()));
//			}

			// Update progress.
			/// @note Bytes is being used for "Size" == progress by the system.
			/// No real need to accumulate that here anyway.
			/// Well, really there is, we could report this as summary info.  Ah well, for tomorrow.
			/// @todo XXXXXXXXXXXXXXXXXXXXXXXXX
//			amlmJob->setTotalAmountAndSize(KJob::Unit::Bytes, total_discovered_file_size_bytes+1);
//			setProcessedAmountAndSize(KJob::Unit::Bytes, total_discovered_file_size_bytes);
			if(amlmJob->totalAmount(KJob::Unit::Files) <= num_files_found_so_far)
			{
				num_possible_files = num_files_found_so_far+1;
//                setTotalAmountAndSize(KJob::Unit::Files, num_possible_files);
				ext_future.setProgressRange(0, num_possible_files);
			}

			amlmJob->setProcessedAmountAndSize(KJob::Unit::Files, num_files_found_so_far);
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
			ext_future.reportCanceled();
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
}

void DirectoryScannerAMLMJob::runFunctor()
{
#if 1
	DirScanFunction(m_ext_future, this,
				m_dir_url, m_name_filters, m_dir_filters, m_iterator_flags);
#else
    // Create the QDirIterator.
    QDirIterator m_dir_iterator(m_dir_url.toLocalFile(), m_name_filters, m_dir_filters, m_iterator_flags);

    // Check for errors.
    QFileInfo file_info(m_dir_url.toLocalFile());
    if(!(file_info.exists() && file_info.isReadable() && file_info.isDir()))
    {
        qWro() << "UNABLE TO READ TOP-LEVEL DIRECTORY:" << m_dir_url;
        qWro() << file_info << file_info.exists() << file_info.isReadable() << file_info.isDir();
//        setSuccessFlag(false);
        return;
    }

    // Count progress in terms of files found.
    setProgressUnit(KJob::Unit::Files);

    int num_files_found_so_far = 0;
    int num_discovered_dirs = 0;
    uint num_possible_files = 0;

    QString status_text = QObject::tr("Scanning for music files");

    Q_EMIT description(this, status_text,
                                QPair<QString,QString>(QObject::tr("Root URL"), m_dir_url.toString()),
                                QPair<QString,QString>(QObject::tr("Current file"), QObject::tr("")));

    m_ext_future.setProgressRange(0, 0);
    m_ext_future.setProgressValueAndText(0, status_text);

    // Iterate through the directory tree.
    while(m_dir_iterator.hasNext())
    {
        Q_ASSERT(!m_i_was_deleted);

        // Go to the next entry and return the path to it.
        QString entry_path = m_dir_iterator.next();
        // Get the QFileInfo for this entry.
        QFileInfo file_info = m_dir_iterator.fileInfo();

        // First check that we have a valid file or dir: Currently exists and is readable by current user.
        if(!(file_info.exists() && file_info.isReadable()))
        {
            qWro() << "UNREADABLE FILE:" << file_info.absoluteFilePath();
        }
        else if(file_info.isDir())
        {
            QDir dir = file_info.absoluteDir();
            num_discovered_dirs++;

            // Update the max range to be the number of files we know we've found so far plus the number
            // of files potentially in this directory.
            num_possible_files = num_files_found_so_far + file_info.dir().count();

//            setTotalAmountAndSize(KJob::Unit::Directories, num_discovered_dirs+1);
//            setProcessedAmountAndSize(KJob::Unit::Directories, num_discovered_dirs);
//            setTotalAmountAndSize(KJob::Unit::Files, num_possible_files+1);
            /// NEW
            m_ext_future.setProgressRange(0, num_possible_files+1);
        }
        else if(file_info.isFile())
        {
            // It's a file.
            num_files_found_so_far++;

            // How big is it?
//            auto file_size = file_info.size();
//            total_discovered_file_size_bytes += file_size;

            QUrl file_url = QUrl::fromLocalFile(entry_path);

            /// @todo
            DirScanResult dir_scan_result(file_url, file_info);
//            qDbo() << "DIRSCANRESULT:" << dir_scan_result;

            Q_EMIT infoMessage(this, QObject::tr("File: %1").arg(file_url.toString()), tr("File: %1").arg(file_url.toString()));

            // Update progress.
            /// @note Bytes is being used for "Size" == progress by the system.
            /// No real need to accumulate that here anyway.
            /// Well, really there is, we could report this as summary info.  Ah well, for tomorrow.
//            setTotalAmountAndSize(KJob::Unit::Bytes, total_discovered_file_size_bytes+1);
//            setProcessedAmountAndSize(KJob::Unit::Bytes, total_discovered_file_size_bytes);
            if(totalAmount(KJob::Unit::Files) <= num_files_found_so_far)
            {
                num_possible_files = num_files_found_so_far+1;
//                setTotalAmountAndSize(KJob::Unit::Files, num_possible_files);
                m_ext_future.setProgressRange(0, num_possible_files);
            }

//            setProcessedAmountAndSize(KJob::Unit::Files, num_files_found_so_far);
            /// NEW
            m_ext_future.setProgressValue(num_files_found_so_far);

			// Report the URL we found to the future.
			m_ext_future.reportResult(dir_scan_result);
        }

        if(functorHandlePauseResumeAndCancel())
        {
            // We've been cancelled.
            qIno() << "CANCELLED";
            m_ext_future.reportCanceled();
            m_ext_future.reportFinished();
            break;
        }
    }

    // We've either completed our work or been cancelled.
	// Either way, run() will handle setting the cancellation status as long as
    // we set success/fail appropriately.
//    if(!wasCancelRequested())
//    {
//        setSuccessFlag(true);
//    }
    num_possible_files = num_files_found_so_far;
    if (!m_ext_future.isCanceled())
    {
        m_ext_future.setProgressRange(0, num_possible_files);
        m_ext_future.setProgressValueAndText(num_files_found_so_far, status_text);
    }

    m_ext_future.reportFinished();

    qDbo() << "RETURNING, ExtFuture:" << m_ext_future; ///< STARTED only, last output of pool thread
#endif
}

