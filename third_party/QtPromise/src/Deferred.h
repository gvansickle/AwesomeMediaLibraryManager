/*! \file
 *
 * \date Created on: 17.02.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_DEFERRED_H_
#define QTPROMISE_DEFERRED_H_

#include <QObject>
#include <QVariant>
#include <QMutex>
#include <QException>
#include <QSharedPointer>
#include <QAtomicInt>


namespace QtPromise {

/*! \brief Communicates the outcome of an asynchronous operation.
 *
 * The usage pattern of Deferred is:
 * - Create a new Deferred
 * - Prepare the asynchronous operation
 * - Configure that the Deferred is resolved or rejected when the
 * result of the asynchronous operation is available
 * - Possibly: configure that the Deferred is notified when the
 * asynchronous operation progresses
 * - Start the asynchronous operation
 * - Create a Promise for the Deferred and return it
 *
 * For example:
\code
using namespace QtPromise;

Promise::Ptr startAsyncOperation()
{
	Deferred::Ptr deferred = Deferred::create();
	MyAsyncObject* asyncObj = new MyAsyncObject(deferred.data()); // We use the Deferred as parent of
	                                                              // asyncObj so we do not leak memory.
	connect(asyncObj, &MyAsyncObject::success, deferred.data(), &Deferred::resolve);
	connect(asyncObj, &MyAsyncObject::failure, deferred.data(), &Deferred::reject);
	connect(asyncObj, &MyAsyncObject::progress, deferred.data(), &Deferred::notify);
	asyncObj->start();
	return Promise::create(deferred);
}
\endcode
 *
 * ## Subclassing ##
 * As a general rule when deriving from Deferred:
 * After construction, a Deferred must always be Pending.
 * The Deferred may only be resolved/rejected when explicitly requested by
 * the creator (i.e. calling resolve() or reject()) or when the control
 * returns to the event loop.\n
 * In other words: do not resolve/reject a Deferred directly in the constructor.
 * Instead use a QTimer::singleShot() with a \c 0 timeout to resolve/reject
 * the Deferred when the control returns to the event loop.
 *
 * A subclass should call checkDestructionInSignalHandler() in its destructor to help
 * debugging crashes caused by deleting the Deferred as reaction to its own signal.

 * When providing specialized (overloaded) resolved() / rejected() / notified() signals in subclasses
 * (see for example NetworkDeferred::resolved()), use the methods
 * resolveAndEmit() / rejectAndEmit() / notifyAndEmit() to resolve/reject/notify and emit the
 * specialized signal in one operation. This is necessary to ensure that
 * checkDestructionInSignalHandler() is working correctly.
 *
 * \threadsafeClass
 * \author jochen.ulrich
 */
class Deferred : public QObject
{
	Q_OBJECT

public:
	/*! Smart pointer to Deferred. */
	typedef QSharedPointer<Deferred> Ptr;

	/*! Possible states of a Deferred or Promise. */
	enum State
	{
		Pending = 0,  /*!< The outcome of the asynchronous operation has not
		               * been reported yet.
		               */
		Resolved = 1, //!< The asynchronous operation completed successfully.
		Rejected = -1 //!< The asynchronous operation failed.
	};

	/*! Creates a pending Deferred object.
	 *
	 * \return QSharedPointer to a new, pending Deferred.
	 */
	static Ptr create();
	/*! Creates a resolved or rejected Deferred.
	 *
	 * This is a convenience method which creates a Deferred and directly
	 * resolves or rejects it with \p data according to \p state.
	 *
	 * \param state Defines whether the Deferred should resolved or rejected.
	 * Should be either Resolved or Rejected. Pending will be treated like Resolved.
	 * \param data The value or rejection reason used to resolve or reject
	 * the Deferred depending on \p state.
	 * \return A QSharedPointer to a new, resolved or rejected Deferred.
	 */
	static Ptr create(State state, const QVariant& data);
	/*! Checks for usage errors and rejects the Deferred when necessary.
	 *
	 * When the Deferred is still pending when being destroyed,
	 * it logs a warning using qDebug().
	 *
	 * \sa checkDestructionInSignalHandler()
	 */
	virtual ~Deferred();

