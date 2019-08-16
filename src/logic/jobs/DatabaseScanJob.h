/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/**
 * @file DatabaseScanJob.h
 */
#ifndef SRC_LOGIC_JOBS_DATABASESCANJOB_H_
#define SRC_LOGIC_JOBS_DATABASESCANJOB_H_

// Std C++
#include <memory>

// Qt5
#include <QObject>

// Ours
class ScanResultsTreeModel;
#include <utils/Stopwatch.h>

/*
 *
 */
class DatabaseScanJob
{
public:

	static void startAsyncDirectoryTraversal_ForDB(const QUrl& dir_url, std::shared_ptr<ScanResultsTreeModel> srtm);

private:
	Stopwatch m_timer;

};

#endif /* SRC_LOGIC_JOBS_DATABASESCANJOB_H_ */
