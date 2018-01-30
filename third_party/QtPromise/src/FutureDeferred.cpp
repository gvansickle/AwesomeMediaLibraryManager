#include "FutureDeferred.h"

#ifndef QT_NO_QFUTURE

namespace QtPromise
{

FutureDeferred::~FutureDeferred()
{
	checkDestructionInSignalHandler();
}

void FutureDeferred::registerMetaTypes()
{
	static QMutex metaTypesLock;
	static bool registered = false;

	QMutexLocker locker(&metaTypesLock);
	if (!registered)
	{
		qRegisterMetaType<Progress>();
		QMetaType::registerEqualsComparator<Progress>();
		qRegisterMetaType<Progress>("FutureDeferred::Progress");
		qRegisterMetaType<Progress>("QtPromise::FutureDeferred::Progress");
		registered = true;
	}
}

void FutureDeferred::futureFinished(const QVariantList& results)
{
	QMutexLocker locker(&m_lock);
	m_results = results;
	this->resolveAndEmit(m_results, &FutureDeferred::resolved);
}

void FutureDeferred::futureCanceled(const QVariantList& results)
{
	QMutexLocker locker(&m_lock);
	m_results = results;
	this->rejectAndEmit(m_results, &FutureDeferred::rejected);
}

void FutureDeferred::futureProgressRangeChanged(int min, int max)
{
	QMutexLocker locker(&m_lock);
	m_progress.min = min;
	m_progress.max = max;
	this->notifyAndEmit(m_progress, &FutureDeferred::notified);
}

void FutureDeferred::futureProgressTextChanged(const QString& text)
{
	QMutexLocker locker(&m_lock);
	m_progress.text = text;
	this->notifyAndEmit(m_progress, &FutureDeferred::notified);
}

void FutureDeferred::futureProgressValueChanged(int value)
{
	QMutexLocker locker(&m_lock);
	m_progress.value = value;
	this->notifyAndEmit(m_progress, &FutureDeferred::notified);
}


} /* namespace QtPromise */

#endif /* QT_NO_QTFUTURE */
