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
#include "utils/DebugHelpers.h"

#include <utils/concurrency/ReportingRunner.h>

M_WARNING("TODO: Do this properly");
#define HAVE_TBB 1

#ifdef HAVE_TBB
#include <tbb/tbb.h>
#endif

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

	void run(QFutureInterface<QString>& control) override
	{
		QDirIterator m_dir_iterator(m_dir_url.toLocalFile(), m_nameFilters, m_dir_filters, m_iterator_flags);
		int num_files_found_so_far = 0;

		while(m_dir_iterator.hasNext())
		{
			if(control.isCanceled())
			{
				// We've been cancelled.
				return;
			}

			num_files_found_so_far++;

//			qDebug() << "Found URL:" << m_dir_iterator.filePath();
			QUrl file_url = QUrl::fromLocalFile(m_dir_iterator.next());
//			qDebug() << file_url;

			/// Send this path to the future.
			control.reportResult(file_url.toString());
			// Update progress.
			control.setProgressRange(0, num_files_found_so_far);
			control.setProgressValue(num_files_found_so_far);
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

#if defined(HAVE_TBB)

class AsyncDirScanner_TBB : public tbb::task
{
public:
	AsyncDirScanner_TBB(tbb::concurrent_queue<QString> &output_queue,
						std::function<void(int)> data_func,
						std::function<void()> finished_func,
						const QUrl &dir_url,
						const QStringList &nameFilters,
						QDir::Filters filters = QDir::NoFilter,
						QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags)
				: m_output_queue(output_queue), m_data_func(data_func), m_finished_func(finished_func),
				  m_dir_url(dir_url), m_nameFilters(nameFilters), m_dir_filters(filters), m_iterator_flags(flags)
		{
			// Nothing.
		}
	virtual ~AsyncDirScanner_TBB() { qDebug() << "Destructor called"; }

	void run()
	{

	}

private:
	tbb::task* execute()
	{
		QDirIterator m_dir_iterator(m_dir_url.toLocalFile(), m_nameFilters, m_dir_filters, m_iterator_flags);
		int num_files_found_so_far = 0;

		while(m_dir_iterator.hasNext())
		{
//			if(control.isCanceled())
//			{
//				// We've been cancelled.
//				return;
//			}

			num_files_found_so_far++;

//			qDebug() << "Found URL:" << m_dir_iterator.filePath();
			QUrl file_url = QUrl::fromLocalFile(m_dir_iterator.next());
//			qDebug() << file_url;

			// Send this path to the future.
			m_output_queue.push(file_url.toString());
			// Notify the GUI thread that there's an URL to pick up.
			m_data_func(num_files_found_so_far);
//			control.reportResult(file_url.toString());
			// Update progress.
//			control.setProgressRange(0, num_files_found_so_far);
//			control.setProgressValue(num_files_found_so_far);
			///control.setProgressValueAndText(num_files_found_so_far, "Hello");
		}
		m_finished_func();
	}

private:

	/// Inter-task communication params.
	tbb::concurrent_queue<QString> &m_output_queue;
	std::function<void(int)> &m_data_func;
	std::function<void()> &m_finished_func;

	/// @name Params for QDirIterator.
	///@{
	QUrl m_dir_url;
	QStringList m_nameFilters;
	QDir::Filters m_dir_filters;
	QDirIterator::IteratorFlags m_iterator_flags;
	///@}
};

inline static void LaunchAsyncDirScanner_TBB(tbb::concurrent_queue<QString> &output_queue,
											 std::function<void(int)> data_func,
											 std::function<void()> finished_func,
							   const QUrl &dir_url,
							   const QStringList &nameFilters,
							   QDir::Filters filters = QDir::NoFilter,
							   QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags)
{
	AsyncDirScanner_TBB* t = new(tbb::task::allocate_root()) AsyncDirScanner_TBB(output_queue, data_func, finished_func,
																				 dir_url,
																				 nameFilters,
																				 filters,
																				 flags);
	tbb::task::enqueue(*t);
}

#endif

#endif //AWESOMEMEDIALIBRARYMANAGER_ASYNCDIRSCANNER_H
