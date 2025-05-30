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

/// @file

#include <config.h>

// Qt
#include <QObject>
#include <QUrl>
#include <QDir>
#include <QDirIterator>
// #include <QWeakPointer>
// #include <QSharedPointer>
// #include <QPromise>

// Ours
#include <logic/DirScanResult.h>
#include "concurrency/AMLMJobT.h"
#include <concurrency/ExtFuture.h>
// #include "utils/UniqueIDMixin.h"

/**
 * Worker function which scans a directory for files.
 *
 * @param promise  The in/out/control ExtFuture.
 * @param dir_url     The URL pointing at the directory to recursively scan.
 * @param name_filters
 * @param dir_filters
 * @param iterator_flags
 */
void DirScanFunction(QPromise<DirScanResult>& promise,
                     const QUrl& dir_url,
                     const QStringList &name_filters,
                     const QDir::Filters dir_filters = QDir::NoFilter,
                     const QDirIterator::IteratorFlags iterator_flags = QDirIterator::NoIteratorFlags);

#endif /* SRC_CONCURRENCY_DIRECTORYSCANJOB_H_ */
