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
#include <QList>

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

/**
 * The futureww ("FutureWatcherWatcher") class template.
 */
template <typename T>
class futureww : public QFutureWatcher<T>
{
public:
    explicit futureww(QObject* parent = 0) : QFutureWatcher<T>(parent) {}
    ~futureww()
    {
        cancel();
        waitForFinished();
    }

    explicit futureww(QFuture<T> qfuture) : futureww(qfuture.parent())
	{
		setFuture(qfuture);
	}

    futureww<T>& operator=(QFuture<T> f) { setFuture(f); return *this; };

	/**
     * Attaches a continuation to the futureww.
     * Sort of.  Type has to be ReturnFutureT(QList<T>), return value is reference to this future.
     * @return
	 */
    template <typename ReturnFutureT>
    ReturnFutureT then(std::function<ReturnFutureT(QList<T>)> continuation_function)
	{
        m_continuation_function = continuation_function;
        connect(this, &QFutureWatcher<T>::resultsReadyAt, m_continuation_function);
        return m_continuation_function(future());
	}

    futureww<T>& on_result(std::function<void(int)> result_function)
	{
        m_result_function = result_function;
        connect(this, &QFutureWatcher<T>::resultReadyAt, m_result_function);
        return *this;
	}

	void cancel()
	{
        QFutureWatcher<T>::cancel();
	}

private:

    std::function<void(QList<T>)> m_continuation_function {nullptr};

    std::function<void(int)> m_result_function {nullptr};
};

#endif /* UTILS_CONCURRENCY_ASYNCTASKMANAGER_H_ */
