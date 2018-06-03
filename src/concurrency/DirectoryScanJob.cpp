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

#include "utils/TheSimplestThings.h"

DirectoryScannerAMLMJob::DirectoryScannerAMLMJob(QObject *parent, const QUrl &dir_url,
                                   const QStringList &nameFilters,
                                   QDir::Filters filters,
                                   QDirIterator::IteratorFlags flags) : AMLMJob(parent)
{
    // Set our object name.
    setObjectName(uniqueQObjectName());

    // Should be in the constructor list, but we're deferring to another constructor.
    // Jeez this language.....
    m_dir_url = dir_url;
    m_nameFilters = nameFilters;
    m_dir_filters = filters;
    m_iterator_flags = flags;

    // Set our capabilities.
    setCapabilities(KJob::Capability::Killable /*| KJob::Capability::Suspendable*/);
}

DirectoryScannerAMLMJob::~DirectoryScannerAMLMJob()
{
M_WARNING("TODO: There's a problem with shared ptrs here");
    qDb() << "DirectoryScannerAMLMJob DELETED:" << this; // << objectName();
}

void DirectoryScannerAMLMJob::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
	// Per the instructions here: https://api.kde.org/frameworks/threadweaver/html/classThreadWeaver_1_1Job.html#a1dd5d0ec7e1953576d6fe787f3cfd30c
	// "Whenever publishing information about the job to the outside world, for example by emitting signals, use self,
	// not this. self is the reference counted object handled by the queue. Using it as signal parameters will amongst
	// other things prevent thejob from being memory managed and deleted."

    /// @warning self: TW:JobPointer is a QSharedPtr<TW::JobInterface>, which inherits from nothing, especially not QObject.
    ///          So "T qobject_cast(QObject *object)" in particular won't work here.

    qDb() << "IN RUN, self/self.data():" << self << self.data() << "TW self Status:" << self->status();
    qDb() << "IN RUN, this:" << this;

    // This unfortunate dance is needed to get a QPointer (which is really a QWeakPointer) to a dynamically-casted
    // AMLMJob, while not losing/screwing up the ref counts.  Hopefully.

M_WARNING("TODO");
//    DirectoryScannerAMLMJobPtr dirscanjob_self = qSharedPointerDynamicCast<DirectoryScannerAMLMJob>(self);//= qSharedPtrToQPointerDynamicCast<AMLMJob>(self);
    DirectoryScannerAMLMJobPtr dirscanjob_self = qSharedPtrToQPointerDynamicCast<DirectoryScannerAMLMJob>(self);

    setProgressUnit(KJob::Unit::Directories);

    qDb() << "IN RUN, " << M_NAME_VAL(dirscanjob_self);
    Q_CHECK_PTR(dirscanjob_self);

    // Create the QDirIterator.
	QDirIterator m_dir_iterator(m_dir_url.toLocalFile(), m_nameFilters, m_dir_filters, m_iterator_flags);

    // Check for errors.
    QFileInfo file_info(m_dir_url.toLocalFile());
    if(!(file_info.exists() && file_info.isReadable() && file_info.isDir()))
    {
        qWr() << "UNABLE TO READ TOP-LEVEL DIRECTORY:" << m_dir_url;
        qWr() << file_info << file_info.exists() << file_info.isReadable() << file_info.isDir();
        dirscanjob_self->setSuccessFlag(false);
        dirscanjob_self->setWasCancelled(false);
        return;
    }


    int num_files_found_so_far = 0;
    int num_discovered_dirs = 0;
    uint num_possible_files = 0;
    qint64 total_discovered_file_size_bytes = 0;

    QString status_text = QObject::tr("Scanning for music files");

    Q_EMIT description(this, status_text,
                                QPair<QString,QString>(QObject::tr("Root URL"), m_dir_url.toString()),
                                QPair<QString,QString>(QObject::tr("Current file"), QObject::tr("")));

    // Iterate through the directory tree.
    while(m_dir_iterator.hasNext())
    {
        if(dirscanjob_self->wasCancelRequested())
        {
            // We've been cancelled.
            qIn() << "CANCELLED";
            break;
        }
        /// @todo Not sure how we'd pause once we get into the TW::run() function.
//        if(report_and_control.isPaused())
//        {
//            // We're paused, wait for a resume signal.
//            report_and_control.waitForResume();
//        }

        // Go to the next entry and return the path to it.
        QString entry_path = m_dir_iterator.next();
        // Get the QFileInfo for this entry.
        auto file_info = m_dir_iterator.fileInfo();

//            qDebug() << "PATH:" << entry_path << "FILEINFO Dir/File:" << file_info.isDir() << file_info.isFile();

        // First check that we have a valid file or dir: Currently exists and is readable by current user.
        if(!(file_info.exists() && file_info.isReadable()))
        {
            qWr() << "UNREADABLE FILE:" << file_info.absoluteFilePath();
        }
        else if(file_info.isDir())
        {
            QDir dir = file_info.absoluteDir();
            num_discovered_dirs++;

//                qDebug() << "FOUND DIRECTORY" << dir << " WITH COUNT:" << dir.count();

            // Update the max range to be the number of files we know we've found so far plus the number
            // of files potentially in this directory.
            num_possible_files = num_files_found_so_far + file_info.dir().count();

            dirscanjob_self->setTotalAmountAndSize(KJob::Unit::Directories, num_discovered_dirs+1);
            dirscanjob_self->setProcessedAmountAndSize(KJob::Unit::Directories, num_discovered_dirs);
            dirscanjob_self->setTotalAmountAndSize(KJob::Unit::Files, num_possible_files+1);
        }
        else if(file_info.isFile())
        {
            // It's a file.
            num_files_found_so_far++;

            // How big is it?
            auto file_size = file_info.size();
            total_discovered_file_size_bytes += file_size;

            QUrl file_url = QUrl::fromLocalFile(entry_path);

            Q_EMIT infoMessage(this, QObject::tr("File: %1").arg(file_url.toString()), tr("File: %1").arg(file_url.toString()));

            // Send the URL we found to the future.  Well, in this case, just Q_EMIT it.
            Q_EMIT entries(this, file_url);

            // Update progress.
            /// @note Bytes is being used for "Size" == progress by the system.
            /// No real need to accumulate that here anyway.
            /// Well, really there is, we could report this as summary info.  Ah well, for tomorrow.
//            setTotalAmountAndSize(KJob::Unit::Bytes, total_discovered_file_size_bytes+1);
//            setProcessedAmountAndSize(KJob::Unit::Bytes, total_discovered_file_size_bytes);
            dirscanjob_self->setProcessedAmountAndSize(KJob::Unit::Files, num_files_found_so_far);
        }
    }

    // We've either completed our work or been cancelled.
    // Either way, defaultEnd() will handle setting the cancellation status as long as
    // we set success/fail appropriately.

    qDb() << "LEAVING RUN";
}




