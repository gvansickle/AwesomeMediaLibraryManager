/*! \file
 * \date Created on: 27.09.2017
 * \author jochen.ulrich
 */

#ifndef QTPROMISE_DEFERRED_IMPL_H_
#define QTPROMISE_DEFERRED_IMPL_H_

#include <QMetaMethod>

#include <utility>

namespace QtPromise {

template<typename ValueType, typename Signal>
void Deferred::resolveAndEmit(const ValueType& value, Signal&& signal)
{
	m_isInSignalHandler.fetchAndAddAcquire(1);
	if (this->resolve(QVariant::fromValue(value)))
		QMetaMethod::fromSignal(std::forward<Signal>(signal)).invoke(this, Q_ARG(ValueType, value));
	m_isInSignalHandler.fetchAndSubRelease(1);
}

template<typename ReasonType, typename Signal>
void Deferred::rejectAndEmit(const ReasonType& reason, Signal&& signal)
{
	m_isInSignalHandler.fetchAndAddAcquire(1);
	if (this->reject(QVariant::fromValue(reason)))
		QMetaMethod::fromSignal(std::forward<Signal>(signal)).invoke(this, Q_ARG(ReasonType, reason));
	m_isInSignalHandler.fetchAndSubRelease(1);
}

template<typename ProgressType, typename Signal>
void Deferred::notifyAndEmit(const ProgressType& progress, Signal&& signal)
{
	m_isInSignalHandler.fetchAndAddAcquire(1);
	if (this->notify(QVariant::fromValue(progress)))
		QMetaMethod::fromSignal(std::forward<Signal>(signal)).invoke(this, Q_ARG(ProgressType, progress));
	m_isInSignalHandler.fetchAndSubRelease(1);
}

}  // namespace QtPromise


#endif /* QTPROMISE_DEFERRED_IMPL_H_ */
