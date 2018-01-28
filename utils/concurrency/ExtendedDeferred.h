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

#ifndef UTILS_CONCURRENCY_EXTENDEDDEFERRED_H_
#define UTILS_CONCURRENCY_EXTENDEDDEFERRED_H_

#include "asyncfuture.h"

/**
 * Extended subclass of Deferred<T>.  Adds callbacks for resultReadyAt(), resultsReadyAt() signals.
 */
template <typename T>
class ExtendedDeferred : public AsyncFuture::Deferred<T>
{
	using BASE_CLASS = AsyncFuture::Deferred<T>;

public:

	ExtendedDeferred() : AsyncFuture::Deferred<T>() {}

	void complete(QFuture<T> future)
	{
		qDebug() << "complete called";
		m_watched_future = future;
		BASE_CLASS::complete(future);
	}

	template <typename Functor>
	void onResultReadyAt(Functor onResultReadyAt)
	{
		QFutureWatcher<T> *watcher = new QFutureWatcher<T>();

		auto wrapper = [=](int index) mutable {

			if (!onResultReadyAt(index)) {
				watcher->disconnect();
				watcher->deleteLater();
			}
		};

		QObject::connect(watcher, &QFutureWatcher<T>::finished,
						 [=]() {
			watcher->disconnect();
			watcher->deleteLater();
		});

		QObject::connect(watcher, &QFutureWatcher<T>::canceled,
						 [=]() {
			watcher->disconnect();
			watcher->deleteLater();
		});

		QObject::connect(watcher, &QFutureWatcher<T>::resultReadyAt, wrapper);

		if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
			watcher->moveToThread(QCoreApplication::instance()->thread());
		}

		watcher->setFuture(m_watched_future); ////this->m_future);
	}

	void reportResult(T value, int index)
	{
		qDebug() << "####################################### value/index:" << value << index;
		BASE_CLASS::deferredFuture->reportResult(value, index);
	}

protected:
	QFuture<T> m_watched_future;
};


#endif /* UTILS_CONCURRENCY_EXTENDEDDEFERRED_H_ */
