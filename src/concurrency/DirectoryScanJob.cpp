#include <utility>

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
#include <ThreadWeaver/DebuggingAids>

/// Ours
#include "utils/TheSimplestThings.h"
#include <logic/DirScanResult.h>
#include <concurrency/ExtAsync.h>

DirectoryScannerAMLMJob::DirectoryScannerAMLMJob(QObject *parent, QUrl dir_url,
                                   const QStringList &nameFilters,
                                   QDir::Filters filters,
                                   QDirIterator::IteratorFlags flags)
    : AMLMJob(parent), m_dir_url(dir_url), m_nameFilters(nameFilters), m_dir_filters(filters), m_iterator_flags(flags)
{
    m_use_extasync = true;

    // Set our object name.
    setObjectName(uniqueQObjectName());

    setProgressUnit(KJob::Unit::Directories);

    // Set our capabilities.
    setCapabilities(KJob::Capability::Killable /*| KJob::Capability::Suspendable*/);
}

DirectoryScannerAMLMJob::~DirectoryScannerAMLMJob()
{
M_WARNING("TODO: There's a problem with shared ptrs here");
qDb() << "DirectoryScannerAMLMJob DELETED:" << this; // << objectName();
}

DirectoryScannerAMLMJobPtr DirectoryScannerAMLMJob::make_job(QObject *parent, QUrl dir_url,
                                                             const QStringList &nameFilters,
                                                             QDir::Filters filters,
                                                             QDirIterator::IteratorFlags flags)
{
    auto retval = new DirectoryScannerAMLMJob(parent, dir_url,
                                              nameFilters,
                                              filters,
                                              flags);
    /// @todo Hook things up in here.

    return retval;
}

void DirectoryScannerAMLMJob::start()
{
    /*ExtFuture<DirScanResult>*/ m_ext_future = ExtAsync::run(this, &DirectoryScannerAMLMJob::work_function);
    qDb() << "ExtFuture<>:" << m_ext_future;
    m_ext_future.then([&](ExtFuture<DirScanResult> extfuture) -> int {
        qDb() << "GOT TO THEN";
        Q_ASSERT(extfuture.isFinished());
        defaultEnd(m_ext_future);
        onUnderlyingAsyncJobDone(!extfuture.isCanceled());
//        KJobCommonDoneOrFailed(!extfuture.isCanceled());
//        emitResult();
        return 1;
        ;});
}

void DirectoryScannerAMLMJob::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_ASSERT(0);

    Q_UNUSED(self);
    Q_UNUSED(thread);
}

bool DirectoryScannerAMLMJob::doKill()
{
    qDb() << "ENTER OVERRIDE DOKILL";
    bool retval = AMLMJob::doKill(m_ext_future);
    qDb() << "EXIT OVERRIDE DOKILL:" << retval;
    return retval;
//    qDb() << "ENTER DOKILL";
//    m_ext_future.cancel();
//    m_ext_future.wait();
//    qDb() << "EXIT DOKILL";
//    return true;
}

void DirectoryScannerAMLMJob::work_function(ExtFuture<DirScanResult> &the_future)
{

    // Create the QDirIterator.
    QDirIterator m_dir_iterator(m_dir_url.toLocalFile(), m_nameFilters, m_dir_filters, m_iterator_flags);

    // Check for errors.
    QFileInfo file_info(m_dir_url.toLocalFile());
    if(!(file_info.exists() && file_info.isReadable() && file_info.isDir()))
    {
        qWr() << "UNABLE TO READ TOP-LEVEL DIRECTORY:" << m_dir_url;
        qWr() << file_info << file_info.exists() << file_info.isReadable() << file_info.isDir();
        setSuccessFlag(false);
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

    the_future.setProgressRange(0, 0);
    the_future.setProgressValueAndText(0, status_text);

    // Iterate through the directory tree.
    while(m_dir_iterator.hasNext())
    {
        if(wasCancelRequested() || the_future.isCanceled())
        {
            // We've been cancelled.
            qIn() << "CANCELLED";
            the_future.reportCanceled();
            break;
        }
        /// @todo Not sure how we'd pause once we get into the TW::run() function.
//        if(the_future.isPaused())
//        {
//            // We're paused, wait for a resume signal.
//            the_future.waitForResume();
//        }

        // Go to the next entry and return the path to it.
        QString entry_path = m_dir_iterator.next();
        // Get the QFileInfo for this entry.
        QFileInfo file_info = m_dir_iterator.fileInfo();

        // First check that we have a valid file or dir: Currently exists and is readable by current user.
        if(!(file_info.exists() && file_info.isReadable()))
        {
            qWr() << "UNREADABLE FILE:" << file_info.absoluteFilePath();
        }
        else if(file_info.isDir())
        {
            QDir dir = file_info.absoluteDir();
            num_discovered_dirs++;

            // Update the max range to be the number of files we know we've found so far plus the number
            // of files potentially in this directory.
            num_possible_files = num_files_found_so_far + file_info.dir().count();

            setTotalAmountAndSize(KJob::Unit::Directories, num_discovered_dirs+1);
            setProcessedAmountAndSize(KJob::Unit::Directories, num_discovered_dirs);
            setTotalAmountAndSize(KJob::Unit::Files, num_possible_files+1);
            /// NEW
            the_future.setProgressRange(0, num_possible_files);
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
            qDb() << "DIRSCANRESULT:" << dir_scan_result;

            Q_EMIT infoMessage(this, QObject::tr("File: %1").arg(file_url.toString()), tr("File: %1").arg(file_url.toString()));

            // Update progress.
            /// @note Bytes is being used for "Size" == progress by the system.
            /// No real need to accumulate that here anyway.
            /// Well, really there is, we could report this as summary info.  Ah well, for tomorrow.
//            setTotalAmountAndSize(KJob::Unit::Bytes, total_discovered_file_size_bytes+1);
//            setProcessedAmountAndSize(KJob::Unit::Bytes, total_discovered_file_size_bytes);
            setProcessedAmountAndSize(KJob::Unit::Files, num_files_found_so_far);
            /// NEW
            the_future.setProgressValueAndText(num_files_found_so_far, status_text);

            // Send the URL we found to the future.  Well, in this case, just Q_EMIT it.
//            Q_EMIT entries(this, file_url);
            Q_EMIT entries(this, dir_scan_result);
        }
    }

    // We've either completed our work or been cancelled.
    // Either way, defaultEnd() will handle setting the cancellation status as long as
    // we set success/fail appropriately.
    if(!(wasCancelRequested() || the_future.isCanceled()))
    {
        setSuccessFlag(true);
    }
    num_possible_files = num_files_found_so_far;
    if (!the_future.isCanceled())
    {
        the_future.setProgressRange(0, num_possible_files);
        the_future.setProgressValueAndText(num_files_found_so_far, status_text);
    }

    /// CHANGE
    the_future.reportFinished();
}