	/*! \return The current state of the Deferred. */
	Deferred::State state() const { QMutexLocker locker(&m_lock); return m_state; }
	/*! \return The current data of the Deferred.
	 * Depending on the state of the Deferred, this is either the resolve value,
	 * the rejection reason or an invalid QVariant when the Deferred is still pending.
	 */
	QVariant data() const { QMutexLocker locker(&m_lock); return m_data; }

Q_SIGNALS:
	/*! Emitted when the asynchronous operation was successful.
	 *
	 * \param value The result of the asynchronous operation.
	 * The actual type of the value depends on the asynchronous operation
	 * and should be documented by the creator of the Deferred.
	 */
	void resolved(const QVariant& value) const;
	/*! Emitted when the asynchronous operation failed.
	 *
	 * \param reason Indication of the reason why the operation failed.
	 * The actual type of the value depends on the asynchronous operation
	 * and should be documented by the creator of the Deferred.
	 */
	void rejected(const QVariant& reason) const;
	/*! Emitted to notify about progress of the asynchronous operation.
	 *
	 * \param progress Information about the progress of the operation.
	 * The actual type of the progress depends on the asynchronous operation
	 * and should be documented by the creator of the Deferred.
	 */
	void notified(const QVariant& progress) const;

public Q_SLOTS:
	/*! Communicates success of the asynchronous operation.
	 *
	 * Call this slot when the asynchronous operation succeeded to submit
	 * the result to the promises.
	 * It is important to make the actual type of the value clear for the promises
	 * as QVariant::value() requires the exact type to be able to convert the value.
	 *
	 * \param value The result of the asynchronous operation.
	 * \return \c true if the Deferred has been set to \ref Resolved.
	 * \c false if the Deferred was not in the \ref Pending state.
	 */
	bool resolve(const QVariant& value = QVariant());
	/*! Communicates failure of the asynchronous operation.
	 *
	 * Call this slot when the asynchronous operation failed and provide a reason
	 * describing why the operation failed.
	 * It is important to make the actual type of the value clear for the promises
	 * as QVariant::value() requires the exact type to be able to convert the value.
	 *
	 * \param reason An object indicating why the operation failed.
	 * \return \c true if the Deferred has been set to \ref Rejected.
	 * \c false if the Deferred was not in the \ref Pending state.
	 */
	bool reject(const QVariant& reason = QVariant());
	/*! Communicates progress of the asynchronous operation.
	 *
	 * Call this slot when you want to inform the promises about progress
	 * of the asynchronous operation.
	 * It is important to make the actual type of the value clear for the promises
	 * as QVariant::value() requires the exact type to be able to convert the value.
	 *
	 * \param progress An object representing the progress of the operation.
	 * \return \c true if the Deferred has been notified.
	 * \c false if the Deferred was not in the \ref Pending state.
	 */
	bool notify(const QVariant& progress = QVariant());

protected:
	/*! Creates a pending Deferred object.
	 */
	Deferred();
	/*! Defines whether the Deferred logs a debug message when resolve() or
	 * reject() is called when the Deferred is already resolved/rejected.
	 *
	 * By default, the debug message is logged.
	 *
	 * \param logInvalidActionMessage If \c true, the message is logged.
	 * If \c false, no message is logged when resolve() / reject() is called
	 * multiple times.
	 */
	void setLogInvalidActionMessage(bool logInvalidActionMessage);

	/*! Checks for destruction in a signal handler and logs an error.
	 *
	 * This method is intended to be used by the destructor of derived classes.
	 * It checks if the current method is executed as reaction to an
	 * own signal of this Deferred. And if it is, it logs a critical message.
	 *
	 * This helps users to debug crashes. To solve the issue, the destruction
	 * of the Deferred (that is the deletion of the last QSharedPointer) must
	 * be done asynchronously, for example using QTimer::singleShot().
	 *
	 * \since 2.0.0
	 */
	void checkDestructionInSignalHandler();

