
#include <QtTest>
#include <QtDebug>
#include <QSignalSpy>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include "NetworkPromise.h"


namespace QtPromise
{
namespace Tests
{

/*! \brief Unit tests for the NetworkPromise class.
 *
 * \author jochen.ulrich
 */
class NetworkPromiseTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void testSuccess();
	void testFail();
	void testHttp();
	void testUpload();
	void testFinishedReply_data();
	void testFinishedReply();
	void testDestroyReply();
	void testAbortReply();
	void testCachedData();
	void testFinishedDeferred_data();
	void testFinishedDeferred();

private:
	struct PromiseSpies
	{
		PromiseSpies(NetworkPromise::Ptr promise);

		QSignalSpy resolved;
		QSignalSpy baseResolved;
		QSignalSpy rejected;
		QSignalSpy baseRejected;
		QSignalSpy notified;
		QSignalSpy baseNotified;
	};
};


//####### Helpers #######
NetworkPromiseTest::PromiseSpies::PromiseSpies(NetworkPromise::Ptr promise)
	: resolved(promise.data(), &NetworkPromise::resolved),
	  rejected(promise.data(), &NetworkPromise::rejected),
	  notified(promise.data(), &NetworkPromise::notified),
	  baseResolved(promise.data(), &Promise::resolved),
	  baseRejected(promise.data(), &Promise::rejected),
	  baseNotified(promise.data(), &Promise::notified)
{
}


//####### Tests #######
/*! \test Tests a successful request with a NetworkPromise.
 */
void NetworkPromiseTest::testSuccess()
{
	QString dataPath = QFINDTESTDATA("data/DummyData.txt");
	QFile dataFile(dataPath);
	dataFile.open(QIODevice::ReadOnly);
	QByteArray expectedData = dataFile.readAll();
	dataFile.close();

	QNetworkAccessManager qnam;
	QNetworkRequest request(QUrl::fromLocalFile(dataPath));
	QNetworkReply* reply = qnam.get(request);

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	PromiseSpies spies(promise);

	QCOMPARE(promise->replyData(), NetworkDeferred::ReplyData(QByteArray(), reply));
	QCOMPARE(promise->error(), NetworkDeferred::Error());

	// "Execute the request"
	QVERIFY(spies.resolved.wait());

	// Check resolved data
	QCOMPARE(spies.resolved.count(), 1);
	NetworkDeferred::ReplyData replyData = spies.resolved.first().first().value<NetworkDeferred::ReplyData>();
	QCOMPARE(replyData.data, expectedData);
	QCOMPARE(replyData.qReply, reply);
	QCOMPARE(promise->replyData(), replyData);
	QCOMPARE(promise->error(), NetworkDeferred::Error());

	QCOMPARE(spies.baseResolved.count(), 1);
	QCOMPARE(spies.baseResolved.first().first().value<NetworkDeferred::ReplyData>(), replyData);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.baseRejected.count(), 0);
	QVERIFY(spies.notified.count() > 0);
	NetworkDeferred::ReplyProgress progress = spies.notified.first().first().value<NetworkDeferred::ReplyProgress>();
	QVERIFY(progress.download.current > 0);
	QVERIFY(progress.download.total > 0);
	QVERIFY(spies.baseNotified.count() > 0);
	QCOMPARE(spies.baseNotified.first().first().value<NetworkDeferred::ReplyProgress>(), progress);
}

/*! \test Tests a failed request with a NetworkPromise.
 */
void NetworkPromiseTest::testFail()
{
	QString dataPath("A_File_that_doesnt_exist_9831874375377535764532134848337483.txt");
	QVERIFY(!QFile::exists(dataPath));

	QNetworkAccessManager qnam;
	QNetworkRequest request(QUrl::fromLocalFile(dataPath));
	QNetworkReply* reply = qnam.get(request);

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	PromiseSpies spies(promise);

	// "Execute the request"
	QVERIFY(spies.rejected.wait());

	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.baseResolved.count(), 0);

	// Check rejected data
	QCOMPARE(spies.rejected.count(), 1);
	NetworkDeferred::Error error = spies.rejected.first().first().value<NetworkDeferred::Error>();
	QCOMPARE(error.code, QNetworkReply::ContentNotFoundError);
	qDebug() << "Error code:" << error.code;
	qDebug() << "Error message:" << error.message;
	QVERIFY(!error.message.isEmpty());
	QCOMPARE(promise->replyData(), NetworkDeferred::ReplyData(QByteArray(), reply));
	QCOMPARE(promise->error(), error);

	QCOMPARE(spies.baseRejected.count(), 1);
	QCOMPARE(spies.baseRejected.first().first().value<NetworkDeferred::Error>(), error);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.baseNotified.count(), 0);
}

