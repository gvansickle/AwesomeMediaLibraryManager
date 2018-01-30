#include "ChildDeferred.h"
#include <QTimer>

namespace QtPromise
{

/*!
 * \cond INTERNAL
 */

ChildDeferred::ChildDeferred(const QVector<Deferred::Ptr>& parents, bool trackResults)
	: Deferred(), m_lock(QMutex::Recursive), m_resolvedCount(0), m_rejectedCount(0)
{
	setLogInvalidActionMessage(false);
	setParents(parents);
	setTrackParentResults(trackResults);

	QObject::connect(this, &Deferred::resolved, this, &ChildDeferred::removeParentsDelayed);
	QObject::connect(this, &Deferred::rejected, this, &ChildDeferred::removeParentsDelayed);
}

ChildDeferred::Ptr ChildDeferred::create(Deferred::Ptr parent, bool trackResults)
{
	return create(QVector<Deferred::Ptr>{parent}, trackResults);
}

ChildDeferred::Ptr ChildDeferred::create(const QVector<Deferred::Ptr>& parents, bool trackResults)
{
	return Ptr(new ChildDeferred(parents, trackResults));
}

ChildDeferred::~ChildDeferred()
{
	checkDestructionInSignalHandler();

	QMutexLocker locker(&m_lock);
	/* We disconnect all parent Deferreds to avoid that they trigger
	 * onParentDestroyed() and onParentRejected() when m_parents is released
	 * and they are still pending.
	 * This ChildDeferred itself will be rejected by the Deferred destructor.
	 */
	disconnectParents();
}

void ChildDeferred::setParent(Deferred::Ptr parent)
{
	setParents(QVector<Deferred::Ptr>{parent});
}

void ChildDeferred::setParents(const QVector<Deferred::Ptr>& parents)
{
	QMutexLocker locker(&m_lock);

	disconnectParents();

	for (Deferred::Ptr parent : parents)
		QObject::connect(parent.data(), &QObject::destroyed, this, &ChildDeferred::onParentDestroyed);

	m_parents = parents;

	// If it is enabled, "restart" the result tracking with the new parents
	if (m_trackParentResults)
		setTrackParentResults(true);
}

void ChildDeferred::addParent(Deferred::Ptr parent)
{
	QMutexLocker locker(&m_lock);

	QObject::connect(parent.data(), &QObject::destroyed, this, &ChildDeferred::onParentDestroyed, Qt::UniqueConnection);

	m_parents.append(parent);

	if (m_trackParentResults)
		trackParentResult(parent.data());
}

void ChildDeferred::removeParents(bool delayed)
{
	QMutexLocker locker(&m_lock);

	disconnectParents();

	if (delayed)
	{
		auto oldParents = m_parents;
		QTimer::singleShot(0, [oldParents]() mutable {
			/* No need to do anything in here.
			 * We just need to hold the parent pointers until the event loop.
			 */
			oldParents.clear();
		});
	}

	m_parents.clear();
}

void ChildDeferred::setTrackParentResults(bool trackParentResults)
{
	QMutexLocker locker(&m_lock);

	m_trackParentResults = trackParentResults;
	if (m_trackParentResults)
	{
		m_resolvedCount = m_rejectedCount = 0;

		for (Deferred::Ptr parent : const_cast<const QVector<Deferred::Ptr>&>(m_parents))
			trackParentResult(parent.data());
	}
	else
	{
		for (Deferred::Ptr parent : const_cast<const QVector<Deferred::Ptr>&>(m_parents))
		{
			QObject::disconnect(parent.data(), &Deferred::resolved, this, &ChildDeferred::onParentResolved);
			QObject::disconnect(parent.data(), &Deferred::rejected, this, &ChildDeferred::onParentRejected);
		}
	}
}

void ChildDeferred::trackParentResult(Deferred* parent)
{
	switch(parent->state())
	{
	case Resolved:
		QTimer::singleShot(0, this, [this, parent]() {
			this->onParentResolved(parent->data());
		});
		break;
	case Rejected:
		QTimer::singleShot(0, this, [this, parent]() {
			this->onParentRejected(parent->data());
		});
		break;
	case Pending:
	default:
		QObject::connect(parent, &Deferred::resolved, this, &ChildDeferred::onParentResolved, Qt::UniqueConnection);
		QObject::connect(parent, &Deferred::rejected, this, &ChildDeferred::onParentRejected, Qt::UniqueConnection);
	}
}

void ChildDeferred::disconnectParents()
{
	for (Deferred::Ptr parent : const_cast<const QVector<Deferred::Ptr>&>(m_parents))
		disconnectParent(parent.data());
}

void ChildDeferred::disconnectParent(Deferred* parent)
{
	QObject::disconnect(parent, 0, this, 0);
	for (auto connection : m_parentConnections.value(parent))
		QObject::disconnect(connection);
	m_parentConnections.remove(parent);
}


void ChildDeferred::onParentDestroyed(QObject* parent)
{
	QMutexLocker locker(&m_lock);
	qCritical("Parent deferred %s is destroyed while child %s is still holding a reference", qUtf8Printable(pointerToQString(parent)), qUtf8Printable(pointerToQString(this)));
	auto deferredParent = static_cast<Deferred*>(parent);
	disconnectParent(deferredParent);
	QVector<int> removeIndices;
	for (int i = m_parents.size()-1; i >= 0; --i)
	{
		if (m_parents.at(i) == deferredParent)
			removeIndices.append(i);
	}

	// removeIndices contains indices sorted from highest to lowest
	for (int i : const_cast<const QVector<int>&>(removeIndices))
		m_parents.removeAt(i);
		
}

void ChildDeferred::onParentResolved(const QVariant& value)
{
	QMutexLocker locker(&m_lock);
	m_resolvedCount += 1;
	Q_EMIT parentResolved(value);
	if (m_resolvedCount == m_parents.size())
	{
		// All parents have been resolved
		QList<QVariant> results;
		for (Deferred::Ptr parent : const_cast<const QVector<Deferred::Ptr>&>(m_parents))
			results.append(parent->data());
		Q_EMIT parentsResolved(results);
	}
}

void ChildDeferred::onParentRejected(const QVariant& reason)
{
	QMutexLocker locker(&m_lock);
	m_rejectedCount += 1;
	Q_EMIT parentRejected(reason);
	if (m_rejectedCount == m_parents.size())
	{
		// All parents have been rejected
		QList<QVariant> reasons;
		for (Deferred::Ptr parent : const_cast<const QVector<Deferred::Ptr>&>(m_parents))
			reasons.append(parent->data());
		Q_EMIT parentsRejected(reasons);
	}
}

/*!
 * \endcond
 */

} /* namespace QtPromise */
