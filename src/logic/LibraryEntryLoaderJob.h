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

#ifndef LIBRARYENTRYLOADERJOB_H
#define LIBRARYENTRYLOADERJOB_H

#include <config.h>

// Std C++
#include <memory>

// Qt5
#include <QObject>
#include <QPersistentModelIndex>
#include <QPointer>

// Ours
#include <concurrency/AMLMJob.h>
#include "LibraryEntry.h"
#include "LibraryRescanner.h" // For MetadataReturnVal


class LibraryEntryLoaderJobResult
{
	Q_GADGET

public:
    LibraryEntryLoaderJobResult() = default;
    LibraryEntryLoaderJobResult(const LibraryEntryLoaderJobResult& other) = default;
    explicit LibraryEntryLoaderJobResult(QPersistentModelIndex pmi, std::shared_ptr<LibraryEntry> le)
    {
        m_original_pindex = pmi;
        m_original_libentry = le;
    }
    ~LibraryEntryLoaderJobResult() = default;

    bool isValid() const
    {
        return (m_new_libentries.size() == m_num_tracks_found)
                && (m_original_libentry);
    }

//protected:
    QPersistentModelIndex m_original_pindex;
    std::shared_ptr<LibraryEntry> m_original_libentry;

    QVector<std::shared_ptr<LibraryEntry>> m_new_libentries;
    int m_num_tracks_found {0};

    void push_back(std::shared_ptr<LibraryEntry> le)
    {
        m_new_libentries.push_back(le);
        m_num_tracks_found++;
    }

//    QTH_FRIEND_QDEBUG_OP(LibraryEntryLoaderJobResult);
//    QTH_FRIEND_QDATASTREAM_OPS(LibraryEntryLoaderJobResult);
};
Q_DECLARE_METATYPE(LibraryEntryLoaderJobResult);
Q_DECLARE_METATYPE(ExtFuture<LibraryEntryLoaderJobResult>);
//QTH_DECLARE_QDEBUG_OP(LibraryEntryLoaderJobResult);
//QTH_DECLARE_QDATASTREAM_OPS(LibraryEntryLoaderJobResult);

class LibraryEntryLoaderJob;
using LibraryEntryLoaderJobPtr = QPointer<LibraryEntryLoaderJob>;

class LibraryEntryLoaderJob : public AMLMJobT<ExtFuture<LibraryEntryLoaderJobResult>>, public UniqueIDMixin<LibraryEntryLoaderJob>
{
    Q_OBJECT

    using BASE_CLASS = AMLMJobT<ExtFuture<LibraryEntryLoaderJobResult>>;

    using UniqueIDMixin<LibraryEntryLoaderJob>::uniqueQObjectName;

//Q_SIGNALS:

protected:
    explicit LibraryEntryLoaderJob(QObject* parent, QPersistentModelIndex pmi, std::shared_ptr<LibraryEntry> libentry);


public:
    /// @name Public types
    /// @{

    using ExtFutureType = ExtFuture<LibraryEntryLoaderJobResult>;

    /// Errors.
    enum
    {
      InvalidQPersistentModelIndex = KJob::UserDefinedError,
      InvalidLibraryEntryURL,
    };

    /// @}

    ~LibraryEntryLoaderJob() override;

    static LibraryEntryLoaderJobPtr make_job(QObject *parent, QPersistentModelIndex pmi, std::shared_ptr<LibraryEntry> libentry);

    /// Overload which makes the AMLMApp singleton the parent.
    static LibraryEntryLoaderJobPtr make_job(QPersistentModelIndex pmi, std::shared_ptr<LibraryEntry> libentry);

protected:

    LibraryEntryLoaderJob* asDerivedTypePtr() override { return this; }

    void runFunctor() override;

private:

    QPersistentModelIndex m_pmi;
    std::shared_ptr<LibraryEntry> m_libentry;
};

#endif // LIBRARYENTRYLOADERJOB_H
