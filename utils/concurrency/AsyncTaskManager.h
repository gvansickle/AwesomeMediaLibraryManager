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

/// @todo Experimental
#include <asyncfuture.h>
#include "ReportingRunner.h"

#include <QtConcurrent>
#include <QObject>
#include <QVector>
#include <QFuture>
#include <QFutureWatcher>
#include <QList>

#include <type_traits>
#include <functional>
#include <algorithm>

class QFutureWatcherBase;

/**
 * The futureww ("FutureWatcherWatcher") class template.
 */
template <typename T>
class futureww : public QFutureWatcher<T>
{
	using BASE_CLASS = QFutureWatcher<T>;

public:
	explicit futureww(QObject* parent = nullptr) : QFutureWatcher<T>(parent) {}
    ~futureww()
    {
		qDebug() << "DESTRUCTOR CALLED";
        cancel();
		BASE_CLASS::waitForFinished();
    }

    futureww(const futureww&) = delete;

//    explicit futureww(QFuture<T> qfuture) : futureww(qfuture.parent())
//	{
//		setFuture(qfuture);
//	}

    /// Assignment operator from QFuture<T>.
	futureww<T>& operator=(QFuture<T> f) { BASE_CLASS::setFuture(f); return *this; }

	/**
     * Attaches a continuation to the futureww.
     * @return Reference to the return value of @a continuation_function.
	 */
    template <typename ReturnFutureT>
    ReturnFutureT& then(std::function<ReturnFutureT(QList<T>)> continuation_function)
	{
        m_continuation_function = std::move(continuation_function);
        connect(this, &QFutureWatcher<T>::resultsReadyAt, m_continuation_function);
		return m_continuation_function(BASE_CLASS::future());
	}

    /**
     * Attaches a "finished" continuation to the future.
     */
    void then(std::function<void()> finished_function)
    {
        m_finished_function = std::move(finished_function);
		QObject::connect(this, &QFutureWatcher<T>::finished, m_finished_function);
    }

	/// @todo Attempt to be more std::experimental.  Incomplete and broken.
	template<typename F>
	QFuture<typename std::result_of<F(QFuture<T>&)>::type>
		then(F&& func)
	{
		return ReportingRunner::run([](QFuture<T>&& fut, F&& func) {
			fut.waitForFinished();
			return std::forward<F>(func)(fut);
		},
		this, std::forward<F>(func)
		);
	}

    futureww<T>& on_resultat(std::function<void(int)> resultat_function)
	{
        m_resultat_function = resultat_function;
        connect(this, &QFutureWatcher<T>::resultReadyAt, m_resultat_function);
        return *this;
	}

    futureww<T>& on_result(std::function<void(T)> result_function)
    {
        m_result_function = std::move(result_function);
		QObject::connect(this, &QFutureWatcher<T>::resultReadyAt, [this](int index){m_result_function(BASE_CLASS::future().resultAt(index));});
        return *this;
    }

    futureww<T>& on_progress(std::function<void(int,int,int)> progress_function)
    {
        m_progress_function = progress_function;
		QObject::connect(this, &QFutureWatcher<T>::progressValueChanged, [this](int val){
    		m_prog_value = val; m_progress_function(m_prog_min, m_prog_max, m_prog_value);
    	});
		QObject::connect(this, &QFutureWatcher<T>::progressRangeChanged, [this](int min, int max){
    		m_prog_min = min;
    		m_prog_max = max;
    		m_progress_function(m_prog_min, m_prog_max, m_prog_value);
    	});
        return *this;
    }

	void cancel()
	{
        QFutureWatcher<T>::cancel();
	}

private:

	int m_prog_min = 0;
	int m_prog_max = 0;
	int m_prog_value = 0;

    std::function<void(QList<T>)> m_continuation_function {nullptr};
    std::function<void()> m_finished_function {nullptr};
    std::function<void()> m_cancelled_function {nullptr};
    std::function<void(int,int,int)> m_progress_function {nullptr};

    std::function<void(int)> m_resultat_function {nullptr};
    std::function<void(T)> m_result_function {nullptr};
};



/*
 *
 */
class AsyncTaskManager: public QObject
{
public:
	AsyncTaskManager(QObject *parent = nullptr);
	virtual ~AsyncTaskManager();

	template <typename T>
	void add_futureww(futureww<T> fww);

	template <typename T>
	void addFuture(const QFuture<T>& future,
			std::function<void(int, QFuture<T>)> on_result,
			std::function<void()> on_finished,
			std::function<void()> on_canceled)
	{
		auto watcher = new QFutureWatcher<T>(this);

		// Make connections.
		connect(watcher, &QFutureWatcher<T>::resultReadyAt, [=](int i){on_result(i, watcher->future());});
		connect(watcher, &QFutureWatcher<T>::finished, on_finished);
		connect(watcher, &QFutureWatcher<T>::canceled, on_canceled);

		watcher->setFuture(future);
		m_future_watchers.push_back(watcher);
	}

private:
	QVector<QFutureWatcherBase*> m_future_watchers;

};


#endif /* UTILS_CONCURRENCY_ASYNCTASKMANAGER_H_ */
