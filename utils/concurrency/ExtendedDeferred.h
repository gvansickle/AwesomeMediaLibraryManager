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

template <typename WatcherType>
void connect_watcher_disconnects(WatcherType *watcher)
{
	QObject::connect(watcher, &WatcherType::finished,
					 [=]() {
		watcher->disconnect();
		watcher->deleteLater();
	});

	QObject::connect(watcher, &WatcherType::canceled,
					 [=]() {
		watcher->disconnect();
		watcher->deleteLater();
	});
}


/**
 *
 */
template <typename T>
class ExtendedDeferredFuture : public AsyncFuture::Private::DeferredFuture<T>
{
	using BASE_CLASS = AsyncFuture::Private::DeferredFuture<T>;

public:
	ExtendedDeferredFuture(QObject* parent = nullptr): BASE_CLASS(parent)
	{
    }

	~ExtendedDeferredFuture() override
    {
		this->cancel();
    }

	/**
	 * Additions to DeferredFuture<>'s tracking.
	 * The base class' track() implementation is called in only two places:
	 * - At the end of DeferredFuture<>'s "void complete(QFuture<T> future)" overload.
	 * - Deferred<>'s track() simply forwards the call to it.
	 */
    template <typename ANY>
	void track(QFuture<ANY> future)
    {
		qDebug() << "ExtendedDeferredFuture TRACK WAS CALLED";
		QPointer<ExtendedDeferredFuture<T>> thiz = this;
        QFutureWatcher<ANY> *watcher = new QFutureWatcher<ANY>();
		watcher->setObjectName("EDFTrackWatcher");

        if ((QThread::currentThread() != QCoreApplication::instance()->thread()))
        {
            watcher->moveToThread(QCoreApplication::instance()->thread());
        }

        connect_watcher_disconnects(watcher);

//        QObject::connect(watcher, &QFutureWatcher<ANY>::resultReadyAt, [=](int index) {
//                    if (thiz.isNull())
//                    {
//                        return;
//                    }
//					//QFutureInterface<T>::reportResult(future.resultAt(index), index);
//					qDebug() << "resultReadyAt, REPORTING RESULT: INDEX/VALUE:" << index << future.resultAt(index);
//					thiz->reportResult(future.resultAt(index), index);
//                });

        watcher->setFuture(future);

        /// @todo We probably need to check for already-completed work and call reportResults() here.
        if(future.resultCount() > 0)
        {
        	qWarning() << "RESULTS EXISITED BEFORE FUTURE WAS SET. NUM:" << future.resultCount();
        }

		watcher->dumpObjectInfo();
		watcher->dumpObjectTree();
    }

    /**
     * Factory function which creates an ExtendedDeferredFuture instance in a shared pointer.
     */
	static QSharedPointer<ExtendedDeferredFuture<T> > create()
	{
		auto deleter = [](ExtendedDeferredFuture<T> *object) {
			if (object->resolved)
			{
				// If object is already resolved, it is not necessary to keep it in memory
				object->deleteLater();
			}
			else
			{
				object->autoDelete = true;
				object->decRefCount();
			}
		};

		QSharedPointer<ExtendedDeferredFuture<T> > ptr(new ExtendedDeferredFuture<T>(), deleter);
		return ptr;
	}

	/// @name reportResult() overloads.
	/// @{

	template <typename R>
	void reportResult(const R& value, int index = -1)
	{
		qDebug() << "REPORTING RESULT: INDEX/VALUE:" << index << value;
		qDebug() << "SENDER:" << this->sender();
		QFutureInterface<T>::reportResult(value, index);
	}

	/// @}
};

/**
 * Extended subclass of Deferred<T>.  Adds callbacks for resultReadyAt(), resultsReadyAt() signals.
 */
template <typename T, typename DeferredFutureType = ExtendedDeferredFuture<T>>
class ExtendedDeferred : public AsyncFuture::Deferred<T>
{
	using BASE_CLASS = AsyncFuture::Deferred<T>;

public:

