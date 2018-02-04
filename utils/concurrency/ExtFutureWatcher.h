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

#ifndef UTILS_CONCURRENCY_EXTFUTUREWATCHER_H_
#define UTILS_CONCURRENCY_EXTFUTUREWATCHER_H_

#include <QFutureWatcher>

#include <QApplication>
#include <functional>

#include <QDebug>
#include <QThread>
#include "utils/DebugHelpers.h"

template <typename T>
class ExtFutureWatcher : public QFutureWatcher<T>
{
	using BASE_CLASS = QFutureWatcher<T>;

	using OnProgressChangeType = std::function<void(int, int, int)>;
	using OnProgressWithTextChangeType = std::function<void(int, int, int, const QString&)>;
	using OnReportResultType = std::function<void(T, int)>;

public:
	explicit ExtFutureWatcher(QObject *parent = nullptr) : QFutureWatcher<T>(parent), m_utility_thread(new QThread(this))
	{
		qDebug() << "CONSTRUCTOR CALLED WITH PARENT:" << parent;
		m_utility_thread->start();
		this->connect(m_utility_thread, &QThread::finished, &QThread::deleteLater);
	}
	/// @note QFutureWatcher<> is derived from QObject.  QObject has a virtual destructor,
	/// while QFutureWatcher<>'s destructor isn't marked either virtual or override.  By the
	/// rules of C++, QFutureWatcher<>'s destructor is actually virtual ("once virtual always virtual"),
	/// so we're good.  Marking this override to avoid confusion.
	~ExtFutureWatcher() override
	{
		qDebug() << "START: QUIT UTILITY THREAD";
		m_utility_thread->quit();
		qDebug() << "END: QUIT UTILITY THREAD";
	}

	/**
	 * Overload of setFuture() which takes a QFutureInterface<T> instead of a QFuture<T>.
	 */
	ExtFutureWatcher<T>& setFuture(QFutureInterface<T> &future_interface);

	/// @name Signal callback interface.
	/// @{

	/**
	 * Register the given @a on_progress_function callback to be called on any change of
	 * the watched QFutureInterface<>'s progress range or value.
	 */
	ExtFutureWatcher<T>& onProgressChange(OnProgressChangeType &&on_progress_function)
	{
		connectOnProgressCallbacks();
		m_on_progress_callbacks.push_back(on_progress_function);
		return *this;
	}

	/**
	 * Register the given @a on_progress_function callback to be called on any change of
	 * the watched QFutureInterface<>'s progress range, value, or text.
	 */
	ExtFutureWatcher<T>& onProgressChange(OnProgressWithTextChangeType &&on_prog_with_text_change_func)
	{
		connectOnProgressWithTextCallbacks();
		m_on_prog_with_text_callbacks.push_back(on_prog_with_text_change_func);

		// Register the same function to be called by the numeric progress update signals.
		/// @todo This results in the callback always being called twice.
		onProgressChange([=](int min, int val, int max){
			on_prog_with_text_change_func(min, val, max, m_last_progress_text);
			;});
		return *this;
	}

	ExtFutureWatcher<T>& onReportResult(OnReportResultType &&on_report_result_callback)
	{
		connectReportResultCallbacks();
		m_on_report_result_callbacks.push_back(on_report_result_callback);
		return *this;
	}

	/// @}

protected:

	QThread* m_utility_thread;

	void connectOnProgressCallbacks();
	void connectOnProgressWithTextCallbacks();
	void connectReportResultCallbacks();

	/// @name Last known progress state.
	/// These are used to try to reduce the number of emits by only
	/// sending signals when the values have actually changed.
	/// @{
	int m_last_progress_min {-1};
	int m_last_progress_value {-1};
	int m_last_progress_max {-1};
	QString m_last_progress_text;
	/// @}

	/**
	 * Callbacks registered to be called when the progress range or value is updated.
	 */
	std::vector<OnProgressChangeType> m_on_progress_callbacks;

	std::vector<OnProgressWithTextChangeType> m_on_prog_with_text_callbacks;

	std::vector<OnReportResultType> m_on_report_result_callbacks;
};

