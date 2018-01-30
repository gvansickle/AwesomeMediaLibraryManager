/*! \file
 *
 * \date Created on: 07.05.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_PROMISESITTER_H_
#define QTPROMISE_PROMISESITTER_H_

#include <QObject>
#include <QMultiHash>
#include <QSharedPointer>
#include <QReadWriteLock>
#include <QVector>
#include "Promise.h"

namespace QtPromise {

/*! \brief Holds Promises until they are resolved or rejected.
 *
 * When attaching actions using Promise::then(), it is necessary to hold a reference
 * to the returned Promise to ensure the action is executed.
 * To avoid storing the Promise as a class member, you can hand over the Promise::Ptr
 * to the PromiseSitter which holds it until the Promise is resolved or rejected.
 *
 * \note Since the Promise will typically be destroyed after removing from the PromiseSitter,
 * it is not removed immediately after it has been resolved or rejected but when the control
 * returns to the event loop. This is necessary to prevent that the Promise is deleted in
 * a handler connected to its own signal.
 *
 * ### Global Instance ###
 * For convenience, there is a global instance of a PromiseSitter which can be retrieved
 * using PromiseSitter::instance().
 *
 * ### Example ###
 *
\code
void MyClass::attachAction(Promise::Ptr promise)
{
	Promise::Ptr finalPromise = promise->then([](const QVariant& data) {
		// Do something
	});
	PromiseSitter::instance()->add(finalPromise);
}
\endcode
 *
 * \threadsafeClass
 * \author jochen.ulrich
 */
class PromiseSitter : public QObject
{
	Q_OBJECT

public:
	/*! Creates a new PromiseSitter.
	 *
	 * \param parent The parent QObject.
	 */
	PromiseSitter(QObject* parent = nullptr) : QObject(parent) {}
	/*! Releases all Promise::Ptr and disconnects the context objects.
	 */
	virtual ~PromiseSitter();

	/*! Provides access to the global PromiseSitter instance.
	 *
	 * \return A pointer to the global PromiseSitter instance.
	 */
	static PromiseSitter* instance();

	/*! Adds a Promise to this PromiseSitter.
	 *
	 * The PromiseSitter tracks the added promises based on the actual pointer
	 * (i.e. QSharedPointer::data()). Therefore, it will keep the promise only
	 * once even when add() is called multiple times with different QSharedPointers
	 * to the same Promise.
	 *
	 * \note It is pointless to store already resolved or rejected promises as their
	 * actions have already been executed. Therefore, trying to add a resolved or
	 * rejected Promise will do nothing.
	 *
	 * \param promise The Promise::Ptr which should be kept until the Promise is
	 * resolved or rejected.
	 * \param contextObj A QObject acting as the lifetime context for
	 * \p promise. When the \p contextObj is destroyed (emits the QObject::destroyed() signal),
	 * the \p promise is removed from the PromiseSitter. If the \p promise has already been added
	 * to the PromiseSitter, the \p contextObj is added to the existing context objects.
	 * This parameter was added in 1.2.0.
	 *
	 * \sa remove()
	 */
	void add(Promise::Ptr promise, const QObject* contextObj) { add(promise, QVector<const QObject*>{contextObj}); }

	/*! \overload
	 *
	 * \param promise The Promise::Ptr which should be kept until the Promise is resolved
	 * or rejected.
	 * \param contextObjs An optional initializer list of QObjects acting as the lifetime context
	 * for \p promise. When **any** of the objects in the list is destroyed (emits the
	 * QObject::destroyed() signal), the \p promise is removed from the PromiseSitter.
	 * If the \p promise has already been added to the PromiseSitter, the \p contextObjs
	 * are added to the existing context objects.
	 * This parameter was added in 1.2.0.
	 *
	 * \sa remove()
	 */
	void add(Promise::Ptr promise, const QVector<const QObject*>& contextObjs = {});

	/*! Explicitly removes a Promise from this PromiseSitter.
	 *
	 * \param promise A raw Pointer to the Promise to be removed from the PromiseSitter.
	 * \return \c true if the PromiseSitter contained the \p promise. \c false otherwise.
	 */
	bool remove(const Promise* promise);

	/*! \overload
	 *
	 * \param promise The Promise::Ptr to be removed from the PromiseSitter.
	 * \return \c true if the PromiseSitter contained a Promise::Ptr to that Promise.
	 * \c false otherwise.
	 */
	bool remove(Promise::Ptr promise) { return remove(promise.data()); }

	/*! Checks if a Promise is hold by this PromiseSitter.
	 *
	 * \param promise The Promise::Ptr to the Promise.
	 * \return \c true if this PromiseSitter contains a Promise::Ptr to the given
	 * Promise. \c false otherwise.
	 */
	bool contains(Promise::Ptr promise) const;

private:
	mutable QReadWriteLock m_lock;
	QHash<const Promise*, Promise::Ptr> m_promises;
	QMultiHash<const Promise*, QMetaObject::Connection> m_sitterConnections;
	QMultiHash<const Promise*, QMetaObject::Connection> m_contextConnections;
};

}  // namespace QtPromise

#endif /* QTPROMISE_PROMISESITTER_H_ */
