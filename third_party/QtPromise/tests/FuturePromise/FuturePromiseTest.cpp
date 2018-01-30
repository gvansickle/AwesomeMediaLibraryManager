
#include <QtTest>
#include <QtDebug>
#include <QSignalSpy>
#include <QtConcurrent>
#include <functional>
#include "FuturePromise.h"


namespace QtPromise
{
namespace Tests
{

/*! \brief Unit tests for the FuturePromise class.
 *
 * \author jochen.ulrich
 */
class FuturePromiseTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void testBasicFuture();
	void testMultipleResults();
	void testProgressReporting();
	void testProgressText();
	void testCancel();
	void testFinishedFuture_data();
	void testFinishedFuture();
	void testFinishedDeferred_data();
	void testFinishedDeferred();

private:
	struct PromiseSpies
	{
		PromiseSpies(FuturePromise::Ptr promise);

		QSignalSpy resolved;
		QSignalSpy baseResolved;
		QSignalSpy rejected;
		QSignalSpy baseRejected;
		QSignalSpy notified;
		QSignalSpy baseNotified;
	};
};


//####### Helpers #######
FuturePromiseTest::PromiseSpies::PromiseSpies(FuturePromise::Ptr promise)
	: resolved(promise.data(), &FuturePromise::resolved),
	  rejected(promise.data(), &FuturePromise::rejected),
	  notified(promise.data(), &FuturePromise::notified),
	  baseResolved(promise.data(), &Promise::resolved),
	  baseRejected(promise.data(), &Promise::rejected),
	  baseNotified(promise.data(), &Promise::notified)
{
}

void FuturePromiseTest::initTestCase()
{
	/* Ensure we have at most one worker thread.
	 * Else it is impossible to have the QFutures report progress in a
	 * predictable manner.
	 */
	QThreadPool::globalInstance()->setMaxThreadCount(2);
}


//####### Tests #######
/*! \test Tests a FuturePromise with a QFuture created from QtConcurrent::run().
 */
void FuturePromiseTest::testBasicFuture()
{
	int returnValue = 0;
	QWaitCondition waitCond;
	QFuture<int> future = QtConcurrent::run([&]() -> int {
		QMutex mutex;
		QMutexLocker locker(&mutex);
		waitCond.wait(&mutex);
		return returnValue;
	});

	FuturePromise::Ptr promise = FuturePromise::create(future);

	PromiseSpies spies(promise);

	QTest::qWait(1 * 1000); // 1s
	QVERIFY(!future.isFinished());
	QCOMPARE(promise->state(), Deferred::Pending);

	returnValue = 42;
	waitCond.wakeAll();

	future.waitForFinished();
	QTRY_COMPARE(promise->state(), Deferred::Resolved);
	QCOMPARE(spies.baseResolved.count(), 1);
	QCOMPARE(spies.resolved.count(), 1);
	QCOMPARE(promise->results(), spies.resolved.first().first().toList());
	QCOMPARE(promise->results().first().value<int>(), returnValue);
}

/*! \test Tests FuturePromise with a QFuture which produces multiple results.
 */
void FuturePromiseTest::testMultipleResults()
{
	QList<int> input;
	input << 1 << 2 << 3;

	QMutex mutex;

	std::function<int(const int&)> mapFunction = [&](const int& value) -> int {
		QMutexLocker locker(&mutex);
		return value * 2;
	};

	// Prevent the thread from running before we set up the promise
	mutex.lock();

	QFuture<int> future = QtConcurrent::mapped(input, mapFunction);

	FuturePromise::Ptr promise = FuturePromise::create(future);

	PromiseSpies spies(promise);

	// Allow the thread to run
	mutex.unlock();

	QTRY_COMPARE(promise->state(), Deferred::Resolved);
	QVariantList expectedResults;
	expectedResults << 2 << 4 << 6;
	QCOMPARE(promise->results(), expectedResults);
	QCOMPARE(spies.baseResolved.count(), 1);
	QCOMPARE(spies.resolved.count(), 1);
	QCOMPARE(spies.resolved.first().first().toList(), promise->results());
	QCOMPARE(spies.baseResolved.first().first(), QVariant::fromValue(promise->results()));
}

/*! \test Tests rejection of a FuturePromise when the QFuture is cancelled.
 */
void FuturePromiseTest::testCancel()
{
	QList<int> input;
	input << 1 << 2 << 3;

	QWaitCondition waitCond;

	std::function<int(const int&)> mapFunction = [&](const int& value) -> int {
		QMutex mutex;
		QMutexLocker locker(&mutex);
		waitCond.wait(&mutex);
		return value * 2;
	};

	QFuture<int> future = QtConcurrent::mapped(input, mapFunction);

	FuturePromise::Ptr promise = FuturePromise::create(future);

	PromiseSpies spies(promise);

	QTest::qWait(1 * 500);

	waitCond.wakeOne();

	QTRY_COMPARE(future.resultCount(), 1);

	future.cancel();
	waitCond.wakeAll();

	QTRY_COMPARE(promise->state(), Deferred::Rejected);
	QCOMPARE(spies.baseRejected.count(), 1);
	QCOMPARE(spies.rejected.count(), 1);
	QCOMPARE(spies.rejected.first().first().toList(), promise->results());
	QCOMPARE(spies.baseRejected.first().first(), QVariant::fromValue(promise->results()));
	QCOMPARE(promise->results().count(), 1);
	QCOMPARE(promise->results().first().value<int>(), 2);
}

/*! \test Tests the notifications of a FuturePromise when the QFuture reports progress.
 */
void FuturePromiseTest::testProgressReporting()
{
	QList<int> input;
	input << 1 << 2 << 3;

	QMutex mutex;

	std::function<int(const int&)> mapFunction = [&](const int& value) -> int {
		QMutexLocker locker(&mutex);
		return value * 2;
	};

	// Prevent the thread from running before we set up the promise
	mutex.lock();

	QFuture<int> future = QtConcurrent::mapped(input, mapFunction);

	FuturePromise::Ptr promise = FuturePromise::create(future);

	PromiseSpies spies(promise);

	QCOMPARE(future.progressValue(), 0);
	QCOMPARE(future.progressMinimum(), 0);
	QCOMPARE(future.progressMaximum(), input.length());

	// The first two notifications inform about progress min/max and initial value (0)
	QTRY_COMPARE(spies.notified.count(), 2);
	FutureDeferred::Progress progress = spies.notified.at(0).first().value<FutureDeferred::Progress>();
	QCOMPARE(progress.min, 0);
	QCOMPARE(progress.max, input.length());
	QCOMPARE(progress.value, -1);

	progress = spies.notified.at(1).first().value<FutureDeferred::Progress>();
	QCOMPARE(progress.min, 0);
	QCOMPARE(progress.max, input.length());
	QCOMPARE(progress.value, 0);

	// Allow the thread to run
	mutex.unlock();

	QTRY_COMPARE(promise->state(), Deferred::Resolved);
	QVERIFY(spies.notified.count() > 2);
	progress = spies.notified.last().first().value<FutureDeferred::Progress>();
	QCOMPARE(progress.min, 0);
	QCOMPARE(progress.max, input.length());
	QCOMPARE(progress.value, input.length());
}

/*! \test Tests notifications of a FuturePromise when the QFuture updates
 * the progress text.
 *
 * Since the QtConcurrent algorithms don't update the progress text at the moment,
 * we fake it to get the coverage.
 */
void FuturePromiseTest::testProgressText()
{
	QList<int> input;
	input << 1 << 2 << 3;

	QMutex mutex;

	std::function<int(const int&)> mapFunction = [&](const int& value) -> int {
		QMutexLocker locker(&mutex);
		return value * 2;
	};

	// Prevent the thread from running before we set up the promise
	mutex.lock();

	QFuture<int> future = QtConcurrent::mapped(input, mapFunction);

	FutureDeferred::Ptr deferred = FutureDeferred::create(future);
	FuturePromise::Ptr promise = FuturePromise::create(deferred);

	PromiseSpies spies(promise);

	QTRY_VERIFY(spies.notified.count() > 0);
	int formerNotifiedCount = spies.notified.count();

	//####### FAKE
	QString fakeTextProgress{"dummy text progress"};
	QMetaObject::invokeMethod(deferred.data(), "futureProgressTextChanged",
	                          Q_ARG(QString, fakeTextProgress));
	//####### END OF FAKE

	QTRY_COMPARE(spies.notified.count(), formerNotifiedCount + 1);
	FutureDeferred::Progress expectedProgress;
	expectedProgress.min = 0;
	expectedProgress.max = input.length();
	expectedProgress.value = 0;
	expectedProgress.text = fakeTextProgress;
	QCOMPARE(spies.notified.last().first().value<FutureDeferred::Progress>(), expectedProgress);

	// Allow the thread to finish
	mutex.unlock();

	QTRY_COMPARE(promise->state(), Deferred::Resolved);
}

/*! Provides the data for the testFinishedFuture() test.
 */
void FuturePromiseTest::testFinishedFuture_data()
{
	/* Note that this data method is also used for
	 * the testFinishedDeferred()!
	 */

	QTest::addColumn<bool>("cancel");
	QTest::addColumn<QVariantList>("expectedResults");

	QTest::newRow("success") << false << (QVariantList() << 1 << 2 << 3);
	QTest::newRow("cancel") << true << QVariantList();
}

/*! \test Tests the FuturePromise with a QFuture which has finished
 * and emitted it's events before the FuturePromise is created.
 */
void FuturePromiseTest::testFinishedFuture()
{
	QFETCH(bool, cancel);

	QList<int> input;
	input << 1 << 2 << 3;

	QMutex mutex;

	std::function<int(const int&)> filterFunction = [&](const int& value) -> bool {
		QMutexLocker locker(&mutex);
		return true;
	};

	// Prevent the thread from running before we could cancel it
	mutex.lock();

	QFuture<int> future = QtConcurrent::filtered(input, filterFunction);

	if (cancel)
		future.cancel();

	// Allow the thread to run
	mutex.unlock();

	QTRY_VERIFY(future.isFinished());

	FuturePromise::Ptr promise = FuturePromise::create(future);

	PromiseSpies spies(promise);

	if (cancel)
	{
		QTRY_COMPARE(promise->state(), Deferred::Rejected);
		QTRY_COMPARE(spies.baseRejected.count(), 1);
		QTRY_COMPARE(spies.rejected.count(), 1);
		QCOMPARE(spies.rejected.first().first().toList(), promise->results());
		QCOMPARE(spies.baseRejected.first().first(), QVariant::fromValue(promise->results()));
	}
	else
	{
		QTRY_COMPARE(promise->state(), Deferred::Resolved);
		QTRY_COMPARE(spies.baseResolved.count(), 1);
		QTRY_COMPARE(spies.resolved.count(), 1);
		QCOMPARE(spies.resolved.first().first().toList(), promise->results());
		QCOMPARE(spies.baseResolved.first().first(), QVariant::fromValue(promise->results()));
	}
	QTEST(promise->results(), "expectedResults");
}


/*! Provides the data for the testFinishedDeferred() test.
 */
void FuturePromiseTest::testFinishedDeferred_data()
{
	// Reuse the data from the testFinishedFuture() test.
	testFinishedFuture_data();
}

/*! \test Tests the FuturePromise with a FutureDeferred which has finished
 * and emitted it's events before the FuturePromise is created.
 */
void FuturePromiseTest::testFinishedDeferred()
{
	QFETCH(bool, cancel);

	QList<int> input;
	input << 1 << 2 << 3;

	QMutex mutex;

	std::function<int(const int&)> filterFunction = [&](const int& value) -> bool {
		QMutexLocker locker(&mutex);
		return true;
	};

	// Prevent the thread from running before we could cancel it
	mutex.lock();

	QFuture<int> future = QtConcurrent::filtered(input, filterFunction);

	if (cancel)
		future.cancel();

	FutureDeferred::Ptr deferred = FutureDeferred::create(future);

	// Allow the thread to run
	mutex.unlock();

	QTRY_VERIFY(deferred->state() != Deferred::Pending);

	FuturePromise::Ptr promise = FuturePromise::create(deferred);

	PromiseSpies spies(promise);

	if (cancel)
	{
		QTRY_COMPARE(promise->state(), Deferred::Rejected);
		QTRY_COMPARE(spies.baseRejected.count(), 1);
		QTRY_COMPARE(spies.rejected.count(), 1);
		QCOMPARE(spies.rejected.first().first().toList(), promise->results());
		QCOMPARE(spies.baseRejected.first().first(), QVariant::fromValue(promise->results()));
	}
	else
	{
		QTRY_COMPARE(promise->state(), Deferred::Resolved);
		QTRY_COMPARE(spies.baseResolved.count(), 1);
		QTRY_COMPARE(spies.resolved.count(), 1);
		QCOMPARE(spies.resolved.first().first().toList(), promise->results());
		QCOMPARE(spies.baseResolved.first().first(), QVariant::fromValue(promise->results()));
	}
	QTEST(promise->results(), "expectedResults");
}

}  // namespace Tests
}  // namespace QtPromise



QTEST_MAIN(QtPromise::Tests::FuturePromiseTest)
#include "FuturePromiseTest.moc"


