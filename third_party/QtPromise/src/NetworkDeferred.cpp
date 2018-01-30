#include "NetworkDeferred.h"
#include <QTimer>

namespace QtPromise {

NetworkDeferred::NetworkDeferred(QNetworkReply* reply)
	: Deferred(), m_reply(reply), m_lock(QMutex::Recursive)
{
	registerMetaTypes();

	m_reply->setParent(this);

	/* In case the reply is already finished, we cannot rely on the
	 * QNetworkReply::finished() signal as it could have been fired already.
	 * Therefore, we call replyFinished() asynchronously. So no matter if
	 * QNetworkReply::finished() has already fired or not, the NetworkDeferred
	 * will be resolved/rejected when control returns to the event loop.
	 */
	if (reply->isFinished())
		QTimer::singleShot(0, this, &NetworkDeferred::replyFinished);
	else
		connect(m_reply, &QNetworkReply::finished, this, &NetworkDeferred::replyFinished);

	/* For the progress signals, we stay in line with QNetworkReply:
	 * if they have been fired before the caller connected slots to them,
	 * the caller cannot get progress notifications anymore.
	 *
	 * In case the reply is already finished but the progress signals are still in the
	 * event queue, the notifications will be triggered before the deferred is resolved/reject
	 * since the call to NetworkDeferred::replyFinished() comes later in the event queue.
	 * Hence, the notifications will work as expected.
	 */
	connect(m_reply, &QNetworkReply::downloadProgress, this, &NetworkDeferred::replyDownloadProgress);
	connect(m_reply, &QNetworkReply::uploadProgress, this, &NetworkDeferred::replyUploadProgress);
	connect(m_reply, &QObject::destroyed, this, &NetworkDeferred::replyDestroyed);
}

NetworkDeferred::~NetworkDeferred()
{
	checkDestructionInSignalHandler();
}

NetworkDeferred::Ptr NetworkDeferred::create(QNetworkReply* reply)
{
	return Ptr(new NetworkDeferred(reply));
}

void NetworkDeferred::registerMetaTypes()
{
	static QMutex metaTypesLock;
	static bool registered = false;

	QMutexLocker locker(&metaTypesLock);
	if (!registered)
	{
		qRegisterMetaType<ReplyData>();
		QMetaType::registerEqualsComparator<ReplyData>();
		qRegisterMetaType<ReplyData>("NetworkDeferred::ReplyData");
		qRegisterMetaType<ReplyData>("QtPromise::NetworkDeferred::ReplyData");
		qRegisterMetaType<Error>();
		QMetaType::registerEqualsComparator<Error>();
		qRegisterMetaType<Error>("NetworkDeferred::Error");
		qRegisterMetaType<Error>("QtPromise::NetworkDeferred::Error");
		qRegisterMetaType<Progress>();
		QMetaType::registerEqualsComparator<Progress>();
		qRegisterMetaType<Progress>("NetworkDeferred::Progress");
		qRegisterMetaType<Progress>("QtPromise::NetworkDeferred::Progress");
		qRegisterMetaType<ReplyProgress>();
		QMetaType::registerEqualsComparator<ReplyProgress>();
		qRegisterMetaType<ReplyProgress>("NetworkDeferred::ReplyProgress");
		qRegisterMetaType<ReplyProgress>("QtPromise::NetworkDeferred::ReplyProgress");
		registered = true;
	}
}

void NetworkDeferred::replyFinished()
{
	QMutexLocker locker(&m_lock);

	if (m_reply->isReadable())
	{
		// Save reply data since it will be removed from QNetworkReply when calling readAll()
		m_buffer = m_reply->readAll();
	}

	ReplyData replyData = this->replyData();
	if (m_reply->error() != QNetworkReply::NoError)
	{
		m_error.code = m_reply->error();
		m_error.message = m_reply->errorString();
		m_error.replyData = replyData;
		this->rejectAndEmit(m_error, &NetworkDeferred::rejected);
	}
	else
	{
		this->resolveAndEmit(replyData, &NetworkDeferred::resolved);
	}
}

void NetworkDeferred::replyDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	QMutexLocker locker(&m_lock);
	/* This check is added since there is some unexpected behavior with upload progress
	 * signals and it's not clear if the same could happen for download progress signals
	 * as well.
	 */
	if (bytesTotal == 0 && m_progress.download.total > 0)
		return;
	m_progress.download.current = bytesReceived;
	m_progress.download.total = bytesTotal;
	this->notifyAndEmit(m_progress, &NetworkDeferred::notified);
}

void NetworkDeferred::replyUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
	QMutexLocker locker(&m_lock);
	/* For some reason, Qt emits an upload progress signal with both values set to 0
	 * after the download of the reply finished although we had an successful upload.
	 */
	if (bytesTotal == 0 && m_progress.upload.total > 0)
		return;
	m_progress.upload.current = bytesSent;
	m_progress.upload.total = bytesTotal;
	this->notifyAndEmit(m_progress, &NetworkDeferred::notified);
}

void NetworkDeferred::replyDestroyed(QObject* reply)
{
	/* Do NOT access m_reply in this method since
	 * its QNetworkReply members have already been destructed
	 * (this method is called from ~QObject()).
	 */
	QMutexLocker locker(&m_lock);
	if (this->state() == Deferred::Pending)
	{
		QString errorMessage = QString("QNetworkReply %1 destroyed while owning NetworkDeferred %2 still pending")
		.arg(pointerToQString(reply))
		.arg(pointerToQString(this));
		qDebug("%s", qUtf8Printable(errorMessage));

		m_error.code = static_cast<QNetworkReply::NetworkError>(-1);
		m_error.message = errorMessage;
		m_error.replyData = ReplyData(m_buffer, nullptr);

		this->rejectAndEmit(m_error, &NetworkDeferred::rejected);
	}
	m_reply = nullptr;
	m_error.replyData.qReply = nullptr;
}



}  // namespace QtPromise
