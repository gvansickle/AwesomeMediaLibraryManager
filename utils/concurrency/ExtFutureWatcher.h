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

#include <functional>
#include <limits> // For numerical_limits.

#include <QApplication>
#include <QDebug>
#include <QThread>

#include "utils/DebugHelpers.h"

/**
 * An extended QFutureWatcher<> class.
 *
 * @note This class by default lives in its own thread, unlike QFutureWatcher<>, which lives in the
 *       thread it's created in.  Among other considerations, that means that the callbacks registered with it
 *       will be called in this ExtFutureWatcher's thread context.
 */
template <typename T>
class ExtFutureWatcher : public QFutureWatcher<T>
{
	using BASE_CLASS = QFutureWatcher<T>;

public:

	using OnProgressWithTextChangeType = std::function<void(int, int, int, QString)>;
	using OnReportResultType = std::function<void(T, int)>;

	explicit ExtFutureWatcher(QObject *parent = nullptr) : QFutureWatcher<T>(parent), m_utility_thread(new QThread(/*no parent*/))
	{
		// @note We don't give the QThread ourselves as a parent, so that it doesn't get deleted
		// before we do.  Since we'll be running in that thread, that's an important safety tip.
		if(parent != nullptr)
		{
			// If we're created with a parent, we can't moveToThread() later.
			qWarning() << "CONSTRUCTOR CALLED WITH NON-NULL PARENT:" << parent;
		}
		// Give ourselves a default name.
		this->setObjectName("ExtFutureWatcher");
		// Give the utility thread a name.
		m_utility_thread->setObjectName("UtilityThread");
		// Connect the finished signal to the deleteLater() slot.
		QObject::connect(m_utility_thread, &QThread::finished, &QThread::deleteLater);
		m_utility_thread->start();
	}

	/// @note QFutureWatcher<> is derived from QObject.  QObject has a virtual destructor,
	/// while QFutureWatcher<>'s destructor isn't marked either virtual or override.  By the
	/// rules of C++, QFutureWatcher<>'s destructor is actually virtual ("once virtual always virtual"),
	/// so we're good.  Marking this override to avoid confusion.
	~ExtFutureWatcher() override
	{
		// I have no idea if this is the right way to go about this.
		// It's definitely too much to be doing in a destructor, but hey, this is Qt, when in Rome....
		// Move ourselves off the utility thread to the main thread, so we can wait for the utility thread to quit.
		// Note: Doesn't seem to make any difference as long as we don't wait(), and use deleteLater().
		//this->moveToThread(QApplication::instance()->thread());
		qDebug() << "QUITTING UTILITY THREAD";
		m_utility_thread->quit();
		// Note: Waiting here seems to be useless.  We don't seem to have been moved to the main thread
		// by the time we get here, and we can't wait on our own threac.
		//qDebug() << "WAITING FOR UTILITY THREAD TO QUIT";
		//m_utility_thread->wait();
		qDebug() << "DELETELATER utilitythread";
		m_utility_thread->deleteLater();
	}

	/**
	 * Overload of setFuture() which takes a QFutureInterface<T> instead of a QFuture<T>.
	 */
	ExtFutureWatcher<T>& setFuture(QFutureInterface<T> &future_interface);

	/// @name Signal callback interface.
	/// @{

	/**
	 * Register the given @a on_progress_function callback to be called on any change of
	 * the watched QFutureInterface<>'s progress range, value, or text.
	 *
	 * @note The callback will run in this ExtFutureWatcher<>'s thread context.
	 */
	ExtFutureWatcher<T>& onProgressChange(OnProgressWithTextChangeType &&on_prog_with_text_change_func)
	{
		connectAllOnProgressCallbacks();
		m_on_prog_with_text_callbacks.push_back(on_prog_with_text_change_func);
		return *this;
	}

	/**
	 * Register the given @a on_progress_function callback to be called on any change of
	 * the watched QFutureInterface<>'s progress range, value, or text.
	 *
	 * @note The callback will run in this ExtFutureWatcher<>'s thread context.
	 */
	ExtFutureWatcher<T>& onReportResult(OnReportResultType &&on_report_result_callback)
	{
		connectReportResultCallbacks();
		m_on_report_result_callbacks.push_back(on_report_result_callback);
		return *this;
	}

	template <typename F>
	ExtFutureWatcher<T>& then(QObject* context, F&& func)
	{
		QObject::connect(this, &ExtFutureWatcher<T>::finished, context, [=](){
			func();
		});
		return *this;
	}

	template <typename F>
	ExtFutureWatcher<T>& then(F&& func)
	{
		return then(QApplication::instance(), std::forward<F>(func));
	}

	/// @}

protected /*slots*/: // Template, can't have "real" slots.

	void onProgressRangeChanged(int min, int max);
	void onProgressTextChanged(const QString& progressText);
	void onProgressValueChanged(int new_value);


protected:

