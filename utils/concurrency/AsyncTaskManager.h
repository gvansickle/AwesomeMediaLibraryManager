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

#ifndef UTILS_CONCURRENCY_ASYNCTASKMANAGER_H_
#define UTILS_CONCURRENCY_ASYNCTASKMANAGER_H_

#include <QObject>
#include <QVector>
#include <QFuture>
#include <QFutureWatcher>
#include <functional>

class QFutureWatcherBase;

/*
 *
 */
class AsyncTaskManager: public QObject
{
public:
	AsyncTaskManager(QObject *parent = 0);
	virtual ~AsyncTaskManager();

	template <typename T>
	void addFuture(const QFuture<T>& future,
			std::function<void()> on_results,
			std::function<void()> on_finished,
			std::function<void()> on_canceled)
	{
		auto watcher = new QFutureWatcher<T>(this);

		// Make connections.
		connect(watcher, &QFutureWatcher<T>::resultsReadyAt, on_results);
		connect(watcher, &QFutureWatcher<T>::finished, on_finished);
		connect(watcher, &QFutureWatcher<T>::canceled, on_canceled);

		watcher->setFuture(future);
		m_future_watchers.push_back(watcher);
	}

private:
	QVector<QFutureWatcherBase*> m_future_watchers;

};

#endif /* UTILS_CONCURRENCY_ASYNCTASKMANAGER_H_ */
