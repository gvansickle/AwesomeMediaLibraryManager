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


/// Qt5
#include <QVector>
#include <QWeakPointer>
#include <QSharedPointer>

/// Ours
#include "LibraryRescannerMapItem.h"
#include "LibraryRescanner.h" ///< For MetadataReturnVal
#include <concurrency/AMLMJobT.h>

class LibraryModel;
class LibraryRescannerJob;
using LibraryRescannerJobPtr = QPointer<LibraryRescannerJob>;

/*
 *
 */
class LibraryRescannerJob: public AMLMJobT<ExtFuture<MetadataReturnVal>>, public UniqueIDMixin<LibraryRescannerJob>
{
    Q_OBJECT

    using BASE_CLASS = AMLMJobT<ExtFuture<MetadataReturnVal>>;

    /**
     * @note CRTP: Still need this to avoid ambiguous name resolution.
     * @see https://stackoverflow.com/a/46916924
     */
    using UniqueIDMixin<LibraryRescannerJob>::uniqueQObjectName;

Q_SIGNALS:
//    void SLOT_processReadyResults(MetadataReturnVal lritem_vec);

protected:
    explicit LibraryRescannerJob(QObject* parent);

public:

    /// @name Public types
    /// @{
    using ExtFutureType = ExtFuture<MetadataReturnVal>;
    /// @}

    ~LibraryRescannerJob() override;

    static LibraryRescannerJobPtr make_job(QObject *parent);
    static LibraryRescannerJobPtr make_job(QObject *parent, LibraryRescannerMapItem item_to_refresh, const LibraryModel *current_libmodel);
//	static LibraryRescannerJobPtr make_job(QObject *parent, LibraryRescannerMapItem-future-iterators item_to_refresh, const LibraryModel *current_libmodel);

	void run_async_rescan();

	/// The map function for rescanning the library to reload metadata from the files.
	/// Runs in an arbitrary thread context, so must be threadsafe.
	MetadataReturnVal refresher_callback(const VecLibRescannerMapItems& mapitem);

public Q_SLOTS:

    void setDataToMap(QVector<VecLibRescannerMapItems> items_to_rescan, const LibraryModel* current_libmodel);

protected:

    void runFunctor() override;

private:
    Q_DISABLE_COPY(LibraryRescannerJob)

    QVector<VecLibRescannerMapItems> m_items_to_rescan;
    const LibraryModel* m_current_libmodel;
};


void library_metadata_rescan_task(ExtFuture<MetadataReturnVal> ext_future, LibraryRescannerJob* job,
                                  QVector<VecLibRescannerMapItems> items_to_rescan);


Q_DECLARE_METATYPE(LibraryRescannerJobPtr);

#endif /* SRC_LOGIC_LIBRARYRESCANNERJOB_H_ */
