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

#ifndef UTILS_CONCURRENCY_EXTASYNCTASK_H_
#define UTILS_CONCURRENCY_EXTASYNCTASK_H_

/// Based on this Stack Overflow reply: https://stackoverflow.com/a/16729619

/**
 * Subclass this to get a controllable task which can be passed to ReportingRunner::run().
 * Note that this class does not derive from QObject (or any other class for that matter).
 * @tparam T  The type returned in the QFuture<T>.
 */
template <typename T, typename FutureTemplateType>
class ExtAsyncTask
{
public:
	virtual ~ExtAsyncTask();

	/**
	 * Override this in your derived class to do the long-running work.
	 * Periodically check @a control->isCanceled() and return if it returns false.
	 * Send results out via one of the control->reportResult() overloads.
	 *
	 * @note Fun Fact about progress reporting: It's all too easy to flood the GUI thread
	 * with progress updates unless you know one non-obvious thing about QFutureWatcher<>'s
	 * progressValueChanged() implementation.  From the Qt5 docs (http://doc.qt.io/qt-5/qfuturewatcher.html):
	 *
	 * "In order to avoid overloading the GUI event loop, QFutureWatcher limits the progress signal emission rate.
	 *  This means that listeners connected to this slot might not get all progress reports the future makes.
	 *  The last progress update (where ***progressValue equals the maximum value***) will always be delivered."
	 *
	 * Emphasis mine.  That means don't do what I did and set the progress range max equal to the progressValue when
	 * you don't know a priori what the range max is (e.g. during a directory traversal).
	 * If you do so, the GUI slows to a crawl.
	 */
	virtual void run(FutureTemplateType<T>& control) = 0;
};

template<class T, typename FutureTemplateType>
ExtAsyncTask<T, FutureTemplateType>::~ExtAsyncTask()
{
	// Nothing.
}

#endif /* UTILS_CONCURRENCY_EXTASYNCTASK_H_ */
