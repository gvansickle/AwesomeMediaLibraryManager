#include "Promise.h"
#include "ChildDeferred.h"
#include <QTimer>
#include <QHash>

namespace QtPromise {

Promise::Promise(Deferred::Ptr deferred)
	: QObject(), m_deferred(deferred)
{
	switch (m_deferred->state())
	{
	case Deferred::Resolved:
		QTimer::singleShot(0, this, [this]() {
			Q_EMIT resolved(this->m_deferred->data());
		});
		break;
	case Deferred::Rejected:
		QTimer::singleShot(0, this, [this]() {
			Q_EMIT rejected(this->m_deferred->data());
		});
		break;
	case Deferred::Pending:
	default:
		connect(m_deferred.data(), &Deferred::resolved, this, &Promise::resolved);
		connect(m_deferred.data(), &Deferred::rejected, this, &Promise::rejected);
		connect(m_deferred.data(), &Deferred::notified, this, &Promise::notified);
		break;
	}
}

Promise::Promise(Deferred::State state, const QVariant& data)
	: Promise(Deferred::create(state, data))
{
}

Promise::Ptr Promise::create(Deferred::Ptr deferred)
{
	return Ptr(new Promise(deferred));
}

Promise::Ptr Promise::createResolved(const QVariant& value)
{
	return Ptr(new Promise(Deferred::Resolved, value));
}

Promise::Ptr Promise::createRejected(const QVariant& reason)
{
	return Ptr(new Promise(Deferred::Rejected, reason));
}

Promise::Ptr Promise::delayedResolve(const QVariant& value, int delayInMillisec)
{
	Deferred::Ptr deferred = Deferred::create();
	Deferred* rawDeferred = deferred.data();
	QTimer::singleShot(delayInMillisec, rawDeferred, [rawDeferred, value]() {
		rawDeferred->resolve(value);
	});
	return Promise::create(deferred);
}

Promise::Ptr Promise::delayedReject(const QVariant& reason, int delayInMillisec)
{
	Deferred::Ptr deferred = Deferred::create();
	Deferred* rawDeferred = deferred.data();
	QTimer::singleShot(delayInMillisec, rawDeferred, [rawDeferred, reason]() {
		rawDeferred->reject(reason);
	});
	return Promise::create(deferred);
}


Deferred::State Promise::state() const
{
	return m_deferred->state();
}

QVariant Promise::data() const
{
	return m_deferred->data();
}



}  // namespace QtPromise


uint qHash(const QtPromise::Promise::Ptr& promisePtr, uint seed)
{
	return qHash(promisePtr.data(), seed);
}