	/*! Resolves this Deferred and emits a signal.
	 *
	 * This is a convenience method which resolves this Deferred with \p value and if it was resolved,
	 * emits the \p signal with \p value. If this Deferred was not resolved, \p signal is *not* emitted.
	 * Typically, \p signal is a specialized form of the resolved() signal.
	 * This method helps detecting deletion of this Deferred in a slot/functor connected to \p signal.
	 *
	 * \tparam ValueType The type of the \p value. Must be registered with Qt's meta type system
	 * since it will be converted to QVariant to call resolve().
	 * \tparam Signal The type of the signal to be emitted. Must be invocable with exactly one parameter
	 * of type \p ValueType.
	 * \param value The data used to resolve this Deferred and emit \p signal.
	 * \param signal The signal which is emitted with \p value.
	 *
	 * \sa resolve()
	 * \sa checkDestructionInSignalHandler()
	 * \since 2.0.0
	 */
	template<typename ValueType, typename Signal>
	void resolveAndEmit(const ValueType& value, Signal&& signal);

	/*! Rejects this Deferred and emits a signal.
	 *
	 * This is a convenience method which rejects this Deferred with \p reason and if it was rejected,
	 * emits the \p signal with \p reason. If this Deferred was not rejected, \p signal is *not* emitted.
	 * Typically, \p signal is a specialized form of the rejected() signal.
	 * This method helps detecting deletion of this Deferred in a slot/functor connected to \p signal.

	 * \tparam ReasonType The type of the \p reason. Must be registered with Qt's meta type system
	 * since it will be converted to QVariant to call reject().
	 * \tparam Signal The type of the signal to be emitted. Must be invocable with exactly one parameter
	 * of type \p ReasonType.
	 * \param reason The reason used to reject this Deferred.
	 * \param signal The signal which is emitted with \p reason.
	 *
	 * \sa reject()
	 * \sa checkDestructionInSignalHandler()
	 * \since 2.0.0
	 */
	template<typename ReasonType, typename Signal>
	void rejectAndEmit(const ReasonType& reason, Signal&& signal);

	/*! Notifies this Deferred and emits a signal.
	 *
	 * This is a convenience method which notifies this Deferred with \p progress and if it was notified,
	 * emits the \p signal with \p progress. If this Deferred was not notified, \p signal is *not* emitted.
	 * Typically, \p signal is a specialized form of the notified() signal.
	 * This method helps detecting deletion of this Deferred in a slot/functor attached to \p signal.
	 *
	 * \tparam ProgressType The type of the \p progress. Must be registered with Qt's meta type system
	 * since it will be converted to QVariant to call notify().
	 * \tparam Signal The type of the signal to be emitted. Must be invocable with exactly one parameter
	 * of type \p ProgressType.
	 * \param progress The progress used to notify this Deferred.
	 * \param signal The signal which is emitted with \p progress.
	 *
	 * \sa notify()
	 * \sa checkDestructionInSignalHandler()
	 * \since 2.0.0
	 */
	template<typename ProgressType, typename Signal>
	void notifyAndEmit(const ProgressType& progress, Signal&& signal);


private:
	void logInvalidActionMessage(const char* action) const;
	
	mutable QMutex m_lock;
	State m_state;
	QVariant m_data;
	bool m_logInvalidActionMessage = true;
	QAtomicInt m_isInSignalHandler;

	static void registerMetaTypes();
};

inline QString pointerToQString(const void* pointer)
{
	return QString("0x%1").arg(reinterpret_cast<quintptr>(pointer),
	                           QT_POINTER_SIZE * 2, 16, QChar('0'));
}

}  // namespace QtPromise

#include "Deferred_impl.h"

Q_DECLARE_METATYPE(QtPromise::Deferred::State)

/*! Returns the hash value for a Deferred smart pointer.
 * \param deferredPtr The QSharedPointer who's hash value should be returned.
 * \param seed The seed used for the calculation.
 * \return The hash value based on the address of the pointer.
 */
uint qHash(const QtPromise::Deferred::Ptr& deferredPtr, uint seed = 0);

#endif /* QTPROMISE_DEFERRED_H_ */