template <typename T>
inline ExtFutureWatcher<T>& ExtFutureWatcher<T>::setFuture(QFutureInterface<T> &future_interface)
{
	/// @todo Move to a non-GUI thread.
	m_utility_thread->setObjectName("UtilityThread");
	this->moveToThread(m_utility_thread);

	BASE_CLASS::setFuture(future_interface.future());
	return *this;
}

template<typename T>
void ExtFutureWatcher<T>::connectOnProgressCallbacks()
{
	if(m_on_progress_callbacks.empty())
	{
		// Registering the first progress callback, so set up the watcher connections.

		// The progressValueChanged(int) signal.
		QObject::connect(this, &ExtFutureWatcher::progressValueChanged, [=](int new_value){
			qDebug() << M_THREADNAME() << "progressValueChanged";
			if(m_last_progress_value != new_value)
			{
				// Value is different, so we do need to send out a signal.
				m_last_progress_value = new_value;
				if(m_last_progress_max < new_value)
				{
					m_last_progress_max = new_value;
				}
				if(m_last_progress_min == -1)
				{
					// Haven't got a valid min yet for some reason, set it to 0.
					m_last_progress_min = 0;
				}

				// Call all the callbacks.
				for(auto cb : m_on_progress_callbacks)
				{
					cb(m_last_progress_min, m_last_progress_value, m_last_progress_max);
				}
			}
			else
			{
				qDebug() << M_THREADNAME() << "NO CHANGE IN PROGRESS VALUE, NOT EMITTING SIGNALS.";
			}
		;});

		// The progressRangeChanged(int min, int max) signal.
		QObject::connect(this, &ExtFutureWatcher::progressRangeChanged, [=](int min, int max){
			qDebug() << M_THREADNAME() << "progressRangeChanged";
			if(m_last_progress_min != min || m_last_progress_max != max)
			{
				// min or max is different, we need to send out a signal.
				m_last_progress_min = min;
				m_last_progress_max = max;
				if(m_last_progress_value < min)
				{
					m_last_progress_value = min;
				}
				else if(m_last_progress_value > max)
				{
					m_last_progress_value = max;
				}

				// Call all the callbacks.
				for(auto cb : m_on_progress_callbacks)
				{
					cb(m_last_progress_min, m_last_progress_value, m_last_progress_max);
				}
			}
			else
			{
				qDebug() << M_THREADNAME() << "NO CHANGE IN PROGRESS MIN/MAX, NOT EMITTING SIGNALS.";
			}
			;});
	}
}

template<typename T>
void ExtFutureWatcher<T>::connectOnProgressWithTextCallbacks()
{
	if(m_on_prog_with_text_callbacks.empty())
	{
		// Registering the first progress+text callback, so set up the watcher connections.

		QObject::connect(this, &ExtFutureWatcher::progressTextChanged, [=](const QString& str){
			qDebug() << M_THREADNAME() << "progressTextChanged";
			if(m_last_progress_text != str)
			{
				// Text is different, so send out a signal.
				m_last_progress_text = str;

				// Call all the callbacks.
				for(auto cb : m_on_prog_with_text_callbacks)
				{
					cb(m_last_progress_min, m_last_progress_value, m_last_progress_max, m_last_progress_text);
				}
			}
			else
			{
				qDebug() << M_THREADNAME() << "NO CHANGE IN PROGRESS TEXT, NOT EMITTING SIGNALS.";
			}
			;});
	}
}

template<typename T>
void ExtFutureWatcher<T>::connectReportResultCallbacks()
{
	if(m_on_report_result_callbacks.empty())
	{
		// Registering the first OnReportResult callback, so set up the watcher connections.

		QObject::connect(this, &ExtFutureWatcher::resultReadyAt, [=](int index){
			qDebug() << M_THREADNAME() << "resultReadyAt";

			// Get the result from the future.  It's guaranteed to be available.
			T temp_val = this->resultAt(index);

			// Call all the callbacks.
			for(auto cb : m_on_report_result_callbacks)
			{
				cb(temp_val, index);
			}
			;});
	}
}

#endif /* UTILS_CONCURRENCY_EXTFUTUREWATCHER_H_ */
