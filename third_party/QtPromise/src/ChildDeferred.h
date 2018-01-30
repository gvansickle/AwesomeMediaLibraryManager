/*! \file
 *
 * \date Created on: 21.02.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_CHILDDEFERRED_H_
#define QTPROMISE_CHILDDEFERRED_H_

#include "Deferred.h"
#include <QList>
#include <QVector>

#include <functional>

namespace QtPromise
{

/*!
 * \cond INTERNAL
 */

/*!
 * \brief A Deferred holding pointers to other (parent) Deferreds.
 *
 * This class is used internally to realize the Promise chaining which is in fact
 * rather a Deferred chain:
 * when Promise::then(), Promise::all() or Promise::any() is called, a ChildDeferred
 * is created which holds QSharedPointers to the original Promises' Deferreds to prevent
 * their destruction.
 *
 * \threadsafeClass
 * \author jochen.ulrich
 */
class ChildDeferred : public Deferred
{
	Q_OBJECT

public:

	/*! Smart pointer to ChildDeferred. */
	typedef QSharedPointer<ChildDeferred> Ptr;

	/*! Defines the type of functions that connected to Deferred signals.
	 *
	 * \sa connectParent()
	 */
	typedef std::function<void(const QVariant&)> WrappedCallbackFunc;


	/*! Disconnects all signals of parent Deferreds.
	 */
	virtual ~ChildDeferred();

	/*! Creates a ChildDeferred which holds a pointer to a parent Deferred.
	 *
	 * \param parent The Deferred which acts as parent to this ChildDeferred.
	 * \param trackResults If \c true, the ChildDeferred listens to the signals
	 * of the \p parent. See setTrackParentResults().
	 * \return QSharedPointer to a new, pending ChildDeferred.
	 */
	static Ptr create(Deferred::Ptr parent, bool trackResults = false);
	/*! Creates a ChildDeferred which holds pointers to multiple parent Deferreds.
	 *
	 * \param parents List of parent Deferreds.
	 * \param trackResults If \c true, the ChildDeferred listens to the signals
	 * of the \p parents. See setTrackParentResults().
	 * \return QSharedPointer to a new, pending ChildDeferred.
	 */
	static Ptr create(const QVector<Deferred::Ptr>& parents, bool trackResults = false);

	/*! Sets the parent of this ChildDeferred.
	 *
	 * \param parent The new parent for this ChildDeferred.
	 */
	void setParent(Deferred::Ptr parent);
	/*! Sets the parents of this ChildDeferred.
	 *
	 * \param parents The new parents of this ChildDeferred.
	 */
	void setParents(const QVector<Deferred::Ptr>& parents);

	/*! Adds a parent to this ChildDeferred.
	 *
	 * \param parent The parent to be added to this ChildDeferred.
	 */
	void addParent(Deferred::Ptr parent);

	/*! \return The parents of this ChildDeferred.
	 */
	QVector<Deferred::Ptr> parents() const { return m_parents; }

	/*! Defines whether this ChildDeferred watches the results of its parent.
	 *
	 * If enabled, the ChildDeferred emits the parentResolved(), parentRejected(), parentsResolved()
	 * and parentsResolved() signals.
	 *
	 * \note When enabling the tracking and there are parents that are already resolved or rejected,
	 * this method will emit appropriate signals asynchronously, that is when the control returns to the event loop,
	 * and no matter if those signals have already been emitted before.
	 * Therefore, enabling this settings should rather be done directly when creating the ChildDeferred.
	 *
	 * \param trackParentResults If \c true, the ChildDeferred listens to the signals
	 * of the \p parents and emits the parentResolved(), parentRejected(), parentsResolved()
	 * and parentsResolved() signals.
	 */
	void setTrackParentResults(bool trackParentResults);


	/*! Disconnects and removes all parents from this ChildDeferred.
	 *
	 * \param delayed If \c true, the QSharedPointers to the parents
	 * are released when control reaches the event loop. If \c false,
	 * the pointers to the parents are immediately released.
	 */
	void removeParents(bool delayed);

