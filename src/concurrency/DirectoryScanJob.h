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

// Qt5
#include <QObject>
#include <QUrl>
#include <QDir>
#include <QDirIterator>
#include <QWeakPointer>
#include <QSharedPointer>

// Ours
#include <logic/DirScanResult.h>
#include "AMLMJob.h"
#include <concurrency/ExtFuture.h>
#include "utils/UniqueIDMixin.h"

class DirectoryScannerAMLMJob;
using DirectoryScannerAMLMJobPtr = QPointer<DirectoryScannerAMLMJob>;


/**
 *
 */
class DirectoryScannerAMLMJob : public AMLMJobT<ExtFuture<DirScanResult>>, public UniqueIDMixin<DirectoryScannerAMLMJob>
{
    Q_OBJECT

	using BASE_CLASS = AMLMJobT<ExtFuture<DirScanResult>>;

    /**
     * @note CRTP: Still need this to avoid ambiguous name resolution.
     * @see https://stackoverflow.com/a/46916924
     */
    using UniqueIDMixin<DirectoryScannerAMLMJob>::uniqueQObjectName;

Q_SIGNALS:

    /**
     * KIO::ListJob-like signal used to send the discovered directory entries to
     * whoever may be listening.
     */
    void entries(DirScanResult dsr, KJob* kjob);

	void SIGNAL_resultsReadyAt(ExtFuture<DirScanResult>& ef, int begin, int end);

protected:
    explicit DirectoryScannerAMLMJob(QObject* parent, QUrl dir_url,
            const QStringList &nameFilters,
            QDir::Filters filters = QDir::NoFilter,
            QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags);

public:

    /// @name Public types
    /// @{
    using ExtFutureType = ExtFuture<DirScanResult>;
    /// @}

    ~DirectoryScannerAMLMJob() override;

	static DirectoryScannerAMLMJobPtr make_job(QObject *parent, const QUrl& dir_url,
											   const QStringList &nameFilters,
											   QDir::Filters filters,
											   QDirIterator::IteratorFlags flags);


    ExtFutureType& get_extfuture_ref() override { return m_ext_future; }

protected:

    DirectoryScannerAMLMJob* asDerivedTypePtr() override { return this; }

    void runFunctor() override;

protected Q_SLOT:

	 void SLOT_onResultsReadyAt(ExtFutureType& ef, int begin, int end) override
	 {
		 qDbo() << "GOT RESULTS:" << begin << end;

         Q_EMIT SIGNAL_resultsReadyAt(ef, begin, end);
	 }

private:

    /// The URL we'll start the traversal from.
    QUrl m_dir_url;
    QStringList m_name_filters;
    QDir::Filters m_dir_filters;
    QDirIterator::IteratorFlags m_iterator_flags;

};

Q_DECLARE_METATYPE(DirectoryScannerAMLMJobPtr);

#endif /* SRC_CONCURRENCY_DIRECTORYSCANJOB_H_ */
