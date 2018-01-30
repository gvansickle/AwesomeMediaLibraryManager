/*! \file
 *
 * \date Created on: 17.02.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_NETWORKPROMISE_H_
#define QTPROMISE_NETWORKPROMISE_H_

#include "Promise.h"
#include "NetworkDeferred.h"

namespace QtPromise
{

/*! \brief A promise to a NetworkDeferred.
 *
 * \sa NetworkDeferred
 */
class NetworkPromise : public Promise
{
	Q_OBJECT

public:

	/*! Smart pointer to a NetworkPromise */
	typedef QSharedPointer<NetworkPromise> Ptr;

	/*! Creates a NetworkPromise for a QNetworkReply.
	 *
	 * \param reply The QNetworkReply performing the transmission.
	 * \note The internally created NetworkDeferred takes ownership of the
	 * \p reply by becoming its parent. If you don't want this, change the
	 * \p reply's parent after calling create() and ensure the \p reply
	 * exists as long as the NetworkDeferred is pending.
	 * \return A NetworkPromise to a new, pending NetworkDeferred for the given
	 * \p reply.
	 *
	 * \sa NetworkDeferred::create(QNetworkReply*)
	 */
	static Ptr create(QNetworkReply* reply);
	/*! Creates a NetworkPromise for a NetworkDeferred.
	 *
	 * \param deferred The NetworkDeferred which should be represented by a NetworkPromise.
	 * \return QSharedPointer to a new NetworkPromise for the given \p deferred.
	 *
	 * \sa Promise::create(Deferred::Ptr)
	 */
	static Ptr create(NetworkDeferred::Ptr deferred);

	/*! \return The data in case this NetworkPromise was already resolved or an empty ReplyData
	 * object otherwise.
	 *
	 * \sa NetworkDeferred::ReplyData()
	 */
	NetworkDeferred::ReplyData replyData() const;

	/*! \return The error in case this NetworkPromsie was already rejected or an empty Error
	 * object otherwise.
	 *
	 * \sa NetworkDeferred::Error()
	 */
	NetworkDeferred::Error error() const;

Q_SIGNALS:
	/*! \copydoc NetworkDeferred::resolved()
	 * \sa NetworkDeferred::resolved()
	 */
	void resolved(const NetworkDeferred::ReplyData& data) const;
	/*! \copydoc NetworkDeferred::rejected()
	 * \sa NetworkDeferred::rejected()
	 */
	void rejected(const NetworkDeferred::Error& reason) const;
	/*! \copydoc NetworkDeferred::notified()
	 * \sa NetworkDeferred::notified()
	 */
	void notified(const NetworkDeferred::ReplyProgress& progress) const;

protected:
	/*! Creates a NetworkDeferred for a QNetworkReply and
	 * then creates a NetworkPromise for that new NetworkDeferred.
	 *
	 * \param reply The QNetworkReply to be represented by a NetworkPromise.
	 * \note The internal NetworkDeferred takes ownership of the \p reply.
	 *
	 * \sa NetworkDeferred(QNetworkReply*)
	 */
	NetworkPromise(QNetworkReply* reply);
	/*! Creates a NetworkPromise for a NetworkDeferred.
	 *
	 * \param deferred The NetworkDeferred which should be represented by a NetworkPromise.
	 */
	NetworkPromise(NetworkDeferred::Ptr deferred);
};

} /* namespace QtPromise */


#endif /* QTPROMISE_NETWORKPROMISE_H_ */
