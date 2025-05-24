/*
 * Copyright 2018, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef SRC_LOGIC_JOBS_LIBRARYRESCANNERJOB_H_
#define SRC_LOGIC_JOBS_LIBRARYRESCANNERJOB_H_

/// @file

// Qt
#include <QPromise>

// Ours
#include "LibraryRescannerMapItem.h"
#include "LibraryRescanner.h" ///< For MetadataReturnVal
#include "ExtFuture.h"
#include "AMLMJob.h"


class LibraryModel;

/**
 * Worker function which converts the LibraryRescanMapItems from @a in_future
 * to MetadataReturnVal's which are sent to @a promise.
 *
 * @param promise
 * @param in_future
 * @param current_libmodel
 */
void library_metadata_rescan_task(QPromise<MetadataReturnVal>& promise, AMLMJob*,
                                  ExtFuture<VecLibRescannerMapItems> in_future,
                                  LibraryModel* current_libmodel);


#endif /* SRC_LOGIC_JOBS_LIBRARYRESCANNERJOB_H_ */
