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

#include <QObject>
#include <QUrl>
#include <QDir>
#include <QDirIterator>

#include <ThreadWeaver/ThreadWeaver>
#include <ThreadWeaver/QObjectDecorator>

#include "AMLMJob.h"

/**
 *
 */
class DirectoryScannerAMLMJob : public AMLMJob
{
    Q_OBJECT

//Q_SIGNALS:
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

public:
    explicit DirectoryScannerAMLMJob(QObject* parent, const QUrl &dir_url,
            const QStringList &nameFilters,
            QDir::Filters filters = QDir::NoFilter,
            QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags);
	~DirectoryScannerAMLMJob() override;

    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread) override;

private:

    QUrl m_dir_url;
    QStringList m_nameFilters;
	QDir::Filters m_dir_filters;
	QDirIterator::IteratorFlags m_iterator_flags;
};




#endif /* SRC_CONCURRENCY_DIRECTORYSCANJOB_H_ */
