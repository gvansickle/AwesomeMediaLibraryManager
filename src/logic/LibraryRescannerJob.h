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

#ifndef SRC_LOGIC_LIBRARYRESCANNERJOB_H_
#define SRC_LOGIC_LIBRARYRESCANNERJOB_H_

#include <src/concurrency/AMLMJob.h>

/// Qt5
#include <QVector>

/// Ours
#include "LibraryRescannerMapItem.h"
#include "LibraryRescanner.h" ///< For MetadataReturnVal

class LibraryModel;
class LibraryRescannerJob;
using LibraryRescannerJobPtr = QPointer<LibraryRescannerJob>;

/*
 *
 */
class LibraryRescannerJob: public AMLMJob, public UniqueIDMixin<LibraryRescannerJob>
{
    Q_OBJECT

    using BASE_CLASS = AMLMJob;

    /**
     * @note CRTP: Still need this to avoid ambiguous name resolution.
     * @see https://stackoverflow.com/a/46916924
     */
    using UniqueIDMixin<LibraryRescannerJob>::uniqueQObjectName;

Q_SIGNALS:


public:
    explicit LibraryRescannerJob(QObject* parent);
	~LibraryRescannerJob() override;

    LibraryRescannerJobPtr setDataToMap(QVector<VecLibRescannerMapItems> items_to_rescan, LibraryModel* current_libmodel);

public Q_SLOTS:
    void processReadyResults(MetadataReturnVal lritem_vec);


protected:

    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread) override;

    /// The map function for rescanning the library to reload metadata from the files.
    /// Runs in an arbitrary thread context, so must be threadsafe.
    MetadataReturnVal refresher_callback(const VecLibRescannerMapItems& mapitem);

private:
    Q_DISABLE_COPY(LibraryRescannerJob)

    QVector<VecLibRescannerMapItems> m_items_to_rescan;
    LibraryModel* m_current_libmodel;
};

#endif /* SRC_LOGIC_LIBRARYRESCANNERJOB_H_ */
