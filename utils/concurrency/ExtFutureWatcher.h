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

#include <QDebug>

template <typename T>
class ExtFutureWatcher : public QFutureWatcher<T>
{
	using BASE_CLASS = QFutureWatcher<T>;

public:
	explicit ExtFutureWatcher(QObject *parent = nullptr) : QFutureWatcher<T>(parent)
	{
		qDebug() << "CONSTRUCTOR CALLED WITH PARENT:" << parent;
	}
	/// @note QFutureWatcher<> is derived from QObject.  QObject has a virtual destructor,
	/// while QFutureWatcher<>'s destructor isn't marked either virtual or override.  By the
	/// rules of C++, QFutureWatcher<>'s destructor is actually virtual ("once virtual always virtual"),
	/// so we're good.  Marking this override to avoid confusion.
	~ExtFutureWatcher() override = default;

	/**
	 * Overload of setFuture() which takes a QFutureInterface<T> instead of a QFuture<T>.
	 */
	void setFuture(QFutureInterface<T> &future_interface);
};

template <typename T>
inline void ExtFutureWatcher<T>::setFuture(QFutureInterface<T> &future_interface)
{
	BASE_CLASS::setFuture(future_interface.future());
}

#endif /* UTILS_CONCURRENCY_EXTFUTUREWATCHER_H_ */
