/*! \file
 *
 * \date Created on: 17.02.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_PROMISE_H_
#define QTPROMISE_PROMISE_H_

#include "Deferred.h"
#include "ChildDeferred.h"

#include <QObject>
#include <QVariant>
#include <QSharedPointer>
#include <QVector>

#include <cstddef>
#include <functional>
#include <type_traits>

namespace QtPromise {


/*! \deprecated Since 2.0.0, you can use \c nullptr instead.
 *
 * "No operation" to be used in combination with Promise::then().
 *
 * This function simply does nothing.
 * It can be used as parameter to Promise::then() to "skip" parameters:
\code
myPromise->then(QtPromise::noop, [](const QVariant& reason) {
  // handle rejection
});
\endcode
 */
inline void noop(const QVariant&) {};


/*! \brief Provides read-only access to the outcome of an asynchronous operation.
 *
 * There are two main usage patterns of Promise objects:
 * - promise chaining using Promise::then()
 * - using the signals
 *
 *	\threadsafeClass
 *	\author jochen.ulrich
 */
class Promise : public QObject
{
	Q_OBJECT

public:
	/*! Smart pointer to a Promise. */
	typedef QSharedPointer<Promise> Ptr;

	/*! Creates a Promise for a given Deferred.
	 *
	 * \param deferred The Deferred whose state is communicated
	 * by the created Promise.
	 * \return QSharedPointer to a new Promise for the given \p deferred.
	 */
	static Ptr create(Deferred::Ptr deferred);
	/*! Creates a resolved Promise.
	 * 
	 * Creates a Deferred, resolves it with the given \p value and returns
	 * a Promise on the Deferred.
	 * 
	 * \param value The value used to resolve the Promise.
	 * \return QSharedPointer to a new, resolved Promise.
	 */
	static Ptr createResolved(const QVariant& value = QVariant());
	/*! Creates a rejected Promise.
	 *
	 * Creates a Deferred, rejects it with the given \p reason and returns
	 * a Promise on the Deferred.
	 * 
	 * \param reason The reason used to reject the Promise.
	 * \return QSharedPointer to a new, rejected Promise.
	 */
	static Ptr createRejected(const QVariant& reason = QVariant());

	/*! Creates a Promise which is resolved after a delay.
	 *
	 * This is useful to do event processing in a Promise chain:
	 * \code
	 * void MyClass::doSomething()
	 * {
	 *     this->myPromise = startAsyncOperation()
	 *     ->then([] (const QVariant& data) -> Promise::Ptr {
	 *         // do something
	 *         return Promise::delayedResolve();
	 *     })->then([] (const QVariant& data) {
	 *         // do something after events have been processed
	 *     });
	 * }
	 * \endcode
	 *
	 * \param value The value used to resolve the Promise.
	 * \param delayInMillisec The delay in milliseconds.
	 * If \p delayInMillisec is \c 0, the resolve is delayed
	 * until the current events in the queue have been processed.
	 * \return QSharedPointer to a new Promise which will be resolved
	 * with \p value after the given \p delayInMillisec.
	 *
	 * \sa delayedReject()
	 * \since 2.0.0
	 */
	static Ptr delayedResolve(const QVariant& value = QVariant(), int delayInMillisec = 0);

	/*! Creates a Promise which is rejected after a delay.
	 *
	 * \param reason The reason used to reject the Promise.
	 * \param delayInMillisec The delay in milliseconds.
	 * If \p delayInMillisec is 0, the reject is delayed
	 * until the current events in the queue have been processed.
	 * \return QSharedPointer to a new Promise which will be rejected
	 * with \p reason after the given \p delayInMillisec.
	 *
	 * \sa delayedResolve()
	 * \since 2.0.0
	 */
	static Ptr delayedReject(const QVariant& reason = QVariant(), int delayInMillisec = 0);

