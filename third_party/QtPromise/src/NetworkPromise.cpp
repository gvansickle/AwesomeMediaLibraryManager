#include "NetworkPromise.h"

namespace QtPromise
{

NetworkPromise::NetworkPromise(QNetworkReply* reply)
	: NetworkPromise(NetworkDeferred::create(reply))
{
}

NetworkPromise::NetworkPromise(NetworkDeferred::Ptr deferred)
	: Promise(deferred)
{
	switch (deferred->state())
	{
	case Deferred::Resolved:
		connect(this, &Promise::resolved, [this, deferred]() {
			Q_EMIT this->resolved(deferred->replyData());
		});
		break;
	case Deferred::Rejected:
		connect(this, &Promise::rejected, [this, deferred]() {
			Q_EMIT this->rejected(deferred->error());
		});
		break;
	case Deferred::Pending:
	default:
		connect(deferred.data(), &NetworkDeferred::resolved, this, &NetworkPromise::resolved);
		connect(deferred.data(), &NetworkDeferred::rejected, this, &NetworkPromise::rejected);
		connect(deferred.data(), &NetworkDeferred::notified, this, &NetworkPromise::notified);
		break;
	}
}

NetworkPromise::Ptr NetworkPromise::create(QNetworkReply* reply)
{
	return Ptr(new NetworkPromise(reply));
}

NetworkPromise::Ptr NetworkPromise::create(NetworkDeferred::Ptr deferred)
{
	return Ptr(new NetworkPromise(deferred));
}

NetworkDeferred::ReplyData NetworkPromise::replyData() const
{
	return m_deferred.staticCast<NetworkDeferred>()->replyData();
}

NetworkDeferred::Error NetworkPromise::error() const
{
	return m_deferred.staticCast<NetworkDeferred>()->error();
}


} /* namespace QtPromise */