	/// The thread in which the callbacks will be executed.
	QThread* m_utility_thread;

	void connectAllOnProgressCallbacks();

	/// Called from within an onProgress*Changed() slot when the last know progress state changes.
	void callAllProgressCallbacks();

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
	 * Callbacks registered to be called when the progress range, value, or text is updated.
	 * These callbacks will only be called if the progress actually did change.
	 */
	std::vector<OnProgressWithTextChangeType> m_on_prog_with_text_callbacks;

	/**
	 * Callbacks registered to be called back when a result is ready.
	 */
	std::vector<OnReportResultType> m_on_report_result_callbacks;
};

template <typename T>
inline ExtFutureWatcher<T>& ExtFutureWatcher<T>::setFuture(QFutureInterface<T> &future_interface)
{
	// Move ourselves to the utility thread.
	this->moveToThread(m_utility_thread);

	BASE_CLASS::setFuture(future_interface.future());
	return *this;
}

template<typename T>
void ExtFutureWatcher<T>::onProgressRangeChanged(int min, int max)
{
	if(m_last_progress_min != min || m_last_progress_max != max)
	{
		// min or max actually did change, so we need to update the last state and
		// call the registered callbacks.
		m_last_progress_min = min;
		m_last_progress_max = max;
		// Clamp progress value between min and max.
		// Would prefer to use std::clamp() here.  Sadly, that's a C++17 function.  Is it really the 21st century, I ask you?
		m_last_progress_value = qBound(m_last_progress_min, m_last_progress_value, m_last_progress_max);

		// Call all the onProgress callbacks.
		callAllProgressCallbacks();
	}
}

template<typename T>
void ExtFutureWatcher<T>::onProgressTextChanged(const QString& progressText)
{
	if(m_last_progress_text != progressText)
	{
		// Progress text actually did change, so we need to update the last state and
		// call the registered callbacks.
		m_last_progress_text = progressText;

		// Make sure last min/max/value aren't still undefined (i.e. ==-1).
		// Default min and/or max to 0 if they are, then default value == clamp(min, val, max);
		m_last_progress_min = qBound(0, m_last_progress_min, std::numeric_limits<int>::max());
		m_last_progress_max = qBound(0, m_last_progress_max, std::numeric_limits<int>::max());
		m_last_progress_value = qBound(m_last_progress_min, m_last_progress_value, m_last_progress_max);

		// Call all the callbacks.
		// Call all the onProgress callbacks.
		callAllProgressCallbacks();
	}
}

template<typename T>
void ExtFutureWatcher<T>::onProgressValueChanged(int new_value)
{
	if(m_last_progress_value != new_value)
	{
		// Progress value actually did change, so we need to update the last state and
		// call the registered callbacks.
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

		// Call all the onProgress callbacks.
		callAllProgressCallbacks();
	}
}

template<typename T>
void ExtFutureWatcher<T>::connectAllOnProgressCallbacks()
{
	if(m_on_prog_with_text_callbacks.empty())
	{
		// No callbacks have been registered yet, which means we haven't connected the progress signals up yet.

		// Connect up this's three progress-related callbacks.
		// Note that Qt5 can't send signals from a class template.  Luckily we have a non-template base-base class,
		// so we can just do this cast and send from there.
		QFutureWatcherBase * fwb = qobject_cast<QFutureWatcherBase*>(this);
		QObject::connect(fwb, &QFutureWatcherBase::progressValueChanged,
						 this, &ExtFutureWatcher<T>::onProgressValueChanged);
		QObject::connect(fwb, &QFutureWatcherBase::progressRangeChanged,
						 this, &ExtFutureWatcher<T>::onProgressRangeChanged);
		QObject::connect(fwb, &QFutureWatcherBase::progressTextChanged,
						 this, &ExtFutureWatcher<T>::onProgressTextChanged);
	}
}

template<typename T>
void ExtFutureWatcher<T>::callAllProgressCallbacks()
{
	// Call all the onProgress callbacks.
//	qDebug() << M_THREADNAME() << "callAllProgressCallbacks";
	for(auto cb : m_on_prog_with_text_callbacks)
	{
		cb(m_last_progress_min, m_last_progress_value, m_last_progress_max, m_last_progress_text);
	}
}

template<typename T>
void ExtFutureWatcher<T>::connectReportResultCallbacks()
{
	if(m_on_report_result_callbacks.empty())
	{
		// Registering the first OnReportResult callback, so set up the watcher connections.
		/// @note The second "this" is the context pointer for the lambda, which does two things:
		/// 1. Disconnects this connection when this is destroyed.
		/// 2. Determines whether the connection will be queued or not.
		QObject::connect(this, &ExtFutureWatcher::resultReadyAt, this, [=](int index){
			//qDebug() << M_THREADNAME() << "resultReadyAt";

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
