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

#include "utils/DebugHelpers.h"

//DirectoryScannerAMLMJob::DirectoryScannerAMLMJob(QObject *parent) : BASE_CLASS(parent)
//{
//    setObjectName(uniqueQObjectName());//"DirectoryScannerAMLMJob");
//    qDb() << "STAGE 1 CONSTRUCTOR COMPLETE";
//}

DirectoryScannerAMLMJob::DirectoryScannerAMLMJob(QObject *parent, const QUrl &dir_url,
                                   const QStringList &nameFilters,
                                   QDir::Filters filters,
                                   QDirIterator::IteratorFlags flags)
    : AMLMJob(parent) //< Get our vtable set up. @todo STILL NEED THIS???
{
M_WARNING("TODO");

    // Should be in the constructor list, but we're deferring to another constructor.
    // Jeez this language.....
    m_dir_url = dir_url;
    m_nameFilters = nameFilters;
    m_dir_filters = filters;
    m_iterator_flags = flags;


    // Set our capabilities.
    setCapabilities(KJob::Capability::Killable /*| KJob::Capability::Suspendable*/);
}

DirectoryScannerAMLMJobPtr DirectoryScannerAMLMJob::make_shared(QObject *parent, const QUrl &dir_url,
                                    const QStringList &nameFilters,
                                    QDir::Filters filters,
                                    QDirIterator::IteratorFlags flags)
{
M_WARNING("TODO: NOT SHARED PTR");
//    return DirectoryScannerAMLMJobPtr::create(parent, dir_url, nameFilters, filters, flags);
    return new DirectoryScannerAMLMJob(parent, dir_url, nameFilters, filters, flags);
}

DirectoryScannerAMLMJob::~DirectoryScannerAMLMJob()
{
    qDb() << "DESTRUCTOR:" << objectName();
}

void DirectoryScannerAMLMJob::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
	// Per the instructions here: https://api.kde.org/frameworks/threadweaver/html/classThreadWeaver_1_1Job.html#a1dd5d0ec7e1953576d6fe787f3cfd30c
	// "Whenever publishing information about the job to the outside world, for example by emitting signals, use self,
	// not this. self is the reference counted object handled by the queue. Using it as signal parameters will amongst
	// other things prevent thejob from being memory managed and deleted."

    qDb() << "IN RUN, self/->:" << self << self.data() << "TW self Status:" << self->status();
    qDb() << "IN RUN, this:" << this;

    AMLMJobPtr amlm_self = dynamic_cast<AMLMJobPtr>(self.data());//qSharedPointerDynamicCast<AMLMJob>(self);
    qDb() << "IN RUN, " << M_NAME_VAL(amlm_self);
    Q_CHECK_PTR(amlm_self);
    KJob* kselfsp = amlm_self;
    qDb() << "IN RUN, " << M_NAME_VAL(kselfsp);
    Q_CHECK_PTR(kselfsp);

//    amlm_self->setAutoDelete(false);
    qDb() << "IN RUN, KJob isAutoDelete()?:" << amlm_self->isAutoDelete();


	QDirIterator m_dir_iterator(m_dir_url.toLocalFile(), m_nameFilters, m_dir_filters, m_iterator_flags);

    int num_files_found_so_far = 0;
    int num_discovered_dirs = 0;
    uint num_possible_files = 0;
    qint64 total_discovered_file_size_bytes = 0;

    QString status_text = QObject::tr("Scanning for music files");

    Q_EMIT amlm_self->description(kselfsp, status_text,
                                QPair<QString,QString>(QObject::tr("Root URL"), m_dir_url.toString()),
                                QPair<QString,QString>(QObject::tr("Current file"), QObject::tr("")));

//		report_and_control.setProgressRange(0, 0);
//		report_and_control.setProgressValueAndText(0, status_text);
        //setPercent(0);

		while(m_dir_iterator.hasNext())
		{
            if(amlm_self->twWasCancelRequested())
            {
                // We've been cancelled.
                qIn() << "CANCELLED";
                break;
            }
//			if(report_and_control.isPaused())
//			{
//				// We're paused, wait for a resume signal.
//				report_and_control.waitForResume();
//			}

			// Go to the next entry and return the path to it.
			QString entry_path = m_dir_iterator.next();
			auto file_info = m_dir_iterator.fileInfo();

//            qDebug() << "PATH:" << entry_path << "FILEINFO Dir/File:" << file_info.isDir() << file_info.isFile();

			if(file_info.isDir())
			{
				QDir dir = file_info.absoluteDir();
                num_discovered_dirs++;

//                qDebug() << "FOUND DIRECTORY" << dir << " WITH COUNT:" << dir.count();

				// Update the max range to be the number of files we know we've found so far plus the number
				// of files potentially in this directory.
				num_possible_files = num_files_found_so_far + file_info.dir().count();

                setProcessedAmount(KJob::Unit::Directories, num_discovered_dirs);
                setTotalAmount(KJob::Unit::Directories, num_discovered_dirs+1);
                setTotalAmount(KJob::Unit::Files, num_possible_files+1);
//				report_and_control.setProgressRange(0, num_possible_files);
			}
			else if(file_info.isFile())
			{
				// It's a file.
				num_files_found_so_far++;

                // How big is it?
                auto file_size = file_info.size();
                total_discovered_file_size_bytes += file_size;

//                qDebug() << "ITS A FILE";

				QUrl file_url = QUrl::fromLocalFile(entry_path);

                Q_EMIT infoMessage(kselfsp, QObject::tr("File: %1").arg(file_url.toString()), tr("RICH File: %1").arg(file_url.toString()));

				// Send this path to the future.
//				report_and_control.reportResult(file_url.toString());

//                qDebug() << M_THREADNAME() << "resultCount:" << report_and_control.resultCount();
				// Update progress.
//				report_and_control.setProgressValueAndText(num_files_found_so_far, status_text);
                setTotalAmount(KJob::Unit::Bytes, total_discovered_file_size_bytes+1);
                setProcessedAmount(KJob::Unit::Bytes, total_discovered_file_size_bytes);
                setProcessedAmount(KJob::Unit::Files, num_files_found_so_far);
			}
		}

		// We're done.  One last thing to clean up: We need to send the now-known max range out.
		// Then we need to send out the final progress value again, because it might have been throttled away
		// by Qt.
//		num_possible_files = num_files_found_so_far;
//		if (!report_and_control.isCanceled())
//		{
//			report_and_control.setProgressRange(0, num_possible_files);
//			report_and_control.setProgressValueAndText(num_files_found_so_far, status_text);
        //		}

//        qDb() << "EMITTING RESULT";

//        aself->asKJob()->emitResult();

        amlm_self->setSuccessFlag(true);

        qDb() << "LEAVING RUN";
}




