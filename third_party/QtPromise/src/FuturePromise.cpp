#include "FuturePromise.h"

namespace QtPromise
{


FuturePromise::FuturePromise(FutureDeferred::Ptr deferred)
	: Promise(deferred)
{
	switch (deferred->state())
	{
	case Deferred::Resolved:
		connect(this, &Promise::resolved, [this, deferred]() {
			Q_EMIT this->resolved(deferred->results());
		});
		break;
	case Deferred::Rejected:
		connect(this, &Promise::rejected, [this, deferred]() {
			Q_EMIT this->rejected(deferred->results());
		});
		break;
	case Deferred::Pending:
	default:
		connect(deferred.data(), &FutureDeferred::resolved, this, &FuturePromise::resolved);
		connect(deferred.data(), &FutureDeferred::rejected, this, &FuturePromise::rejected);
		connect(deferred.data(), &FutureDeferred::notified, this, &FuturePromise::notified);
		break;
	}
}

FuturePromise::Ptr FuturePromise::create(FutureDeferred::Ptr deferred)
{
	return Ptr(new FuturePromise(deferred));
}

QVariantList FuturePromise::results() const
{
	return m_deferred.staticCast<FutureDeferred>()->results();
}


} /* namespace QtPromise */
