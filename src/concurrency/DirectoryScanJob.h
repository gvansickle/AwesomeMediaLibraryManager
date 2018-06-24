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

#ifndef SRC_CONCURRENCY_DIRECTORYSCANJOB_H_
#define SRC_CONCURRENCY_DIRECTORYSCANJOB_H_

#include <config.h>

/// Qt5
#include <QObject>
#include <QUrl>
#include <QDir>
#include <QDirIterator>
#include <QWeakPointer>
#include <QSharedPointer>

/// KF5
#include <ThreadWeaver/ThreadWeaver>

#include <logic/DirScanResult.h>
#include "AMLMJob.h"
#include <concurrency/ExtFuture.h>
#include "utils/UniqueIDMixin.h"

class DirectoryScannerAMLMJob;
using DirectoryScannerAMLMJobPtr = QPointer<DirectoryScannerAMLMJob>;

/**
 *
 */
class DirectoryScannerAMLMJob : public AMLMJob, public UniqueIDMixin<DirectoryScannerAMLMJob>
{
    Q_OBJECT

    using BASE_CLASS = AMLMJob;

    /**
     * @note CRTP: Still need this to avoid ambiguous name resolution.
     * @see https://stackoverflow.com/a/46916924
     */
    using UniqueIDMixin<DirectoryScannerAMLMJob>::uniqueQObjectName;

Q_SIGNALS:
//    /// ThreadWeaver::QObjectDecorator signals, only three:
//    /*
//    *  // This signal is emitted when this job is being processed by a thread.
//    *  void started(ThreadWeaver::JobPointer);
//    *  // This signal is emitted when the job has been finished (no matter if it succeeded or not).
//    *  void done(ThreadWeaver::JobPointer);
//    *  // This signal is emitted when success() returns false after the job is executed.
//    *  void failed(ThreadWeaver::JobPointer);
//    */
//    void started(ThreadWeaver::JobPointer);
//    void done(ThreadWeaver::JobPointer);
//    void failed(ThreadWeaver::JobPointer);

    /**
     * KIO::ListJob-like signal used to send the discovered directory entries to
     * whoever may be listening.
     * Differs in the parameters.  KIO::Job is a KJob here, and the UDSEntryList
     * is not a list or a UDSEntry, but a single QUrl.
     */
//    void entries(KJob* kjob, const QUrl& url);
    void entries(KJob* kjob, const DirScanResult& url);

public:
    explicit DirectoryScannerAMLMJob(QObject* parent, QUrl dir_url,
            const QStringList &nameFilters,
            QDir::Filters filters = QDir::NoFilter,
            QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags);

    ~DirectoryScannerAMLMJob() override;

    static DirectoryScannerAMLMJobPtr make_job(QObject *parent, QUrl dir_url,
                                               const QStringList &nameFilters,
                                               QDir::Filters filters,
                                               QDirIterator::IteratorFlags flags);

    /**
     * "Subclasses must implement start(), which should trigger the execution of the job (although the work should be done
     *  asynchronously)."
     *
     * @note Per comments, KF5 KIO::Jobs autostart; this is overridden to be a no-op.
     */
    Q_SCRIPTABLE void start() override;

protected:

    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread) override;

private:

    void work_function(ExtFuture<DirScanResult>& the_future);
    ExtFuture<DirScanResult> m_ext_future;

    QUrl m_dir_url;
    QStringList m_nameFilters;
    QDir::Filters m_dir_filters;
    QDirIterator::IteratorFlags m_iterator_flags;

};

Q_DECLARE_METATYPE(DirectoryScannerAMLMJobPtr);


#endif /* SRC_CONCURRENCY_DIRECTORYSCANJOB_H_ */