	/*! Connects callbacks to the signals of the parent and remembers the connections.
	 *
	 * Using this method to connect to the parent's signals ensures that the callbacks
	 * are removed when this ChildDeferred is destroyed or when the \p parent is removed
	 * from this ChildDeferred.
	 *
	 * \tparam ResolvedFunc A method expecting a `const QVariant&` as parameter.
	 * \tparam RejectedFunc A method expecting a `const QVariant&` as parameter.
	 * \tparam NotifiedFunc A method expecting a `const QVariant&` as parameter.
	 * \param parent A QSharedPointer to a Deferred. The provided callbacks are connected
	 * to the corresponding signals of \p parent. Expects that\p parent is a parent of this
	 * ChildDeferred. See also setParent().
	 * \param resolveCallback A callback connected to \p parent's Deferred::resolved() signal.
	 * \param rejectCallback A callback connected to \p parent's Deferred::rejected() signal.
	 * \param notifyCallback A callback connected to \p parent's Deferred::notified() signal.
	 */
	template <typename ResolvedFunc, typename RejectedFunc, typename NotifiedFunc>
	void connectParent(Deferred::Ptr parent, ResolvedFunc&& resolveCallback, RejectedFunc&& rejectCallback, NotifiedFunc&& notifyCallback);

	/*! \returns \c true if this ChildDeferred is watching the results of its parents.
	 *
	 * \sa setTrackResults()
	 */
	bool isTrackingParentResults() const { return m_trackParentResults; }

Q_SIGNALS:
	/*! Emitted when one of the parent Deferreds is resolved.
	 *
	 * \param value The result of the resolved Deferred.
	 *
	 * \sa Deferred::resolved()
	 */
	void parentResolved(const QVariant& value) const;
	/*! Emitted when all parent Deferreds are resolved.
	 *
	 * \param results List of the results of the Deferreds in
	 * the order of the Deferreds as provided to create().
	 */
	void parentsResolved(QList<QVariant> results) const;
	/*! Emitted when one of the parent Deferreds is rejected.
	 *
	 * \param reason The reason why the Deferred was rejected.
	 *
	 * \sa Deferred::rejected()
	 */
	void parentRejected(const QVariant& reason) const;
	/*! Emitted when all parent Deferreds are rejected.
	 *
	 * \param reasons List of the reasons why the Deferreds were
	 * rejected in the order of the Deferreds as provided to create().
	 */
	void parentsRejected(QList<QVariant> reasons) const;

protected:
	/*! Creates a pending ChildDeferred object holding a pointer to a parent Deferred.
	 *
	 * \param parent The Deferred which should exist as long as this ChildDeferred exists.
	 */
	ChildDeferred(const QVector<Deferred::Ptr>& parents, bool trackResults);

private Q_SLOTS:
	void onParentDestroyed(QObject* parent);
	void onParentResolved(const QVariant& value);
	void onParentRejected(const QVariant& reason);

	void removeParentsDelayed() { removeParents(true); }

private:
	void trackParentResult(Deferred* parent);
	void disconnectParents();
	void disconnectParent(Deferred* parent);

	mutable QMutex m_lock;
	QVector<Deferred::Ptr> m_parents;
	QHash<Deferred*, QVector<QMetaObject::Connection>> m_parentConnections;
	int m_resolvedCount;
	int m_rejectedCount;
	bool m_trackParentResults;
};


//####### Template Implementation #######
template <typename ResolvedFunc, typename RejectedFunc, typename NotifiedFunc>
void ChildDeferred::connectParent(Deferred::Ptr parent, ResolvedFunc&& resolvedCallback, RejectedFunc&& rejectedCallback, NotifiedFunc&& notifiedCallback)
{
	Q_ASSERT_X(m_parents.contains(parent), "ChildDeferred::connectParent()", "parent should be added as parent to this ChildDeferred");

	/* For some reason, the wrapped callbacks are not called when we add a context object.
	 * Therefore, we have to remember the connections to disconnect the lambdas manually.
	 */
	QVector<QMetaObject::Connection>& parentConnections = m_parentConnections[parent.data()];
	parentConnections << QObject::connect(parent.data(), &Deferred::resolved, std::forward<ResolvedFunc>(resolvedCallback));
	parentConnections << QObject::connect(parent.data(), &Deferred::rejected, std::forward<RejectedFunc>(rejectedCallback));
	parentConnections << QObject::connect(parent.data(), &Deferred::notified, std::forward<NotifiedFunc>(notifiedCallback));
}


/*!
 * \endcond
 */

} /* namespace QtPromise */

#endif /* QTPROMISE_CHILDDEFERRED_H_ */
