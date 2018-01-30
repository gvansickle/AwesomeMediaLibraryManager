/*! \file
 *
 * \date Created on: 25.09.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_PROMISE_IMPL_H_
#define QTPROMISE_PROMISE_IMPL_H_

#include <cstddef>

namespace QtPromise {

template<typename ResolvedFunc, typename RejectedFunc, typename NotifiedFunc>
Promise::Ptr Promise::then(ResolvedFunc&& resolvedCallback, RejectedFunc&& rejectedCallback, NotifiedFunc&& notifiedCallback) const
{


#if __cplusplus >= 201703L
	static_assert(   std::is_same<ResolvedFunc, std::nullptr_t>::value
	              || std::is_invocable_r<void, ResolvedFunc, const QVariant&>::value
	              || std::is_invocable_r<QVariant, ResolvedFunc, const QVariant&>::value
	              || std::is_invocable_r<Promise::Ptr, ResolvedFunc, const QVariant&>::value,
	              "resolvedCallback must be invocable with a const QVariant& and return void, QVariant or Promise::Ptr "
	              "or it must be nullptr");
	static_assert(   std::is_same<RejectedFunc, std::nullptr_t>::value
	              || std::is_invocable_r<void, RejectedFunc, const QVariant&>::value
	              || std::is_invocable_r<QVariant, RejectedFunc, const QVariant&>::value
	              || std::is_invocable_r<Promise::Ptr, RejectedFunc, const QVariant&>::value,
		          "rejectedCallback must be invocable with a const QVariant& and return void, QVariant or Promise::Ptr "
		          "or it must be nullptr");
	static_assert(   std::is_same<NotifiedFunc, std::nullptr_t>::value
	              || std::is_invocable_r<void, NotifiedFunc, const QVariant&>::value
	              || std::is_invocable_r<QVariant, NotifiedFunc, const QVariant&>::value
	              || std::is_invocable_r<Promise::Ptr, NotifiedFunc, const QVariant&>::value,
		          "notifiedCallback must be invocable with a const QVariant& and return void, QVariant or Promise::Ptr "
		          "or it must be nullptr");
#endif

	switch(this->state())
	{
	case Deferred::Resolved:
		return callCallback(std::forward<ResolvedFunc>(resolvedCallback));
	case Deferred::Rejected:
		return callCallback(std::forward<RejectedFunc>(rejectedCallback));
	case Deferred::Pending:
	default:
		ChildDeferred::Ptr newDeferred = ChildDeferred::create(m_deferred);

		newDeferred->connectParent(m_deferred,       createCallbackWrapper(newDeferred.data(), std::forward<ResolvedFunc>(resolvedCallback), Deferred::Resolved),
		                                             createCallbackWrapper(newDeferred.data(), std::forward<RejectedFunc>(rejectedCallback), Deferred::Rejected),
		                                       createNotifyCallbackWrapper(newDeferred.data(), std::forward<NotifiedFunc>(notifiedCallback)));
		
		return create(newDeferred.staticCast<Deferred>());
	}
}

template<typename NullCallbackFunc, typename std::enable_if<std::is_same<NullCallbackFunc, std::nullptr_t>::value>::type*>
Promise::Ptr Promise::callCallback(NullCallbackFunc&&) const
{
	return create(m_deferred);
}

template<typename VoidCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value>::type*>
Promise::Ptr Promise::callCallback(VoidCallbackFunc&& func) const
{
	func(m_deferred->data());
	return create(m_deferred);
}

template<typename VariantCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value>::type*>
Promise::Ptr Promise::callCallback(VariantCallbackFunc&& func) const
{
	QVariant newValue(func(m_deferred->data()));
	Deferred::Ptr newDeferred = Deferred::create();
	/* We always resolve the new deferred since returning a value from a RejectedFunc means
	 * "the problem has been resolved".
	 */
	newDeferred->resolve(newValue);
	return create(newDeferred);
}

template<typename PromiseCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value>::type*>
Promise::Ptr Promise::callCallback(PromiseCallbackFunc&& func) const
{
	return func(m_deferred->data());
}


template <typename NullCallbackFunc, typename std::enable_if<std::is_same<NullCallbackFunc, std::nullptr_t>::value>::type*>
ChildDeferred::WrappedCallbackFunc Promise::createCallbackWrapper(ChildDeferred* newDeferred, NullCallbackFunc, Deferred::State state)
{
	Q_ASSERT_X(state != Deferred::Pending, "Promise::createCallbackWrapper()", "state must not be Pending (this is a bug in QtPromise)");
	using namespace std::placeholders;

	if (state == Deferred::Resolved)
		return std::bind(&ChildDeferred::resolve, newDeferred, _1);
	else // state == Deferred::Rejected
		return std::bind(&ChildDeferred::reject, newDeferred, _1);
}

template <typename VoidCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value>::type*>
ChildDeferred::WrappedCallbackFunc Promise::createCallbackWrapper(ChildDeferred* newDeferred, VoidCallbackFunc func, Deferred::State state)
{
	Q_ASSERT_X(state != Deferred::Pending, "Promise::createCallbackWrapper()", "state must not be Pending (this is a bug in QtPromise)");
	if (state == Deferred::Resolved)
		return [newDeferred, func](const QVariant& data) {
		func(data);
		newDeferred->resolve(data);
	};
	else // state == Deferred::Rejected
		return [newDeferred, func](const QVariant& data) {
		func(data);
		newDeferred->reject(data);
	};
}

