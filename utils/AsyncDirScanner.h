/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include <utils/concurrency/ReportingRunner.h>
#include <utils/DebugHelpers.h>

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
	~AsyncDirScanner() override { qDebug() << "Destructor called"; }

	void run(QFutureInterface<QString>& report_and_control) override
	{
		/// @todo TEST
#if 0
//		QObject signal_source;
		QFutureWatcher<QString> fw;
		fw.connect(&fw, &QFutureWatcher<QString>::progressValueChanged, [=](int b) -> void {
			qDebug() << M_THREADNAME() << "ASYNC PROGRESS SIGNAL: " << b; // << ":" << future.resultAt(b);
		});
		fw.setFuture(report_and_control.future());
		/// @note QFutureWatcher needs to be on either the main thread or maybe a thread with an event loop,
		/// or it seems to never get called and/or signals don't get emitted.
		fw.moveToThread(QCoreApplication::instance()->thread());
#endif
		/// @todo TEST

		QDirIterator m_dir_iterator(m_dir_url.toLocalFile(), m_nameFilters, m_dir_filters, m_iterator_flags);
		int num_files_found_so_far = 0;

		while(m_dir_iterator.hasNext())
		{
			if(report_and_control.isCanceled())
			{
				// We've been cancelled.
				return;
			}

			num_files_found_so_far++;

//			qDebug() << "Found URL:" << m_dir_iterator.filePath();
			QUrl file_url = QUrl::fromLocalFile(m_dir_iterator.next());
//			qDebug() << file_url;

			/// Send this path to the future.
			report_and_control.reportResult(file_url.toString());
			qDebug() << M_THREADNAME() << "resultCount:" << report_and_control.resultCount();
			// Update progress.
			report_and_control.setProgressRange(0, num_files_found_so_far);
			report_and_control.setProgressValue(num_files_found_so_far);
			///control.setProgressValueAndText(num_files_found_so_far, "Hello");
		}
	}

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