/*! \test Tests a HTTP request with a NetworkPromise.
 */
void NetworkPromiseTest::testHttp()
{
	QNetworkAccessManager qnam;
	if(qnam.networkAccessible() == QNetworkAccessManager::NotAccessible)
		QSKIP("Network not accessible");

	QNetworkRequest request(QUrl("http://www.google.com"));
	QNetworkReply* reply = qnam.get(request);

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	PromiseSpies spies(promise);
	QTRY_VERIFY_WITH_TIMEOUT(reply->isFinished(), 20 * 1000);
	if (reply->error() == QNetworkReply::NoError)
		QCOMPARE(spies.resolved.count(), 1);
	else
		QCOMPARE(spies.rejected.count(), 1);
}

void NetworkPromiseTest::testUpload()
{
	QNetworkAccessManager qnam;
	QNetworkRequest request(QUrl("https://eu.httpbin.org/post"));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
	QString data("foo bar");
	QNetworkReply* reply = qnam.post(request, data.toUtf8());

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	PromiseSpies spies(promise);
	QTRY_VERIFY_WITH_TIMEOUT(reply->isFinished(), 20 * 1000);
	QVERIFY(spies.notified.count() > 0);
	QVERIFY(spies.notified.last().first().value<NetworkDeferred::ReplyProgress>().upload.current > 0);
	QJsonDocument json = QJsonDocument::fromJson(promise->replyData().data);
	QCOMPARE(json.object()["data"].toString(), data);
}

/*! Provides the data for the testFinishedReply() test.
 */
void NetworkPromiseTest::testFinishedReply_data()
{
	/* Note that this data method is also used for
	 * the testFinishedDeferred()!
	 */

	QTest::addColumn<QString>("dataPath");
	QTest::addColumn<bool>("expectResolve");

	QTest::newRow("success") << QFINDTESTDATA("data/DummyData.txt") << true;
	QTest::newRow("fail") << "Non_existent_File_45389045765.txt" << false;
}

/*! \test Tests the NetworkPromise with a QNetworkReply which has finished
 * and emitted it's events before the NetworkPromise is created.
 */
void NetworkPromiseTest::testFinishedReply()
{
	QFETCH(QString, dataPath);
	QFETCH(bool, expectResolve);

	QByteArray expectedData;
	QFile dataFile(dataPath);
	if (dataFile.open(QIODevice::ReadOnly))
	{
		expectedData = dataFile.readAll();
		dataFile.close();
	}

	QNetworkAccessManager qnam;
	QNetworkRequest request(QUrl::fromLocalFile(dataPath));
	QNetworkReply* reply = qnam.get(request);

	QTRY_VERIFY(reply->isFinished());
	QTest::qWait(50); // Make sure the events (signals) of the reply are processed

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	PromiseSpies spies(promise);

	bool resolvedCalled = false;
	QVariant resolvedData;
	Promise::Ptr newPromise = promise->then([&](const QVariant& data) {
		resolvedCalled = true;
		resolvedData = data;
	});

	if (expectResolve)
		QVERIFY(spies.resolved.wait(500));
	else
		QVERIFY(spies.rejected.wait(500));
	QCOMPARE(spies.resolved.count(), expectResolve? 1 : 0);
	QCOMPARE(spies.baseResolved.count(), expectResolve? 1 : 0);
	QCOMPARE(spies.rejected.count(), expectResolve? 0 : 1);
	QCOMPARE(spies.baseRejected.count(), expectResolve? 0 : 1);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.baseNotified.count(), 0);
	QCOMPARE(resolvedCalled, expectResolve);
	QCOMPARE(resolvedData.value<NetworkDeferred::ReplyData>().data, expectedData);
}

/*! \test Tests destroying a QNetworkReply while it is attached to a NetworkPromise.
 */
void NetworkPromiseTest::testDestroyReply()
{
	QNetworkAccessManager qnam;
	if(qnam.networkAccessible() == QNetworkAccessManager::NotAccessible)
		QSKIP("Network not accessible");

	QNetworkRequest request(QUrl("http://www.google.com"));
	QNetworkReply* reply = qnam.get(request);

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	QCOMPARE(promise->state(), Deferred::Pending);

	delete reply;

	QCOMPARE(promise->state(), Deferred::Rejected);
	QCOMPARE(static_cast<int>(promise->error().code), -1);
	QCOMPARE(promise->replyData().qReply, static_cast<QNetworkReply*>(nullptr));
}