template<typename VariantCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value>::type*>
ChildDeferred::WrappedCallbackFunc Promise::createCallbackWrapper(ChildDeferred* newDeferred, VariantCallbackFunc func, Deferred::State)
{
	return [newDeferred, func](const QVariant& data) {
		QVariant newValue = QVariant::fromValue(func(data));
		/* We always resolve the new deferred since returning a value from a RejectedFunc means
		 * "the problem has been resolved".
		 */
		newDeferred->resolve(newValue);
	};
}

template<typename PromiseCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value>::type*>
ChildDeferred::WrappedCallbackFunc Promise::createCallbackWrapper(ChildDeferred* newDeferred, PromiseCallbackFunc func, Deferred::State)
{
	return [newDeferred, func](const QVariant& data) {
		Deferred::Ptr intermedDeferred = func(data)->m_deferred;
		switch (intermedDeferred->state())
		{
		case Deferred::Resolved:
			newDeferred->resolve(intermedDeferred->data());
			break;
		case Deferred::Rejected:
			newDeferred->reject(intermedDeferred->data());
			break;
		case Deferred::Pending:
		default:
			// Disconnect and remove all previous parents
			newDeferred->removeParents(true);
			// The intermedDeferred is the new parent
			newDeferred->setParent(intermedDeferred);
			using namespace std::placeholders;
			newDeferred->connectParent(intermedDeferred, std::bind(&ChildDeferred::resolve, newDeferred, _1),
			                                             std::bind(&ChildDeferred::reject, newDeferred, _1),
			                                             std::bind(&ChildDeferred::notify, newDeferred, _1));
			break;
		}
	};
}

template <typename NullCallbackFunc, typename std::enable_if<std::is_same<NullCallbackFunc, std::nullptr_t>::value>::type*>
ChildDeferred::WrappedCallbackFunc Promise::createNotifyCallbackWrapper(ChildDeferred* newDeferred, NullCallbackFunc)
{
	using namespace std::placeholders;
	return std::bind(&ChildDeferred::notify, newDeferred, _1);
}

template <typename VoidCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<VoidCallbackFunc(const QVariant&)>::type, void>::value>::type*>
ChildDeferred::WrappedCallbackFunc Promise::createNotifyCallbackWrapper(ChildDeferred* newDeferred, VoidCallbackFunc func)
{
	return [newDeferred, func](const QVariant& data) {
		func(data);
		newDeferred->notify(data);
	};
}

template<typename VariantCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<VariantCallbackFunc(const QVariant&)>::type, QVariant>::value>::type*>
ChildDeferred::WrappedCallbackFunc Promise::createNotifyCallbackWrapper(ChildDeferred* newDeferred, VariantCallbackFunc func)
{
	return [newDeferred, func](const QVariant& data) {
		newDeferred->notify(func(data));
	};
}

template<typename PromiseCallbackFunc, typename std::enable_if<std::is_convertible<typename std::result_of<PromiseCallbackFunc(const QVariant&)>::type, Promise::Ptr>::value>::type*>
ChildDeferred::WrappedCallbackFunc Promise::createNotifyCallbackWrapper(ChildDeferred* newDeferred, PromiseCallbackFunc func)
{
	return [newDeferred, func](const QVariant& data) {
		Deferred::Ptr intermedDeferred = func(data)->m_deferred;
		switch (intermedDeferred->state())
		{
		case Deferred::Pending:
		{
			newDeferred->addParent(intermedDeferred);
			QObject::connect(intermedDeferred.data(), &Deferred::resolved, newDeferred, &Deferred::notify);
			QObject::connect(intermedDeferred.data(), &Deferred::notified, newDeferred, &Deferred::notify);
			break;
		}
		case Deferred::Resolved:
			newDeferred->notify(intermedDeferred->data());
			break;
		case Deferred::Rejected:
		default:
			break;
		}
	};
}

template<typename PromiseContainer>
Promise::Ptr Promise::all_impl(const PromiseContainer& promises)
{
	QVector<Deferred::Ptr> deferreds;
	for (Promise::Ptr promise : promises)
		deferreds.append(promise->m_deferred);
	ChildDeferred::Ptr combinedDeferred = ChildDeferred::create(deferreds, true);

	QObject::connect(combinedDeferred.data(), &ChildDeferred::parentsResolved, combinedDeferred.data(), &Deferred::resolve);
	QObject::connect(combinedDeferred.data(), &ChildDeferred::parentRejected, combinedDeferred.data(), &Deferred::reject);

	return create(combinedDeferred);
}

template<typename PromiseContainer>
Promise::Ptr Promise::any_impl(const PromiseContainer& promises)
{
	QVector<Deferred::Ptr> deferreds;
	for (Promise::Ptr promise : promises)
		deferreds.append(promise->m_deferred);
	ChildDeferred::Ptr combinedDeferred = ChildDeferred::create(deferreds, true);

	QObject::connect(combinedDeferred.data(), &ChildDeferred::parentResolved, combinedDeferred.data(), &Deferred::resolve);
	QObject::connect(combinedDeferred.data(), &ChildDeferred::parentsRejected, combinedDeferred.data(), &Deferred::reject);

	return create(combinedDeferred);
}

} // namespace QtPromise

#endif /* QTPROMISE_PROMISE_IMPL_H_ */
