/*! \file
 *
 * \date Created on: 02.07.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_FUTUREDEFERRED_H_
#define QTPROMISE_FUTUREDEFERRED_H_

#ifndef QT_NO_QFUTURE

#include <QFutureWatcher>
#include <QTimer>
#include <QAtomicInt>
#include "Deferred.h"

namespace QtPromise
{

/*! \brief Creates a Deferred for a QFuture.
 *
 * A FutureDeferred is resolved when the QFuture finishes and rejected
 * when the QFuture is canceled. In both cases, the data provided will be a
 * QVariantList of the results which have become available.
 * Additionally, the deferred is notified with a Progress object whenever
 * the QFuture's progress changes.
 *
 * \note It is required that the result type of the QFuture (i.e. the type passed
 * as template parameter) is registered in Qt's meta type system using Q_DECLARE_METATYPE
 * since it will be stored in a QVariantList.
 *
 * Similar to NetwordDeferred, FutureDeferred emits its signals when the control
 * returns to the event loop. This ensures that the signals can be handled even if
 * the QFuture is already finished when the FutureDeferred is created.
 *
 * In most cases, it is not necessary to create a FutureDeferred directly but instead
 * use the convenience method FuturePromise::create(const QFuture<T>&) which creates a
 * FutureDeferred and directly returns a promise on it.
 * Creating a FutureDeferred directly is only needed if the deferred should be
 * resolved/rejected/notified independently of the QFuture, which should be
 * a very rare use case.
 *
 * \threadsafeClass
 * \author jochen.ulrich
 * \since 1.1.0
 *
 * \sa FuturePromise
 * \sa \ref page_asyncSignalEmission
 */
class FutureDeferred : public Deferred
{
	Q_OBJECT

public:
	/*! Smart pointer to FutureDeferred. */
	typedef QSharedPointer<FutureDeferred> Ptr;

	/*! Checks for usage errors.
	 *
	 * \sa Deferred::checkDestructionInSignalHandler()
	 */
	virtual ~FutureDeferred();

	/*! Creates a FutureDeferred for a QFuture.
	 *
	 * \tparam T The result type of the \p future.
	 * \param future The QFuture representing the asynchronous operation.
	 * \return QSharedPointer to a new, pending FutureDeferred.
	 */
	template<typename T>
	static Ptr create(const QFuture<T>& future);

	/*! Represents the progress of a download or upload.
	 *
	 * \note This type is registered in Qt's meta type system using
	 * Q_DECLARE_METATYPE() and using qRegisterMetaType() and
	 * QMetaType::registerEqualsComparator() in FutureDeferred().
	 */
	struct Progress
	{
		/*! Minimum value of the progress. */
		int min = -1;
		/*! Maximum value of the progress. */
		int max = -1;
		/*! Current value of the progress. */
		int value = -1;
		/*! Status text of the progress. */
		QString text;

		/*! Compares two Progress objects for equality.
		 *
		 * \param other The Progress object to compare to.
		 * \return \c true if \p min, \p max, \p value and \p text are equal
		 * for \c this and \p other. \c false otherwise.
		 */
		bool operator==(const Progress& other) const
		{
			return min == other.min && max == other.max && value == other.value && text == other.text;
		}
	};

	/*! \return The list of results in case this NetworkDeferred is resolved.
	 * If this NetworkDeferred is rejected, returns the results that have been produced
	 * up to the time when the QFuture was cancelled.
	 * Otherwise, returns an empty list.
	 *
	 * \sa rejected()
	 */
	QVariantList results() const { QMutexLocker locker(&m_lock); return m_results; }

Q_SIGNALS:
	/*! Emitted when the QFuture finishes successfully.
	 *
	 * \param results The results provided by the QFuture.
	 */
	void resolved(const QVariantList& results) const;
	/*! Emitted when the QFuture was canceled.
	 *
	 * \param results The results that have been produced by the QFuture until it was cancelled.
	 * \note This includes only continuous results. Meaning results with index in the range
	 * from 0 to `future.resultCount() - 1`.
	 *
	 * \sa QFuture::resultCount()
	 */
	void rejected(const QVariantList& results) const;
	/*! Emitted when the progress of the QFuture changes.
	 *
	 * \note When using a QFuture from one of the QtConcurrent algorithms,
	 * the notified() signal is emitted twice right when the asynchronous
	 * operation starts. This arises from the behavior of the QFutureWatcher
	 * which first signals a change in the progress range (min and max) when they
	 * are set to the initial values and then signals a change in the progress value
	 * when it is set to the initial value.
	 *
	 * \param progress A FutureDeferred::Progress object.
	 */
	void notified(const QtPromise::FutureDeferred::Progress& progress) const;

protected:
	/*! Creates a FutureDeferred for a given QFuture.
	 *
	 * \tparam T The result type of the \p future.
	 * \param future The QFuture which is represented by the created FutureDeferred.
	 */
	template<typename T>
	FutureDeferred(const QFuture<T>& future);

private Q_SLOTS:
	void futureFinished(const QVariantList& results);
	void futureCanceled(const QVariantList& results);
	void futureProgressRangeChanged(int min, int max);
	void futureProgressTextChanged(const QString& text);
	void futureProgressValueChanged(int value);

private:
	template<typename T>
	static QVariantList resultsFromFuture(const QFuture<T>& future);

	mutable QMutex m_lock;
	QVariantList m_results;
	Progress m_progress;

	static void registerMetaTypes();
};



//####### Template Method Implementation #######
template<typename T>
FutureDeferred::FutureDeferred(const QFuture<T>& future)
{
	registerMetaTypes();

	if (future.isCanceled())
	{
		QTimer::singleShot(0, this, [this, future] {
			this->futureCanceled(resultsFromFuture(future));
		});
	}
	else if (future.isFinished())
	{
		QTimer::singleShot(0, this, [this, future] {
			this->futureFinished(resultsFromFuture(future));
		});
	}
	else
	{
		auto futureWatcher = new QFutureWatcher<T>{this};

		connect(futureWatcher, &QFutureWatcher<T>::finished, [this, future] {
			if (!future.isCanceled())
				this->futureFinished(resultsFromFuture(future));
		});
		connect(futureWatcher, &QFutureWatcher<T>::canceled, [this, future] {
			this->futureCanceled(resultsFromFuture(future));
		});
		connect(futureWatcher, &QFutureWatcher<T>::progressRangeChanged,
		        this, &FutureDeferred::futureProgressRangeChanged);
		connect(futureWatcher, &QFutureWatcher<T>::progressTextChanged,
		        this, &FutureDeferred::futureProgressTextChanged);
		connect(futureWatcher, &QFutureWatcher<T>::progressValueChanged,
		        this, &FutureDeferred::futureProgressValueChanged);

		futureWatcher->setFuture(future);
	}

}

template<typename T>
FutureDeferred::Ptr FutureDeferred::create(const QFuture<T>& future)
{
	return Ptr(new FutureDeferred(future));
}

template<typename T>
QVariantList FutureDeferred::resultsFromFuture(const QFuture<T>& future)
{
	/* future.results() does *NOT* return the achieved results
	 * after the future has been cancelled.
	 * So we get the results "manually" .
	 */
	QVariantList results;
	const int resultCount = future.resultCount();
	for (int i=0; i < resultCount; ++i)
		results << future.resultAt(i);
	return results;
}

} /* namespace QtPromise */

Q_DECLARE_METATYPE(QtPromise::FutureDeferred::Progress)


#endif /* QT_NO_QTFUTURE */

#endif /* QTPROMISE_FUTUREDEFERRED_H_ */