	ExtendedDeferred() : AsyncFuture::Deferred<T>()
	{
		/// @todo Not sure if we are leaking a smart_ptr here or not, the Deferred<T>() constructor
		/// has already created a DeferredFuture<T> and put it in deferredFuture.
		QSharedPointer<DeferredFutureType> extdeffut_sp = DeferredFutureType::create();
		//this->deferredFuture = qSharedPointerCast<AsyncFuture::Private::DeferredFuture<T>>(extdeffut_sp);
		this->deferredFuture = extdeffut_sp;
		this->m_future = this->deferredFuture->future();
	}

	void complete(QFuture<QFuture<T>> future)
	{
		qDebug() << "complete(QFuture<QFuture<T>>) called";
		BASE_CLASS::complete(future);
	}

	void complete(QFuture<T> future)
	{
		qDebug() << "complete(QFuture<T>) called";
		BASE_CLASS::complete(future);
		// The base class complete() call above called DeferredFuture's track().
		// Now call our overridden one.
		qSharedPointerDynamicCast<DeferredFutureType>(this->deferredFuture)->track(future);
		qSharedPointerDynamicCast<DeferredFutureType>(this->deferredFuture)->QObject::dumpObjectInfo();
	}

    template <typename Functor>
	typename std::enable_if<std::is_same<typename std::result_of<Functor(int)>::type, void>::value, void>::type
	onResultReadyAt(Functor functor)
	{
    	onResultReadyAt([=](int index) mutable -> bool {
            functor(index);
            return true;
        });
    }

	template <typename Functor>
	typename std::enable_if<std::is_same<typename std::result_of<Functor(int)>::type, bool>::value, void>::type
	onResultReadyAt(Functor onResultReadyAt)
	{
		QFutureWatcher<T> *watcher = new QFutureWatcher<T>();

		auto wrapper = [=](int index) mutable {

			if (!onResultReadyAt(index))
			{
				watcher->disconnect();
				watcher->deleteLater();
			}
		};

		connect_watcher_disconnects(watcher);

		QObject::connect(watcher, &QFutureWatcher<T>::resultReadyAt, wrapper);

		if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
			watcher->moveToThread(QCoreApplication::instance()->thread());
		}

		watcher->setFuture(this->m_future);
	}

    template <typename Functor>
    typename std::enable_if<std::is_same<typename std::result_of<Functor(T, int)>::type, void>::value, void>::type
	onReportResult(Functor functor)
    {
    	onReportResult([=](T value, int index) mutable -> bool {
        	qDebug() << "INDEX/VALUE:" << index << value;
            functor(value, index);
            return true;
        });
    }

	template <typename Functor>
    typename std::enable_if<std::is_same<typename std::result_of<Functor(T, int)>::type, bool>::value, void>::type
	onReportResult(Functor functor)
	{
		QFutureWatcher<T> *watcher = new QFutureWatcher<T>();
		watcher->setObjectName("onReportResultBoolWatcher");

		auto wrapper = [=](int index) mutable {
        	qDebug() << "INDEX/VALUE:" << index << watcher->resultAt(index);
			if (!functor(watcher->resultAt(index), index))
			{
				watcher->disconnect();
				watcher->deleteLater();
			}
		};

		connect_watcher_disconnects(watcher);

		QObject::connect(watcher, &QFutureWatcher<T>::resultReadyAt, wrapper);

		if (QThread::currentThread() != QCoreApplication::instance()->thread())
		{
			watcher->moveToThread(QCoreApplication::instance()->thread());
		}

		watcher->setFuture(this->m_future);
	}

	template <typename ANY>
	void track(QFuture<ANY> future)
	{
		qSharedPointerDynamicCast<DeferredFutureType>(this->deferredFuture)->track(future);
	}

protected:

};

template <typename T>
auto extended_deferred() -> ExtendedDeferred<T>
{
	return ExtendedDeferred<T>();
}


#endif /* UTILS_CONCURRENCY_EXTENDEDDEFERRED_H_ */
