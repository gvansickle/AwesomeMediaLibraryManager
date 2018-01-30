#include "PromiseSitter.h"

namespace QtPromise {

Q_GLOBAL_STATIC(PromiseSitter, promiseSitterGlobalInstance)

PromiseSitter* PromiseSitter::instance()
{
	return promiseSitterGlobalInstance;
}

PromiseSitter::~PromiseSitter()
{
	for (auto iter = m_sitterConnections.cbegin(); iter != m_sitterConnections.cend(); ++iter)
		QObject::disconnect(iter.value());
	for (auto iter = m_contextConnections.cbegin(); iter != m_contextConnections.cend(); ++iter)
		QObject::disconnect(iter.value());
}

void PromiseSitter::add(Promise::Ptr promise, const QVector<const QObject*>& contextObjs)
{
	if (promise->state() == Deferred::Pending)
	{
		QWriteLocker locker{&m_lock};
		Promise* rawPromise = promise.data();
		if (!m_promises.contains(rawPromise))
		{
			/* Need to use QueuedConnection since we may not delete the promise
			 * in a slot connected to its signal.
			 */
			auto resolveConnection = QObject::connect(rawPromise, &Promise::resolved, this, [this, rawPromise](const QVariant&) {
				this->remove(rawPromise);
			}, Qt::QueuedConnection);
			m_sitterConnections.insert(rawPromise, resolveConnection);
			auto rejectConnection = QObject::connect(rawPromise, &Promise::rejected, this, [this, rawPromise](const QVariant&) {
				this->remove(rawPromise);
			}, Qt::QueuedConnection);
			m_sitterConnections.insert(rawPromise, rejectConnection);
			m_promises.insert(rawPromise, promise);
		}
		// Connect context objects
		for (auto contextObj : contextObjs)
		{
			if (contextObj)
			{
				auto contextConnection = QObject::connect(contextObj, &QObject::destroyed, rawPromise, [this, rawPromise](QObject*) {
					this->remove(rawPromise);
				});
				m_contextConnections.insert(rawPromise, contextConnection);
			}
		}
	}
}

bool PromiseSitter::remove(const Promise* promise)
{
	QWriteLocker locker{&m_lock};
	bool removed = (m_promises.remove(promise) > 0);

	if (removed)
	{
		for (auto connection : m_sitterConnections.values(promise))
			QObject::disconnect(connection);
		m_sitterConnections.remove(promise);

		for (auto connection : m_contextConnections.values(promise))
			QObject::disconnect(connection);
		m_contextConnections.remove(promise);
	}

	return removed;
}

bool PromiseSitter::contains(Promise::Ptr promise) const
{
	QReadLocker locker{&m_lock};
	return m_promises.contains(promise.data());
}

} // namespace QtPromise
