/*! \file
 *
 * \date Created on: 09.08.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_FUTUREPROMISE_H_
#define QTPROMISE_FUTUREPROMISE_H_

#include "Promise.h"
#include "FutureDeferred.h"

namespace QtPromise
{

/*! \brief A promise to a FutureDeferred.
 *
 * \sa FutureDeferred
 */
class FuturePromise : public Promise
{
	Q_OBJECT

public:

	/*! Smart pointer to a FuturePromise */
	typedef QSharedPointer<FuturePromise> Ptr;

	/*! Creates a FuturePromise for a QFuture.
	 *
	 * \param future The QFuture representing the asynchronous operation.
	 * \return A FuturePromise to a new, pending FutureDeferred for the given
	 * \p future.
	 */
	template<typename T>
	static Ptr create(QFuture<T> future);
	/*! Creates a FuturePromise for a FutureDeferred.
	 *
	 * \param deferred The FutureDeferred which should be represented by a FuturePromise.
	 * \return QSharedPointer to a new FuturePromise for the given \p deferred.
	 *
	 * \sa Promise::create(Deferred::Ptr)
	 */
	static Ptr create(FutureDeferred::Ptr deferred);

	/*! \return The list of results in case this NetworkPromise is resolved.
	 * If this NetworkPromise is rejected, returns the results that have been produced
	 * up to the time when the QFuture was cancelled.
	 * Otherwise, returns an empty list.
	 *
	 * \sa FutureDeferred::rejected()
	 */
	QVariantList results() const;

Q_SIGNALS:
	/*! \copydoc FutureDeferred::resolved()
	 * \sa FutureDeferred::resolved()
	 */
	void resolved(const QVariantList& results) const;
	/*! \copydoc FutureDeferred::rejected()
	 * \sa FutureDeferred::rejected()
	 */
	void rejected(const QVariantList& results) const;
	/*! \copydoc FutureDeferred::notified()
	 * \sa FutureDeferred::notified()
	 */
	void notified(const QtPromise::FutureDeferred::Progress& progress) const;

protected:
	/*! Creates a FutureDeferred for a QFuture and then creates
	 * a FuturePromise for that new FutureDeferred.
	 *
	 * \param future The QFuture to be represented by a FuturePromise.
	 *
	 * \sa NetworkDeferred(QNetworkReply*)
	 */
	template<typename T>
	FuturePromise(QFuture<T> future);
	/*! Creates a FuturePromise for a FutureDeferred.
	 *
	 * \param deferred The FutureDeferred which should be represented by a FuturePromise.
	 */
	FuturePromise(FutureDeferred::Ptr deferred);
};


//####### Template Method Implementation #######
template<typename T>
FuturePromise::FuturePromise(QFuture<T> future)
	: FuturePromise(FutureDeferred::create(future))
{
}

template<typename T>
FuturePromise::Ptr FuturePromise::create(QFuture<T> future)
{
	return Ptr(new FuturePromise(future));
}


} /* namespace QtPromise */


#endif /* QTPROMISE_FUTUREPROMISE_H_ */
