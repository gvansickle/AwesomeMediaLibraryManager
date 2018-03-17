/*
 * Copyright 2017, 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef AWESOMEMEDIALIBRARYMANAGER_ASYNCDIRSCANNER_H
#define AWESOMEMEDIALIBRARYMANAGER_ASYNCDIRSCANNER_H


#include <QUrl>
#include <QDirIterator>
#include <QDebug>

#include <concurrency/ReportingRunner.h>

/**
 * Class for asynchronously scanning a directory tree.
 */
class AsyncDirScanner : public ControllableTask<QString>
{
public:
	AsyncDirScanner(const QUrl &dir_url,
	                const QStringList &nameFilters,
	                QDir::Filters filters = QDir::NoFilter,
	                QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags)
			: m_dir_url(dir_url), m_nameFilters(nameFilters), m_dir_filters(filters), m_iterator_flags(flags)
	{
		// Nothing.
	}
	~AsyncDirScanner() override;

	void run(QFutureInterface<QString>& report_and_control) override;

private:

	/// @name Params for QDirIterator.
	///@{
	QUrl m_dir_url;
	QStringList m_nameFilters;
	QDir::Filters m_dir_filters;
	QDirIterator::IteratorFlags m_iterator_flags;
	///@}
};



#endif //AWESOMEMEDIALIBRARYMANAGER_ASYNCDIRSCANNER_H
