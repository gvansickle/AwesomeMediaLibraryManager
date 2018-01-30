#include "Deferred.h"

#include <QHash>

namespace QtPromise {


Deferred::Deferred()
	: QObject(nullptr)
	, m_state(Pending)
	, m_lock(QMutex::Recursive)
	, m_isInSignalHandler{0}
{
	registerMetaTypes();
}

void Deferred::registerMetaTypes()
{
	static QMutex metaTypesLock;
	static bool registered = false;

	QMutexLocker locker(&metaTypesLock);
	if (!registered)
	{
		qRegisterMetaType<State>();
		QMetaType::registerEqualsComparator<State>();
		qRegisterMetaType<State>("Deferred::State");
		qRegisterMetaType<State>("QtPromise::Deferred::State");
		registered = true;
	}
}

Deferred::Ptr Deferred::create()
{
	return Deferred::Ptr(new Deferred());
}

Deferred::Ptr Deferred::create(State state, const QVariant& data)
{
	Deferred::Ptr deferred = create();
	switch (state)
	{
	case Rejected:
		deferred->reject(data);
		break;
	case Resolved:
	default:
		deferred->resolve(data);
		break;
	}
	return deferred;
}


Deferred::~Deferred()
{
	checkDestructionInSignalHandler();

	QMutexLocker locker(&m_lock);
	if (m_state == Pending)
		qDebug("Deferred %s destroyed while still pending", qUtf8Printable(pointerToQString(this)));
}

void Deferred::setLogInvalidActionMessage(bool logInvalidActionMessage)
{
	m_logInvalidActionMessage = logInvalidActionMessage;
}

void Deferred::logInvalidActionMessage(const char* action) const
{
	if (m_logInvalidActionMessage)
		qDebug("Cannot %s Deferred %s which is already %s", action, qUtf8Printable(pointerToQString(this)), m_state==Resolved?"resolved":"rejected");
}

void Deferred::checkDestructionInSignalHandler()
{
	if (m_isInSignalHandler.fetchAndStoreOrdered(0) > 0)
		qCritical("Deferred %s destroyed as reaction to its own signal", qUtf8Printable(pointerToQString(this)));
}

bool Deferred::resolve(const QVariant& value)
{
	QMutexLocker locker(&m_lock);

	if (m_state == Pending)
	{
		m_data = value;
		m_state = Resolved;
		m_isInSignalHandler.fetchAndAddAcquire(1);
		Q_EMIT resolved(m_data);
		m_isInSignalHandler.fetchAndSubRelease(1);
		return true;
	}
	else
	{
		logInvalidActionMessage("resolve");
		return false;
	}
}

bool Deferred::reject(const QVariant& reason)
{
	QMutexLocker locker(&m_lock);

	if (m_state == Pending)
	{
		m_data = reason;
		m_state = Rejected;
		m_isInSignalHandler.fetchAndAddAcquire(1);
		Q_EMIT rejected(m_data);
		m_isInSignalHandler.fetchAndSubRelease(1);
		return true;
	}
	else
	{
		logInvalidActionMessage("reject");
		return false;
	}
}

bool Deferred::notify(const QVariant& progress)
{
	QMutexLocker locker(&m_lock);

	if (m_state == Pending)
	{
		m_isInSignalHandler.fetchAndAddAcquire(1);
		Q_EMIT notified(progress);
		m_isInSignalHandler.fetchAndSubRelease(1);
		return true;
	}
	else
	{
		logInvalidActionMessage("notify");
		return false;
	}
}

}  // namespace QtPromise


uint qHash(const QtPromise::Deferred::Ptr& deferredPtr, uint seed)
{
	return qHash(deferredPtr.data(), seed);
}
