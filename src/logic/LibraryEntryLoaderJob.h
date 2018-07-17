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

// Ours
#include <src/concurrency/AMLMJob.h>
#include "LibraryEntry.h"


class LibraryEntryLoaderJob;
using LibraryEntryLoaderJobPtr = QPointer<LibraryEntryLoaderJob>;

class LibraryEntryLoaderJob : public AMLMJob, public UniqueIDMixin<LibraryEntryLoaderJob>
{
    Q_OBJECT

    using BASE_CLASS = AMLMJob;

    using UniqueIDMixin<LibraryEntryLoaderJob>::uniqueQObjectName;

Q_SIGNALS:
//    void SIGNAL_ImageBytes(QByteArray);

protected:
    explicit LibraryEntryLoaderJob(QObject* parent, QPersistentModelIndex pmi, std::shared_ptr<LibraryEntry> libentry);

public:
    /// @name Public types
    /// @{
    using ExtFutureType = ExtFuture<QByteArray>;
    /// @}

    ~LibraryEntryLoaderJob() override;

    static LibraryEntryLoaderJobPtr make_job(QObject *parent, QPersistentModelIndex pmi, std::shared_ptr<LibraryEntry> libentry);

    QByteArray m_byte_array;

    ExtFutureType& get_extfuture_ref() override { return m_ext_future; }

protected:

    LibraryEntryLoaderJob* asDerivedTypePtr() override { return this; }

    void runFunctor() override;

private:

    ExtFutureType m_ext_future;

};

#endif // LIBRARYENTRYLOADERJOB_H
