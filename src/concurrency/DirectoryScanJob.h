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

/**
 * This is the actual ThreadWeaver::Job.
 * Does not derive from QObject (or anything else).
 */
class DirectoryScanner : public ThreadWeaver::Job
{
    /// @todo Do we actually need this?
    friend class DirectoryScannerJob;

public:
//    explicit DirectoryScanner(/*ClassDerivedFromTW::Job*/* file);
	explicit DirectoryScanner(const QUrl &dir_url,
            const QStringList &nameFilters,
            QDir::Filters filters = QDir::NoFilter,
            QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags);
	~DirectoryScanner() override;

    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread) override;
    void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
    void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;

    /// Return true if operation succeeded, false if not.
    bool success() const override;

private:

    QUrl m_dir_url;
    QStringList m_nameFilters;
	QDir::Filters m_dir_filters;
	QDirIterator::IteratorFlags m_iterator_flags;
};

/**
 * Decorator to allow a wrapped DirectoryScanner to communicate with the outside world.
 * Inherits from QObject and IdDecorator.
 */
class DirectoryScannerJob : public ThreadWeaver::QObjectDecorator
{
	Q_OBJECT

public:
	explicit DirectoryScannerJob(const QUrl &dir_url,
            const QStringList &nameFilters,
            QDir::Filters filters = QDir::NoFilter,
            QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags,
//            DirectoryScanner* dir_scanner = nullptr,
            QObject* parent = 0)
        : ThreadWeaver::QObjectDecorator(new DirectoryScanner(dir_url, nameFilters, filters, flags),
                                         /*autoDelete?*/ false, parent)
	{}

//	DirectoryScanner* job() { return &m_dir_scanner; }

private:
//	DirectoryScanner m_dir_scanner;
};


#endif /* SRC_CONCURRENCY_DIRECTORYSCANJOB_H_ */