	/*! Combines multiple Promises using "and" semantics.
	 *
	 * Creates a Promise which is resolved when *all* provided promises
	 * are resolved and rejected when any of the promises is rejected.
	 * When resolved, the value is a QList<QVariant> of the values of promises
	 * in the order of the \p promises.
	 * When rejected, the reason is the rejection reason of the first rejected
	 * promise.
	 *
	 * \tparam PromiseContainer A container type of Promise::Ptr objects.
	 * The container type must be iterable using a range-based \c for loop.
	 * \param promises A \p PromiseContainer of the promises which should
	 * be combined.
	 * \return A QSharedPointer to a new Promise which is resolved when all
	 * \p promises are resolved and rejected when any of the \p promises is rejected.
	 * The returned Promise is *not* notified.
	 */
	template<typename PromiseContainer>
	static Ptr all(PromiseContainer&& promises) { return Promise::all_impl(std::forward<PromiseContainer>(promises)); }
	/*! \overload
	 * Overload for initializer lists.
	 */
	template<typename ListType>
	static Ptr all(const std::initializer_list<ListType>& promises) { return Promise::all_impl(promises); }

	/*! Combines multiple Promises using "or" semantics.
	 *
	 * Creates a Promise which is resolved when *any* provided promise
	 * is resolved and rejected when all of the promises are rejected.
	 * When resolved, the value is the resolve value of the first resolved
	 * promise.
	 * When rejected, the reason is a QList<QVariant> of the rejection reasons
	 * of the promises in the order of the \p promises.
	 *
	 * \tparam PromiseContainer A container type of Promise::Ptr objects.
	 * The container type must be iterable using a range-based \c for loop.
	 * \param promises A \p PromiseContainer of the promises which should
	 * be combined.
	 * \return A QSharedPointer to a new Promise which is resolved when any
	 * promise of the \p promises is resolved and rejected when all the \p promises
	 * are rejected.
	 * The returned Promise is *not* notified.
	 */
	template<typename PromiseContainer>
	static Ptr any(PromiseContainer&& promises) { return Promise::any_impl(std::forward<PromiseContainer>(promises)); }
	/*! \overload
	 * Overload for initializer lists.
	 */
	template<typename ListType>
	static Ptr any(const std::initializer_list<ListType>& promises) { return Promise::any_impl(promises); }

	/*! Default destructor */
	virtual ~Promise() = default;

	/*! \return The current state of the Promise's Deferred.
	 *
	 * \sa Deferred::state()
	 */
	Deferred::State state() const;
	/*! \return The current data of the Promise's Deferred.
	 *
	 * \sa Deferred::data()
	 */
	QVariant data() const;