/*! \test Tests aborting a QNetworkReply while it is attached to a NetworkPromise.
 */
void NetworkPromiseTest::testAbortReply()
{
	QNetworkAccessManager qnam;
	if(qnam.networkAccessible() == QNetworkAccessManager::NotAccessible)
		QSKIP("Network not accessible");

	QNetworkRequest request(QUrl("http://www.google.com"));
	QNetworkReply* reply = qnam.get(request);

	NetworkPromise::Ptr promise = NetworkPromise::create(reply);

	QCOMPARE(promise->state(), Deferred::Pending);

	reply->abort();

	QCOMPARE(promise->state(), Deferred::Rejected);
	QCOMPARE(promise->error().code, QNetworkReply::OperationCanceledError);
	QCOMPARE(promise->replyData().qReply, reply);
}

/*! \test Tests the NetworkPromise with cached data.
 */
void NetworkPromiseTest::testCachedData()
{
	QTemporaryDir tempDir;
	if (!tempDir.isValid())
		QFAIL("Could not create temporary directory");

	QNetworkAccessManager qnam;
	QNetworkDiskCache diskCache(&qnam);
	diskCache.setCacheDirectory(tempDir.path());
	qnam.setCache(&diskCache);
	
	// Load the data for the first time to cache it
	QString dataPath = QFINDTESTDATA("data/DummyData.txt");
	QNetworkRequest request(QUrl::fromLocalFile(dataPath));
	QScopedPointer<QNetworkReply> firstReply{qnam.get(request)};
	QTRY_VERIFY(firstReply->isFinished());
	
	// Load the data for the second time.
	QNetworkReply* secondReply = qnam.get(request);
	NetworkPromise::Ptr promise = NetworkPromise::create(secondReply);
	QTRY_VERIFY(secondReply->isFinished());
	QTRY_COMPARE(promise->state(), Deferred::Resolved);
}

/*! Provides the data for the testFinishedDeferred() test.
 */
void NetworkPromiseTest::testFinishedDeferred_data()
{
	// Reuse the data from the testFinishedReply() test.
	testFinishedReply_data();
}

/*! \test Tests the NetworkPromise with a NetworkDeferred which has finished
 * and emitted it's events before the NetworkPromise is created.
 */
void NetworkPromiseTest::testFinishedDeferred()
{
	QFETCH(QString, dataPath);
	QFETCH(bool, expectResolve);

	QByteArray expectedData;
	QFile dataFile(dataPath);
	if (dataFile.open(QIODevice::ReadOnly))
	{
		expectedData = dataFile.readAll();
		dataFile.close();
	}

	QNetworkAccessManager qnam;
	QNetworkRequest request(QUrl::fromLocalFile(dataPath));
	QNetworkReply* reply = qnam.get(request);

	NetworkDeferred::Ptr deferred = NetworkDeferred::create(reply);

	QTRY_VERIFY(reply->isFinished());
	QTRY_COMPARE(deferred->state(), expectResolve? Deferred::Resolved : Deferred::Rejected);

	NetworkPromise::Ptr promise = NetworkPromise::create(deferred);

	PromiseSpies spies(promise);

	bool resolvedCalled = false;
	QVariant resolvedData;
	Promise::Ptr newPromise = promise->then([&](const QVariant& data) {
		resolvedCalled = true;
		resolvedData = data;
	});

	if (expectResolve)
		QVERIFY(spies.resolved.wait(500));
	else
		QVERIFY(spies.rejected.wait(500));
	QCOMPARE(spies.resolved.count(), expectResolve? 1 : 0);
	QCOMPARE(spies.baseResolved.count(), expectResolve? 1 : 0);
	QCOMPARE(spies.rejected.count(), expectResolve? 0 : 1);
	QCOMPARE(spies.baseRejected.count(), expectResolve? 0 : 1);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.baseNotified.count(), 0);
	QCOMPARE(resolvedCalled, expectResolve);
	QCOMPARE(resolvedData.value<NetworkDeferred::ReplyData>().data, expectedData);
}


}  // namespace Tests
}  // namespace QtPromise



QTEST_MAIN(QtPromise::Tests::NetworkPromiseTest)
#include "NetworkPromiseTest.moc"