	/*! Attaches actions to be executed when the Promise is resolved, rejected
	 * or notified (promise chaining).
	 *
	 * This method ensures that the provided callback functions are called when this
	 * Promise is resolved, rejected or notified. This method then returns a new Promise
	 * which is resolved, rejected and notified depending on the types of the callbacks
	 * and their returned values.
	 *
	 * For the \p resolvedCallback and \p rejectedCallback, the following rules apply:
	 * - If the callback is \c nullptr or returns `void`, the returned Promise is
	 * resolved or rejected identical to this Promise.
	 * - If the callback returns `QVariant`, the returned Promise is **resolved** with
	 * the returned value.
	 * To make this absolute clear: returning a `QVariant` from a \p rejectedCallback
	 * will **resolve** the returned Promise.
	 * - If the callback returns `Promise::Ptr`, the returned Promise is resolved or
	 * rejected identical to the Promise returned by the callback.
	 * Once this Promise has been resolved or rejected, notifying the Promise returned
	 * by the callback will notify the returned Promise.
	 *
	 * For the \p notifiedCallback, the following rules apply:
	 * - If the callback is \c nullptr or returns `void`, the returned Promise is notified
	 * identically to this Promise.
	 * - If the callback returns `QVariant`, the callback is called when this promise is notified
	 * and the returned Promise is notified with the value returned by the callback.
	 * - If the callback returns `Promise::Ptr`, the returned Promise is notified identical
	 * to the Promise returned by the callback.
	 * Additionally, resolving the Promise returned by the callback or returning an already resolved
	 * Promise will also notify the returned Promise with the resolve data.
	 * Rejecting the Promise or returning a rejected Promise will do nothing which means it filters
	 * the notification.
	 *
	 * Note the special behavior of notifications:\n
	 * Before this Promise is resolved or rejected, the Promise returned by then() will be notified
	 * with the notifications from this Promise, possibly modified by the \p notifiedCallback.
	 * Once this Promise is resolved or rejected, if the corresponding callback (\p resolvedCallback
	 * or \p rejectedCallback) returns a Promise::Ptr, the Promise returned by then() will be
	 * notified with the notifications from that Promise.
	 *
	 * \warning Never delete the Promise directly from a callback. Defer any action which could lead
	 * to the deletion of the Promise to the event loop. See \ref page_ownership for more
	 * information. For that reason, it also not reasonable to enter the event loop (for example using
	 * QCoreApplication::processEvents()) from a callback. Use delayedResolve() or delayedReject()
	 * instead.
	 *
	 * \tparam ResolvedFunc A callback function type expecting a `const QVariant&` as parameter
	 * and returning either `void`, `QVariant` or `Promise::Ptr`. Or `std::nullptr_t`.
	 * \tparam RejectedFunc A callback function type expecting a `const QVariant&` as parameter
	 * and returning either `void`, `QVariant` or `Promise::Ptr`. Or `std::nullptr_t`.
	 * \tparam NotifiedFunc A callback function type expecting a `const QVariant&` as parameter
	 * and returning either `void` or `QVariant` or `Promise::Ptr`. Or `std::nullptr_t`.
	 * \param resolvedCallback A callback which is executed when the Promise's Deferred is resolved.
	 * The callback will receive the data passed to Deferred::resolve() as parameter.
	 * If `nullptr`, the resolved signal is just passed through to the returned Promise.
	 * \param rejectedCallback A callback which is executed when the Promise's Deferred is rejected.
	 * The callback will receive the data passed to Deferred::reject() as parameter.
	 * If `nullptr`, the rejected signal is just passed through to the returned Promise.
	 * \param notifiedCallback A callback which is executed when the Promise's Deferred is notified.
	 * The callback will receive the data passed to Deferred::notify() as parameter.
	 * If `nullptr`, the notified signals are just passed through to the returned Promise.
	 * \return A new Promise which is resolved/rejected/notified depending on the type and return
	 * value of the \p resolvedCallback/\p rejectedCallback/\p notifiedCallback callback. See above for details.
	 *
	 * \sa \ref page_promiseChaining
	 */
	template<typename ResolvedFunc, typename RejectedFunc = std::nullptr_t, typename NotifiedFunc = std::nullptr_t>
	Ptr then(ResolvedFunc&& resolvedCallback, RejectedFunc&& rejectedCallback = nullptr, NotifiedFunc&& notifiedCallback = nullptr ) const;

	/*! Attaches an action to be executed when the Promise is either resolved or rejected.
	 *
	 * `promise->always(func)` equivalent to `promise->then(func, func)`.
	 *
	 * \tparam AlwaysFunc A callback function type expecting a `const QVariant&` as parameter
	 * and returning either `void`, `QVariant` or `Promise::Ptr`.
	 * \param alwaysCallback A callback which is executed when the Promise's Deferred is resolved
	 * or rejected. The callback will receive the data passed to Deferred::resolve() or
	 * Deferred::reject() as parameter.
	 * \return A new Promise which is resolved/rejected depending on the type and return
	 * value of the \p alwaysCallback callback. See above for details.
	 *
	 * \sa then()
	 */
	template <typename AlwaysFunc>
	Ptr always(AlwaysFunc&& alwaysCallback) const { return this->then(std::forward<AlwaysFunc>(alwaysCallback), std::forward<AlwaysFunc>(alwaysCallback)); }


Q_SIGNALS:
	/*! Emitted when the Promise's Deferred is resolved.
	 *
	 * \param value The result of the asynchronous operation.
	 * The actual type of the value depends on the asynchronous operation
	 * and should be documented by the creator of the Promise.
	 *
	 * \sa Deferred::resolved()
	 */
	void resolved(const QVariant& value) const;

	/*! Emitted when the Promise's Deferred is rejected.
	 *
	 * \param reason Indication of the reason why the operation failed.
	 * The actual type of the value depends on the asynchronous operation
	 * and should be documented by the creator of the Promise.
	 *
	 * \sa Deferred::rejected()
	 */
	void rejected(const QVariant& reason) const;
	/*! Emitted when the Promise's Deferred is notified.
	 *
	 * \param progress Information about the progress of the operation.
	 * The actual type of the progress depends on the asynchronous operation
	 * and should be documented by the creator of the Promise.
	 *
	 * \sa Deferred::notified()
	 */
	void notified(const QVariant& progress) const;


protected:

	/*! Creates a Promise object for a Deferred.
	 *
	 * \param deferred The Deferred which should be represented by the Promise.
	 */
	Promise(Deferred::Ptr deferred);

	/*! Convenience constructor to create a resolved or rejected Promise.
	 *
	 * Creates a Deferred, creates a Promise on that Deferred and
	 * resolves or rejects it with \p data depending on \p state.
	 *
	 * \param state Defines whether the created Promise's Deferred
	 * should resolved or rejected.
	 * \param data The value or rejection reason according to \p state.
	 */
	Promise(Deferred::State state, const QVariant& data);

	/*! The Deferred represented by this Promise.
	 */
	Deferred::Ptr m_deferred;


private:

	template<typename NullCallbackFunc, typename std::enable_if<std::is_same<NullCallbackFunc, std::nullptr_t>::value>::type* = nullptr>
	Ptr callCallback(NullCallbackFunc&&) const;
	template<typename VoidCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value>::type* = nullptr>
	Ptr callCallback(VoidCallbackFunc&& func) const;
	template<typename VariantCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value>::type* = nullptr>
	Ptr callCallback(VariantCallbackFunc&& func) const;
	template<typename PromiseCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value>::type* = nullptr>
	Ptr callCallback(PromiseCallbackFunc&& func) const;

	template <typename NullCallbackFunc, typename std::enable_if<std::is_same<NullCallbackFunc, std::nullptr_t>::value>::type* = nullptr>
	static ChildDeferred::WrappedCallbackFunc createCallbackWrapper(ChildDeferred* newDeferred, NullCallbackFunc func, Deferred::State state);
	template <typename VoidCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value>::type* = nullptr>
	static ChildDeferred::WrappedCallbackFunc createCallbackWrapper(ChildDeferred* newDeferred, VoidCallbackFunc func, Deferred::State state);
	template<typename VariantCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value>::type* = nullptr>
	static ChildDeferred::WrappedCallbackFunc createCallbackWrapper(ChildDeferred* newDeferred, VariantCallbackFunc func, Deferred::State state);
	template<typename PromiseCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value>::type* = nullptr>
	static ChildDeferred::WrappedCallbackFunc createCallbackWrapper(ChildDeferred* newDeferred, PromiseCallbackFunc func, Deferred::State state);

	template <typename NullCallbackFunc, typename std::enable_if<std::is_same<NullCallbackFunc, std::nullptr_t>::value>::type* = nullptr>
	static ChildDeferred::WrappedCallbackFunc createNotifyCallbackWrapper(ChildDeferred* newDeferred, NullCallbackFunc func);
	template <typename VoidCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value>::type* = nullptr>
	static ChildDeferred::WrappedCallbackFunc createNotifyCallbackWrapper(ChildDeferred* newDeferred, VoidCallbackFunc func);
	template<typename VariantCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value>::type* = nullptr>
	static ChildDeferred::WrappedCallbackFunc createNotifyCallbackWrapper(ChildDeferred* newDeferred, VariantCallbackFunc func);
	template<typename PromiseCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value>::type* = nullptr>
	static ChildDeferred::WrappedCallbackFunc createNotifyCallbackWrapper(ChildDeferred* newDeferred, PromiseCallbackFunc func);


	template<typename PromiseContainer>
	static Ptr all_impl(const PromiseContainer& promises);
	template<typename PromiseContainer>
	static Ptr any_impl(const PromiseContainer& promises);


};

}  // namespace QtPromise

#include "Promise_impl.h"

/*! Returns the hash value for a Promise smart pointer.
 * @param promisePtr The QSharedPointer who's hash value should be returned.
 * @param seed The seed used for the calculation.
 * @return The hash value based on the address of the pointer.
 */
uint qHash(const QtPromise::Promise::Ptr& promisePtr, uint seed = 0);

#endif /* QTPROMISE_PROMISE_H_ */
