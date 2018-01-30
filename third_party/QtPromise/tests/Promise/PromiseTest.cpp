
#include <QtTest>
#include <stdexcept>
#include <string>
#include "Promise.h"

Q_DECLARE_METATYPE(QList<QtPromise::Deferred::Ptr>)
Q_DECLARE_METATYPE(QList<int>)

namespace QtPromise
{
namespace Tests
{

const QString ACTION_RESOLVE = QStringLiteral("resolve");
const QString ACTION_REJECT = QStringLiteral("reject");
const QString ACTION_NOTIFY = QStringLiteral("notify");

/*! Unit tests for the Promise class.
 *
 * \author jochen.ulrich
 */
class PromiseTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void testConstructor();
	void testConstructorWithResolvedDeferred();
	void testConstructorWithRejectedDeferred();
	void testCreateResolvedPromise();
	void testCreateRejectedPromise();
	void testResolve();
	void testReject();
	void testNotify();
	void testThenNullCallback_data();
	void testThenNullCallback();
	void testThenVoidCallback_data();
	void testThenVoidCallback();
	void testThenVariantCallback_data();
	void testThenVariantCallback();
	void testThenPromiseCallback_data();
	void testThenPromiseCallback();
	void testThenNotify();
	void testThenNotifyPromiseCallback_data();
	void testThenNotifyPromiseCallback();
	void testAlways_data();
	void testAlways();
	void testThreeLevelChain();
	void testAsyncChain();
	void testAll();
	void testAllReject();
	void testAny();
	void testAnyReject();
	void testAllAnySync_data();
	void testAllAnySync();
	void testAllAnyInitializerList();
	void testPromiseDestruction();
	void testChainDestruction();
	void testParentDeferredDestruction();
	void testDelay_data();
	void testDelay();
	void testQHash();

private:
	struct PromiseSpies
	{
		PromiseSpies(Promise::Ptr promise);

		QSignalSpy resolved;
		QSignalSpy rejected;
		QSignalSpy notified;
	};

	void callActionOnDeferred(Deferred::Ptr& deferred, const QString& action, const QVariant& data, int repetitions);

};


//####### Helper #######

PromiseTest::PromiseSpies::PromiseSpies(Promise::Ptr promise)
	: resolved(promise.data(), &Promise::resolved),
	  rejected(promise.data(), &Promise::rejected),
	  notified(promise.data(), &Promise::notified)
{
}

/*! Helper method which calls an action on a Deferred.
 *
 * \param deferred The Deferred on which the action should be executed.
 * \param action The action to be executed.
 * \param data The data to be passed to the action.
 * \param repetitions The number of times the action should be called.
 */
void PromiseTest::callActionOnDeferred(Deferred::Ptr& deferred, const QString& action, const QVariant& data, int repetitions)
{
	for (int i=0; i < repetitions; ++i)
	{
		if (action == ACTION_RESOLVE)
			deferred->resolve(data);
		else if (action == ACTION_REJECT)
			deferred->reject(data);
		else if (action == ACTION_NOTIFY)
			deferred->notify(data);
		else
			throw std::invalid_argument(std::string("Unknown action: ") + action.toStdString());
	}
}


//####### Tests #######

/*! \test Tests the Promise::create() method.
 */
void PromiseTest::testConstructor()
{
	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);

	QCOMPARE(promise->state(), Deferred::Pending);
	QVERIFY(promise->data().isNull());
}

/*! \test Tests the Promise::create() method with
 * an already resolved Deferred.
 */
void PromiseTest::testConstructorWithResolvedDeferred()
{
	Deferred::Ptr deferred = Deferred::create();
	QString value("string");
	deferred->resolve(value);
	Promise::Ptr promise = Promise::create(deferred);

	PromiseSpies spies(promise);

	QCOMPARE(promise->state(), Deferred::Resolved);
	QCOMPARE(promise->data().toString(), value);

	QVERIFY(spies.resolved.wait());
	QCOMPARE(spies.resolved.count(), 1);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);
}

/*! \test Tests the Promise::create() method with
 * an already rejected Deferred.
 */
void PromiseTest::testConstructorWithRejectedDeferred()
{
	Deferred::Ptr deferred = Deferred::create();
	QString value("string");
	deferred->reject(value);
	Promise::Ptr promise = Promise::create(deferred);

	PromiseSpies spies(promise);

	QCOMPARE(promise->state(), Deferred::Rejected);
	QCOMPARE(promise->data().toString(), value);

	QVERIFY(spies.rejected.wait());
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 1);
	QCOMPARE(spies.notified.count(), 0);
}

/*! \test Tests the Promise::createResolved() method.
 */
void PromiseTest::testCreateResolvedPromise()
{
	QString myString = "String";
	Promise::Ptr promise = Promise::createResolved(myString);

	QCOMPARE(promise->state(), Deferred::Resolved);
	QCOMPARE(promise->data().toString(), myString);
}

/*! \test Tests the Promise::createRejected() method.
 */
void PromiseTest::testCreateRejectedPromise()
{
	QString myString = "String";
	Promise::Ptr promise = Promise::createRejected(myString);

	QCOMPARE(promise->state(), Deferred::Rejected);
	QCOMPARE(promise->data().toString(), myString);
}

/*! \test Tests resolving a Promise.
 */
void PromiseTest::testResolve()
{
	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);

	PromiseSpies spies(promise);

	QString value("myString");
	deferred->resolve(value);

	QCOMPARE(promise->state(), Deferred::Resolved);
	QCOMPARE(promise->data().toString(), value);
	QCOMPARE(spies.resolved.count(), 1);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.resolved.first().first().toString(), value);
}

/*! \test Tests rejecting a Promise.
 */
void PromiseTest::testReject()
{
	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);

	PromiseSpies spies(promise);

	QString value("myString");
	deferred->reject(value);

	QCOMPARE(promise->state(), Deferred::Rejected);
	QCOMPARE(promise->data().toString(), value);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 1);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.rejected.first().first().toString(), value);
}

/*! \test Tests notifying a Promise.
 */
void PromiseTest::testNotify()
{
	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);

	PromiseSpies spies(promise);

	QString firstValue("myString");
	deferred->notify(firstValue);

	QCOMPARE(promise->state(), Deferred::Pending);
	QVERIFY(promise->data().isNull());
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 1);
	QCOMPARE(spies.notified.at(0).first().toString(), firstValue);

	int secondValue(7);
	deferred->notify(secondValue);

	QCOMPARE(promise->state(), Deferred::Pending);
	QVERIFY(promise->data().isNull());
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 2);
	QCOMPARE(spies.notified.at(1).first().toInt(), secondValue);
}

/*! Provides the data for the testThenNullCallback() test.
 */
void PromiseTest::testThenNullCallback_data()
{
	QTest::addColumn<bool>("async");
	QTest::addColumn<QString>("action");
	QTest::addColumn<QVariant>("data");
	QTest::addColumn<Deferred::State>("expectedState");
	QTest::addColumn<QVariant>("expectedData");
	QTest::addColumn<QVariantList>("expectedResolvedCalls");
	QTest::addColumn<QVariantList>("expectedRejectedCalls");
	QTest::addColumn<QVariantList>("expectedNotifiedCalls");

	QVariant data("my string value");
	//                              // async // action         // data // expectedState      // expectedData // expectedResolvedCalls    // expectedRejectedCalls    // expectedNotifiedCalls
	QTest::newRow("sync resolve")  << false  << ACTION_RESOLVE << data << Deferred::Resolved << data         << (QVariantList() << data) << (QVariantList())         << (QVariantList());
	QTest::newRow("sync reject")   << false  << ACTION_REJECT  << data << Deferred::Rejected << data         << (QVariantList())         << (QVariantList() << data) << (QVariantList());
	QTest::newRow("sync notify")   << false  << ACTION_NOTIFY  << data << Deferred::Pending  << QVariant()   << (QVariantList())         << (QVariantList())         << (QVariantList());
	QTest::newRow("async resolve") << true   << ACTION_RESOLVE << data << Deferred::Resolved << data         << (QVariantList() << data) << (QVariantList())         << (QVariantList());
	QTest::newRow("async reject")  << true   << ACTION_REJECT  << data << Deferred::Rejected << data         << (QVariantList())         << (QVariantList() << data) << (QVariantList());
	QTest::newRow("async notify")  << true   << ACTION_NOTIFY  << data << Deferred::Pending  << QVariant()   << (QVariantList())         << (QVariantList())         << (QVariantList() << data << data);
}

/*! \test Tests the Promise::then() method with a \c nullptr as parameter.
 */
void PromiseTest::testThenNullCallback()
{
	QFETCH(bool, async);
	QFETCH(QString, action);
	QFETCH(QVariant, data);


	Deferred::Ptr deferred = Deferred::create();

	if (!async)
		callActionOnDeferred(deferred, action, data, 2);

	Promise::Ptr promise = Promise::create(deferred);

	QVariantList resolvedCalls;
	QVariantList rejectedCalls;
	QVariantList notifiedCalls;

	Promise::Ptr newPromise = promise->then(nullptr, nullptr, nullptr);
	Promise::Ptr spyPromise = newPromise->then([&resolvedCalls](const QVariant& value) {
		resolvedCalls.push_back(value);
	}, [&rejectedCalls](const QVariant& reason) {
		rejectedCalls.push_back(reason);
	}, [&notifiedCalls](const QVariant& progress) {
		notifiedCalls.push_back(progress);
	});

	if (async)
	{
		QCOMPARE(promise->state(), Deferred::Pending);
		QCOMPARE(newPromise->state(), Deferred::Pending);
		QCOMPARE(resolvedCalls.count(), 0);
		QCOMPARE(rejectedCalls.count(), 0);
		QCOMPARE(notifiedCalls.count(), 0);

		callActionOnDeferred(deferred, action, data, 2);
	}

	QTEST(promise->state(), "expectedState");
	QTEST(newPromise->state(), "expectedState");
	QTEST(promise->data(), "expectedData");
	QTEST(newPromise->data(), "expectedData");
	QTEST(resolvedCalls, "expectedResolvedCalls");
	QTEST(rejectedCalls, "expectedRejectedCalls");
	QTEST(notifiedCalls, "expectedNotifiedCalls");

	if (deferred->state() == Deferred::Pending)
		deferred->resolve();
}

/*! Provides the data for the testThenVoidCallback() test.
 */
void PromiseTest::testThenVoidCallback_data()
{
	QTest::addColumn<bool>("async");
	QTest::addColumn<QString>("action");
	QTest::addColumn<QVariant>("data");
	QTest::addColumn<Deferred::State>("expectedState");
	QTest::addColumn<QVariant>("expectedData");
	QTest::addColumn<QVariantList>("expectedResolvedCalls");
	QTest::addColumn<QVariantList>("expectedRejectedCalls");
	QTest::addColumn<QVariantList>("expectedNotifiedCalls");

	QVariant data("my string value");
	//                              // async // action         // data // expectedState      // expectedData // expectedResolvedCalls    // expectedRejectedCalls    // expectedNotifiedCalls
	QTest::newRow("sync resolve")  << false  << ACTION_RESOLVE << data << Deferred::Resolved << data         << (QVariantList() << data) << (QVariantList())         << (QVariantList());
	QTest::newRow("sync reject")   << false  << ACTION_REJECT  << data << Deferred::Rejected << data         << (QVariantList())         << (QVariantList() << data) << (QVariantList());
	QTest::newRow("sync notify")   << false  << ACTION_NOTIFY  << data << Deferred::Pending  << QVariant()   << (QVariantList())         << (QVariantList())         << (QVariantList());
	QTest::newRow("async resolve") << true   << ACTION_RESOLVE << data << Deferred::Resolved << data         << (QVariantList() << data) << (QVariantList())         << (QVariantList());
	QTest::newRow("async reject")  << true   << ACTION_REJECT  << data << Deferred::Rejected << data         << (QVariantList())         << (QVariantList() << data) << (QVariantList());
	QTest::newRow("async notify")  << true   << ACTION_NOTIFY  << data << Deferred::Pending  << QVariant()   << (QVariantList())         << (QVariantList())         << (QVariantList() << data << data);
}

/*! \test Tests the Promise::then() method with a callback returning void.
 */
void PromiseTest::testThenVoidCallback()
{
	QFETCH(bool, async);
	QFETCH(QString, action);
	QFETCH(QVariant, data);


	Deferred::Ptr deferred = Deferred::create();

	if (!async)
		callActionOnDeferred(deferred, action, data, 2);

	Promise::Ptr promise = Promise::create(deferred);

	QVariantList resolvedCalls;
	QVariantList rejectedCalls;
	QVariantList notifiedCalls;

	Promise::Ptr newPromise = promise->then([&resolvedCalls](const QVariant& value) {
		resolvedCalls.push_back(value);
	}, [&rejectedCalls](const QVariant& reason) {
		rejectedCalls.push_back(reason);
	}, [&notifiedCalls](const QVariant& progress) {
		notifiedCalls.push_back(progress);
	});

	QVariantList chainedNotifiedCalls;

	Promise::Ptr chainedNotifyPromise = newPromise->then(nullptr, nullptr, [&chainedNotifiedCalls] (const QVariant& progress) {
		chainedNotifiedCalls.push_back(progress);
	});

	if (async)
	{
		QCOMPARE(promise->state(), Deferred::Pending);
		QCOMPARE(newPromise->state(), Deferred::Pending);
		QCOMPARE(resolvedCalls.count(), 0);
		QCOMPARE(rejectedCalls.count(), 0);
		QCOMPARE(notifiedCalls.count(), 0);

		callActionOnDeferred(deferred, action, data, 2);
	}

	QTEST(promise->state(), "expectedState");
	QTEST(newPromise->state(), "expectedState");
	QTEST(promise->data(), "expectedData");
	QTEST(newPromise->data(), "expectedData");
	QTEST(resolvedCalls, "expectedResolvedCalls");
	QTEST(rejectedCalls, "expectedRejectedCalls");
	QTEST(notifiedCalls, "expectedNotifiedCalls");
	QTEST(chainedNotifiedCalls, "expectedNotifiedCalls");

	if (deferred->state() == Deferred::Pending)
		deferred->resolve(); // Avoid warning
}

/*! Provides the data for the testThenVariantCallback() test.
 */
void PromiseTest::testThenVariantCallback_data()
{
	QTest::addColumn<bool>("async");
	QTest::addColumn<QString>("action");
	QTest::addColumn<QVariant>("originalData");
	QTest::addColumn<QVariant>("chainedData");
	QTest::addColumn<Deferred::State>("expectedOriginalState");
	QTest::addColumn<QVariantList>("expectedOriginalNotifiedCalls");
	QTest::addColumn<Deferred::State>("expectedChainedState");
	QTest::addColumn<QVariant>("expectedChainedData");
	QTest::addColumn<QVariantList>("expectedResolvedCalls");
	QTest::addColumn<QVariantList>("expectedRejectedCalls");
	QTest::addColumn<QVariantList>("expectedNotifiedCalls");


	QVariant originalData(42);
	QVariant chainedData("my string value");
	//                             // async // action         // originalData // chainedData // expectedOriginalState // expectedOriginalNotifiedCalls                    // expectedChainedState // expectedChainedData // expectedResolvedCalls           // expectedRejectedCalls // expectedNotifiedCalls
	QTest::newRow("sync resolve")  << false << ACTION_RESOLVE << originalData << chainedData << Deferred::Resolved    << (QVariantList())                                 << Deferred::Resolved   << chainedData         << (QVariantList() << chainedData) << (QVariantList())      << (QVariantList());
	QTest::newRow("sync reject")   << false << ACTION_REJECT  << originalData << chainedData << Deferred::Rejected    << (QVariantList())                                 << Deferred::Resolved   << chainedData         << (QVariantList() << chainedData) << (QVariantList())      << (QVariantList());
	QTest::newRow("sync notify")   << false << ACTION_NOTIFY  << originalData << chainedData << Deferred::Pending     << (QVariantList())                                 << Deferred::Pending    << QVariant()          << (QVariantList())                << (QVariantList())      << (QVariantList());
	QTest::newRow("async resolve") << true  << ACTION_RESOLVE << originalData << chainedData << Deferred::Resolved    << (QVariantList())                                 << Deferred::Resolved   << chainedData         << (QVariantList() << chainedData) << (QVariantList())      << (QVariantList());
	QTest::newRow("async reject")  << true  << ACTION_REJECT  << originalData << chainedData << Deferred::Rejected    << (QVariantList())                                 << Deferred::Resolved   << chainedData         << (QVariantList() << chainedData) << (QVariantList())      << (QVariantList());
	QTest::newRow("async notify")  << true  << ACTION_NOTIFY  << originalData << chainedData << Deferred::Pending     << (QVariantList() << originalData << originalData) << Deferred::Pending    << QVariant()          << (QVariantList())                << (QVariantList())      << (QVariantList() << chainedData << chainedData);
}

/*! \test Tests the Promise::then() method with a callback returning QVariant.
 */
void PromiseTest::testThenVariantCallback()
{
	QFETCH(bool, async);
	QFETCH(QString, action);
	QFETCH(QVariant, originalData);
	QFETCH(QVariant, chainedData);


	Deferred::Ptr deferred = Deferred::create();

	if (!async)
		callActionOnDeferred(deferred, action, originalData, 2);

	Promise::Ptr promise = Promise::create(deferred);

	QVariantList originalNotifiedCalls;

	Promise::Ptr newPromise = promise->then([&](const QVariant&) -> QVariant {
		return chainedData;
	}, [&](const QVariant&) -> QVariant {
		return chainedData;
	}, [&](const QVariant& progress) -> QVariant {
		originalNotifiedCalls.push_back(progress);
		return chainedData;
	});

	QVariantList resolvedCalls;
	QVariantList rejectedCalls;
	QVariantList notifiedCalls;

	Promise::Ptr spyPromise = newPromise->then([&](const QVariant& value) {
		resolvedCalls.push_back(value);
	}, [&](const QVariant& reason) {
		rejectedCalls.push_back(reason);
	}, [&](const QVariant& progress) {
		notifiedCalls.push_back(progress);
	});

	if (async)
	{
		QCOMPARE(promise->state(), Deferred::Pending);
		QCOMPARE(newPromise->state(), Deferred::Pending);

		callActionOnDeferred(deferred, action, originalData, 2);
	}

	QTEST(promise->state(), "expectedOriginalState");
	QTEST(originalNotifiedCalls, "expectedOriginalNotifiedCalls");
	QTEST(newPromise->state(), "expectedChainedState");
	QTEST(newPromise->data(), "expectedChainedData");
	QTEST(resolvedCalls, "expectedResolvedCalls");
	QTEST(rejectedCalls, "expectedRejectedCalls");
	QTEST(notifiedCalls, "expectedNotifiedCalls");

	if (deferred->state() == Deferred::Pending)
		deferred->resolve(); // Avoid warning
}

/*! Provides the data for the testThenPromiseCallback() test.
 */
void PromiseTest::testThenPromiseCallback_data()
{
	QTest::addColumn<bool>("async");
	QTest::addColumn<QString>("action");
	QTest::addColumn<QString>("callbackAction");
	QTest::addColumn<QVariant>("callbackData");
	QTest::addColumn<Deferred::State>("expectedOriginalState");
	QTest::addColumn<Deferred::State>("expectedChainedState");
	QTest::addColumn<QVariant>("expectedChainedData");
	QTest::addColumn<QVariantList>("expectedResolvedCalls");
	QTest::addColumn<QVariantList>("expectedRejectedCalls");


	QVariant data("my string value");
	//                                        // async // action         // callbackAction      // callbackData // expectedOriginalState // expectedChainedState // expectedChainedData // expectedResolvedCalls    // expectedRejectedCalls
	QTest::newRow("sync resolve -> reject")   << false << ACTION_RESOLVE << ACTION_REJECT       << data         << Deferred::Resolved    << Deferred::Rejected   << data                << (QVariantList())         << (QVariantList() << data);
	QTest::newRow("sync resolve -> resolve")  << false << ACTION_RESOLVE << ACTION_RESOLVE      << data         << Deferred::Resolved    << Deferred::Resolved   << data                << (QVariantList() << data) << (QVariantList());
	QTest::newRow("sync reject -> resolve")   << false << ACTION_REJECT  << ACTION_RESOLVE      << data         << Deferred::Rejected    << Deferred::Resolved   << data                << (QVariantList() << data) << (QVariantList());
	QTest::newRow("sync reject -> reject")    << false << ACTION_REJECT  << ACTION_REJECT       << data         << Deferred::Rejected    << Deferred::Rejected   << data                << (QVariantList())         << (QVariantList() << data);
	QTest::newRow("async resolve -> reject")  << true  << ACTION_RESOLVE << ACTION_REJECT       << data         << Deferred::Resolved    << Deferred::Rejected   << data                << (QVariantList())         << (QVariantList() << data);
	QTest::newRow("async resolve -> resolve") << true  << ACTION_RESOLVE << ACTION_RESOLVE      << data         << Deferred::Resolved    << Deferred::Resolved   << data                << (QVariantList() << data) << (QVariantList());
	QTest::newRow("async reject -> resolve")  << true  << ACTION_REJECT  << ACTION_RESOLVE      << data         << Deferred::Rejected    << Deferred::Resolved   << data                << (QVariantList() << data) << (QVariantList());
	QTest::newRow("async reject -> reject")   << true  << ACTION_REJECT  << ACTION_REJECT       << data         << Deferred::Rejected    << Deferred::Rejected   << data                << (QVariantList())         << (QVariantList() << data);
}

/*! \test Tests the Promise::then() method with a callback returning Promise::Ptr.
 */
void PromiseTest::testThenPromiseCallback()
{
	QFETCH(bool, async);
	QFETCH(QString, action);
	QFETCH(QString, callbackAction);
	QFETCH(QVariant, callbackData);


	Deferred::Ptr deferred = Deferred::create();
	QVariant deferredData("initial data");

	if (!async)
		callActionOnDeferred(deferred, action, deferredData, 1);

	Promise::Ptr promise = Promise::create(deferred);

	auto callback = [&](const QVariant&) -> Promise::Ptr {
		if (callbackAction == ACTION_RESOLVE)
			return Promise::createResolved(callbackData);
		else if (callbackAction == ACTION_REJECT)
			return Promise::createRejected(callbackData);
		else
		{
			QTest::qFail("Invalid callbackAction", __FILE__, __LINE__);
			return Promise::createRejected(QVariant());
		}
	};

	Promise::Ptr newPromise = promise->then(callback, callback);

	QVariantList resolvedCalls;
	QVariantList rejectedCalls;

	Promise::Ptr spyPromise = newPromise->then([&](const QVariant& value) {
		resolvedCalls.push_back(value);
	}, [&](const QVariant& reason) {
		rejectedCalls.push_back(reason);
	});

	if (async)
	{
		QCOMPARE(promise->state(), Deferred::Pending);
		QCOMPARE(newPromise->state(), Deferred::Pending);

		callActionOnDeferred(deferred, action, deferredData, 1);
	}

	QTEST(promise->state(), "expectedOriginalState");
	QTEST(newPromise->state(), "expectedChainedState");
	QTEST(newPromise->data(), "expectedChainedData");
	QTEST(resolvedCalls, "expectedResolvedCalls");
	QTEST(rejectedCalls, "expectedRejectedCalls");

	if (deferred->state() == Deferred::Pending)
		deferred->resolve(); // Avoid warning
}

/*! \test Tests notifying the promise returned by Promise::then().
 */
void PromiseTest::testThenNotify()
{
	// Setup test scenario
	Deferred::Ptr originalDeferred = Deferred::create();
	Deferred::Ptr resolveDeferred = Deferred::create();
	Deferred::Ptr notifyDeferred = Deferred::create();

	Promise::Ptr originalPromise = Promise::create(originalDeferred);

	Promise::Ptr chainedPromise = originalPromise
			->then([&](const QVariant& data) -> Promise::Ptr {
		return Promise::create(resolveDeferred);
	}, nullptr, [&](const QVariant& progress) -> Promise::Ptr {
		return Promise::create(notifyDeferred);
	});

	PromiseSpies spies(chainedPromise);

	// Run test sequence

	/* Notifying one of the callback Deferreds before the original
	 * Deferred should do nothing.
	 */
	resolveDeferred->notify();
	notifyDeferred->notify();

	QTRY_VERIFY(spies.notified.empty());

	// Notifying the original Deferred should "enable" the notifyDeferred.
	originalDeferred->notify(QVariant("first notify"));

	QTRY_VERIFY(spies.notified.empty());

	QVariant secondNotify{"second notify"};
	notifyDeferred->notify(secondNotify);

	QTRY_COMPARE(spies.notified.last().first(), secondNotify);

	QVariant thirdNotify{"third notify"};
	notifyDeferred->notify(thirdNotify);

	QTRY_COMPARE(spies.notified.last().first(), thirdNotify);
	spies.notified.clear();

	// The notifyDeferred is now in control of notifying.
	originalDeferred->notify(QVariant("fourth notify"));
	resolveDeferred->notify(QVariant{"fifth notify"});

	QTRY_VERIFY(spies.notified.empty());
	spies.notified.clear();

	// Resolve the original Deferred should "enable" the resolveDeferred.
	originalDeferred->resolve();

	QTRY_COMPARE(originalPromise->state(), Deferred::Resolved);
	QCOMPARE(chainedPromise->state(), Deferred::Pending);
	QVERIFY(spies.notified.empty());

	QVariant sixthNotify{"sixth notify"};
	resolveDeferred->notify(sixthNotify);

	QTRY_VERIFY(!spies.notified.empty());
	QCOMPARE(spies.notified.last().first(), sixthNotify);
	spies.notified.clear();

	// The resolveDeferred is now in control of notifying.
	originalDeferred->notify(QVariant("seventh notify"));
	notifyDeferred->notify(QVariant("eighth notify"));

	QTest::qWait(100);
	QVERIFY(spies.notified.empty());

	// Avoid warning
	notifyDeferred->resolve();
	resolveDeferred->resolve();
}

/*! Provides the data for the testThenNotifyPromiseCallback() test.
 */
void PromiseTest::testThenNotifyPromiseCallback_data()
{
	QTest::addColumn<QList<bool>>("resolveRejectSequence");
	QTest::addColumn<QVariantList>("notifyData");
	QTest::addColumn<QVariantList>("expectedNotifyCalls");

	QVariant notifyData{"notify"};
	QVariant notifyData2{"notify2"};

	QTest::newRow("resolve") << (QList<bool>{} << true)
	                         << (QVariantList{} << notifyData)
	                         << (QVariantList{} << notifyData);
	QTest::newRow("reject")  << (QList<bool>{} << false)
	                         << (QVariantList{} << notifyData)
	                         << QVariantList();
	QTest::newRow("resolve, reject") << (QList<bool>{} << true << false)
	                                 << (QVariantList{} << notifyData << notifyData2)
	                                 << (QVariantList{} << notifyData);
	QTest::newRow("reject, resolve") << (QList<bool>{} << false << true)
	                                 << (QVariantList{} << notifyData << notifyData2)
	                                 << (QVariantList{} << notifyData2);
}

/*! \test Tests returning a resolved or rejected Promise from a notifyCallback of
 * Promise::then().
 */
void PromiseTest::testThenNotifyPromiseCallback()
{
	QFETCH(QList<bool>, resolveRejectSequence);
	QFETCH(QVariantList, notifyData);

	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);

	QVariantList notifiedCalls;

	auto iResolveRejectSeq = QListIterator<bool>{resolveRejectSequence};
	auto iNotifyData = QListIterator<QVariant>{notifyData};
	Promise::Ptr resultPromise = promise->then(nullptr, nullptr,
	                                           [&](const QVariant& progress) -> Promise::Ptr {
		if (iResolveRejectSeq.next())
			return Promise::createResolved(iNotifyData.next());
		else
			return Promise::createRejected(iNotifyData.next());
	})
	->then(nullptr, nullptr, [&](const QVariant& progress) {
		notifiedCalls.push_back(progress);
	});

	callActionOnDeferred(deferred, ACTION_NOTIFY, QVariant{"dummy data"}, resolveRejectSequence.count());

	QTEST(notifiedCalls, "expectedNotifyCalls");

	// Avoid warning
	deferred->resolve();
}

/*! Provides the data for the testAlways() test.
 */
void PromiseTest::testAlways_data()
{
	QTest::addColumn<QString>("action");

	//                       // action
	QTest::newRow("resolve") << ACTION_RESOLVE;
	QTest::newRow("reject")  << ACTION_REJECT;
}

/*! \test Tests the Promise::always() method.
 */
void PromiseTest::testAlways()
{
	QFETCH(QString, action);

	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);

	QVariantList callbackCalls;

	Promise::Ptr newPromise = promise->always([&](const QVariant& data) {
		callbackCalls.push_back(data);
	});

	QVariant originalData("initial data");
	callActionOnDeferred(deferred, action, originalData, 1);

	QVERIFY(promise->state() == newPromise->state());
	QCOMPARE(callbackCalls, QVariantList() << originalData);
}

/*! \test Tests if a Promise chain survives deletion of intermediate Promises.
 */
void PromiseTest::testThreeLevelChain()
{
	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr originalPromise = Promise::create(deferred);

	QVariantList callbackCalls;

	Promise::Ptr finalPromise = originalPromise->then([&](const QVariant& data) {
		callbackCalls.push_back(data);
	})->then([&](const QVariant& data) {
		callbackCalls.push_back(data);
	});

	QVariant data("my data");
	callActionOnDeferred(deferred, ACTION_RESOLVE, data, 1);
	QCOMPARE(callbackCalls, QVariantList() << data << data);
}

/*! \test Tests chaining multiple asynchronous operations.
 */
void PromiseTest::testAsyncChain()
{
	Deferred::Ptr deferred = Deferred::create();
	Deferred::Ptr transmitDeferred = Deferred::create();
	Promise::Ptr originalPromise = Promise::create(deferred);

	Promise::Ptr finalPromise = originalPromise
			->then([&](const QVariant&) -> Promise::Ptr {
		Deferred::Ptr secondDeferred = Deferred::create();
		QObject::connect(transmitDeferred.data(), &Deferred::resolved,
		                 secondDeferred.data(), &Deferred::resolve);
		return Promise::create(secondDeferred);
	});

	QVariant data("my data");
	callActionOnDeferred(deferred, ACTION_RESOLVE, data, 1);

	QTRY_COMPARE(finalPromise->state(), Deferred::Pending);

	QVariant secondData("second data");
	qDebug("Resolve transmitDeferred");
	callActionOnDeferred(transmitDeferred, ACTION_RESOLVE, secondData, 1);

	QTRY_COMPARE(finalPromise->state(), Deferred::Resolved);
	QTRY_COMPARE(finalPromise->data(), secondData);
}

/*! \test Tests resolving a combined Promise created with Promise::all().
 */
void PromiseTest::testAll()
{
	QList<Deferred::Ptr> deferreds;
	deferreds << Deferred::create() << Deferred::create() << Deferred::create();

	QList<Promise::Ptr> promises;
	for (Deferred::Ptr deferred : deferreds)
		promises << Promise::create(deferred);

	Promise::Ptr combinedPromise = Promise::all(promises);

	PromiseSpies spies(combinedPromise);

	QTRY_COMPARE(spies.resolved.count(), 0);
	QTRY_COMPARE(spies.rejected.count(), 0);
	QTRY_COMPARE(spies.notified.count(), 0);

	QList<QVariant> results;
	results << "My string" << 15 << QVariant::fromValue(QList<int>() << 7 << 13);

	deferreds[0]->resolve(results[0]);

	QTRY_COMPARE(spies.resolved.count(), 0);
	QTRY_COMPARE(spies.rejected.count(), 0);
	QTRY_COMPARE(spies.notified.count(), 0);

	deferreds[2]->resolve(results[2]);

	QTRY_COMPARE(spies.resolved.count(), 0);
	QTRY_COMPARE(spies.rejected.count(), 0);
	QTRY_COMPARE(spies.notified.count(), 0);

	deferreds[1]->resolve(results[1]);

	QTRY_COMPARE(spies.resolved.count(), 1);
	QTRY_COMPARE(spies.rejected.count(), 0);
	QTRY_COMPARE(spies.notified.count(), 0);
	QTRY_COMPARE(spies.resolved.first().first(), QVariant::fromValue(results));
}

/*! \test Tests rejecting a combined Promise created with Promise::all().
 */
void PromiseTest::testAllReject()
{
	QList<Deferred::Ptr> deferreds;
	deferreds << Deferred::create() << Deferred::create() << Deferred::create();

	QList<Promise::Ptr> promises;
	for (Deferred::Ptr deferred : deferreds)
		promises << Promise::create(deferred);

	Promise::Ptr combinedPromise = Promise::all(promises);

	PromiseSpies spies(combinedPromise);

	QTRY_COMPARE(spies.resolved.count(), 0);
	QTRY_COMPARE(spies.rejected.count(), 0);
	QTRY_COMPARE(spies.notified.count(), 0);

	QVariant rejectReason = "Error string";

	deferreds[0]->resolve(13);

	QTRY_COMPARE(spies.resolved.count(), 0);
	QTRY_COMPARE(spies.rejected.count(), 0);
	QTRY_COMPARE(spies.notified.count(), 0);

	deferreds[1]->reject(rejectReason);

	QTRY_COMPARE(spies.resolved.count(), 0);
	QTRY_COMPARE(spies.rejected.count(), 1);
	QTRY_COMPARE(spies.notified.count(), 0);
	QTRY_COMPARE(spies.rejected.first().first(), rejectReason);

	// Avoid warning
	deferreds[2]->resolve();
}

/*! \test Tests resolving a combined Promise created with Promise::any().
 */
void PromiseTest::testAny()
{
	QList<Deferred::Ptr> deferreds;
	deferreds << Deferred::create() << Deferred::create() << Deferred::create();

	QList<Promise::Ptr> promises;
	for (Deferred::Ptr deferred : deferreds)
		promises << Promise::create(deferred);

	Promise::Ptr combinedPromise = Promise::any(promises);

	PromiseSpies spies(combinedPromise);

	QTRY_COMPARE(spies.resolved.count(), 0);
	QTRY_COMPARE(spies.rejected.count(), 0);
	QTRY_COMPARE(spies.notified.count(), 0);

	QVariant rejectReason = "Error string";
	QVariant result = 13;

	deferreds[0]->reject(rejectReason);

	QTRY_COMPARE(spies.resolved.count(), 0);
	QTRY_COMPARE(spies.rejected.count(), 0);
	QTRY_COMPARE(spies.notified.count(), 0);

	deferreds[1]->resolve(result);

	QTRY_COMPARE(spies.resolved.count(), 1);
	QTRY_COMPARE(spies.rejected.count(), 0);
	QTRY_COMPARE(spies.notified.count(), 0);
	QTRY_COMPARE(spies.resolved.first().first(), QVariant::fromValue(result));

	// Avoid warning
	deferreds[2]->resolve();
}

/*! \test Tests rejecting a combined Promise created with Promise::any().
 */
void PromiseTest::testAnyReject()
{
	QList<Deferred::Ptr> deferreds;
	deferreds << Deferred::create() << Deferred::create() << Deferred::create();

	QList<Promise::Ptr> promises;
	for (Deferred::Ptr deferred : deferreds)
		promises << Promise::create(deferred);

	Promise::Ptr combinedPromise = Promise::any(promises);

	PromiseSpies spies(combinedPromise);

	QTRY_COMPARE(spies.resolved.count(), 0);
	QTRY_COMPARE(spies.rejected.count(), 0);
	QTRY_COMPARE(spies.notified.count(), 0);

	QList<QVariant> rejectReasons;
	rejectReasons << "My string" << 15 << QVariant::fromValue(QList<int>() << 7 << 13);

	deferreds[0]->reject(rejectReasons[0]);

	QTRY_COMPARE(spies.resolved.count(), 0);
	QTRY_COMPARE(spies.rejected.count(), 0);
	QTRY_COMPARE(spies.notified.count(), 0);

	deferreds[2]->reject(rejectReasons[2]);

	QTRY_COMPARE(spies.resolved.count(), 0);
	QTRY_COMPARE(spies.rejected.count(), 0);
	QTRY_COMPARE(spies.notified.count(), 0);

	deferreds[1]->reject(rejectReasons[1]);

	QTRY_COMPARE(spies.resolved.count(), 0);
	QTRY_COMPARE(spies.rejected.count(), 1);
	QTRY_COMPARE(spies.notified.count(), 0);
	QTRY_COMPARE(spies.rejected.first().first(), QVariant::fromValue(rejectReasons));
}

/*! Provides the data for the testAllAnySync() test.
 */
void PromiseTest::testAllAnySync_data()
{
	QTest::addColumn<QList<Deferred::Ptr>>("deferreds");
	QTest::addColumn<QList<int>>("expectedAllSignalCounts");
	QTest::addColumn<QList<int>>("expectedAnySignalCounts");

	QList<Deferred::Ptr> oneResolvedDeferreds;
	oneResolvedDeferreds << Deferred::create() << Deferred::create() << Deferred::create();
	oneResolvedDeferreds[0]->resolve("foo");
	//                            // deferreds            // expectedAllSignalCounts       // expectedAnySignalCounts
	QTest::newRow("one resolved") << oneResolvedDeferreds << (QList<int>() << 0 << 0 << 0) << (QList<int>() << 1 << 0 << 0);

	QList<Deferred::Ptr> allResolvedDeferreds;
	allResolvedDeferreds << Deferred::create() << Deferred::create() << Deferred::create();
	allResolvedDeferreds[0]->resolve("foo");
	allResolvedDeferreds[1]->resolve(17);
	allResolvedDeferreds[2]->resolve(true);
	//                            // deferreds            // expectedAllSignalCounts       // expectedAnySignalCounts
	QTest::newRow("all resolved") << allResolvedDeferreds << (QList<int>() << 1 << 0 << 0) << (QList<int>() << 1 << 0 << 0);

	QList<Deferred::Ptr> oneRejectedDeferreds;
	oneRejectedDeferreds << Deferred::create() << Deferred::create() << Deferred::create();
	oneRejectedDeferreds[0]->reject("foo");
	//                            // deferreds            // expectedAllSignalCounts       // expectedAnySignalCounts
	QTest::newRow("one rejected") << oneRejectedDeferreds << (QList<int>() << 0 << 1 << 0) << (QList<int>() << 0 << 0 << 0);

	QList<Deferred::Ptr> allRejectedDeferreds;
	allRejectedDeferreds << Deferred::create() << Deferred::create() << Deferred::create();
	allRejectedDeferreds[0]->reject("foo");
	allRejectedDeferreds[1]->reject(21);
	allRejectedDeferreds[2]->reject(false);
	//                            // deferreds            // expectedAllSignalCounts       // expectedAnySignalCounts
	QTest::newRow("all rejected") << allRejectedDeferreds << (QList<int>() << 0 << 1 << 0) << (QList<int>() << 0 << 1 << 0);
}

/*! \test Tests Promise::all() and Promise::any()
 * when created with already resolved promises.
 */
void PromiseTest::testAllAnySync()
{
	QFETCH(QList<Deferred::Ptr>, deferreds);
	QFETCH(QList<int>, expectedAllSignalCounts);
	QFETCH(QList<int>, expectedAnySignalCounts);

	QList<Promise::Ptr> promises;
	for (Deferred::Ptr deferred : deferreds)
		promises << Promise::create(deferred);
	
	Promise::Ptr allPromise = Promise::all(promises);
	Promise::Ptr anyPromise = Promise::any(promises);
	
	PromiseSpies allSpies(allPromise);
	PromiseSpies anySpies(anyPromise);
	
	QTRY_COMPARE(allSpies.resolved.count(), expectedAllSignalCounts[0]);
	QTRY_COMPARE(allSpies.rejected.count(), expectedAllSignalCounts[1]);
	QTRY_COMPARE(allSpies.notified.count(), expectedAllSignalCounts[2]);
	QTRY_COMPARE(anySpies.resolved.count(), expectedAnySignalCounts[0]);
	QTRY_COMPARE(anySpies.rejected.count(), expectedAnySignalCounts[1]);
	QTRY_COMPARE(anySpies.notified.count(), expectedAnySignalCounts[2]);

	// Avoid warnings
	for (Deferred::Ptr deferred : deferreds)
		if (deferred->state() == Deferred::Pending)
			deferred->resolve();
}

/*! \test Tests Promise::all() and Promise::any()
 * with initializer lists.
 */
void PromiseTest::testAllAnyInitializerList()
{
	Promise::Ptr firstPromise = Promise::createResolved(17);
	Promise::Ptr secondPromise = Promise::createResolved(4);

	Promise::Ptr allPromise = Promise::all({firstPromise, secondPromise});
	Promise::Ptr anyPromise = Promise::any({firstPromise, secondPromise});

	QTRY_COMPARE(allPromise->state(), Deferred::Resolved);
	QTRY_COMPARE(anyPromise->state(), Deferred::Resolved);
}

/*! \test Tests destruction of a Promise only.
 */
void PromiseTest::testPromiseDestruction()
{
	Deferred::Ptr deferred = Deferred::create();
	QVariantList callbackCalls;
	{
		Promise::Ptr promise = Promise::create(deferred);
		auto finalPromise = promise->always([&](const QVariant& data) {
			callbackCalls.push_back(data);
		});
	}

	QCOMPARE(callbackCalls.count(), 0);

	deferred->resolve("foo");

	/* Ensure that the callbacks are *NOT* called after the chain
	 * has been destructed.
	 */
	QTest::qWait(100);
	QCOMPARE(callbackCalls.count(), 0);
}

/*! \test Tests destruction of a complete Promise chain.
 */
void PromiseTest::testChainDestruction()
{
	Deferred::Ptr deferred = Deferred::create();
	QVariantList callbackCalls;
	{
		Promise::Ptr finalPromise;
		{
			Promise::Ptr originalPromise = Promise::create(deferred);

			finalPromise = originalPromise
			->then(nullptr, nullptr, nullptr)
			->then([&](const QVariant& data) {
				callbackCalls.push_back(data);
			}, [&](const QVariant& data) {
				callbackCalls.push_back(data);
			}, [&](const QVariant& data) {
				callbackCalls.push_back(data);
			});
		}

		// Allow potential callbacks to be called
		QTest::qWait(100);

		QVERIFY(callbackCalls.isEmpty());
	}

	// The chain should be *immediately* destroyed, so no qWait() here!
	QVERIFY(callbackCalls.isEmpty());

	deferred->resolve("foo");

	/* Ensure that the callbacks are *NOT* called after the chain
	 * has been destructed.
	 */
	QTest::qWait(100);
	QVERIFY(callbackCalls.isEmpty());
}

/*! \test Tests destruction of a parent Deferred while a ChildDeferred
 * is still holding a reference.
 * This is testing behavior of the library in case of defective usage.
 */
void PromiseTest::testParentDeferredDestruction()
{
	/* We need to dynamically allocate the Deferred::Ptr and
	 * explicitly leak it. This prevents that the QSharedPointer class
	 * tries to delete the Deferred object after we have deleted it
	 * because there still is a QSharedPointer holding a reference.
	 * This way, we can delete the Deferred object without having the
	 * test crash.
	 */
	Deferred::Ptr* deferred = new Deferred::Ptr(Deferred::create());

	ChildDeferred::Ptr childDeferred = ChildDeferred::create(*deferred);

	QCOMPARE(childDeferred->parents().size(), 1);

	(*deferred)->resolve(); // Avoid warning

	// This is buggy code which would lead to a crash in a real application
	delete deferred->data(); // THIS IS AN ERROR! DON'T DO THIS IN A REAL APPLICATION!

	QCOMPARE(childDeferred->parents().size(), 0);

	childDeferred->resolve(); // Avoid warning
}

/*! Provides the data for the PromiseTest::testDelay() test.
 */
void PromiseTest::testDelay_data()
{
	QTest::addColumn<QString>("action");
	QTest::addColumn<int>("delay");

	//                                // action         // delay
	QTest::newRow("deferred resolve") << ACTION_RESOLVE << 0;
	QTest::newRow("deferred reject")  << ACTION_REJECT  << 0;
	QTest::newRow("delayed resolve")  << ACTION_RESOLVE << 100;
	QTest::newRow("delayed reject")   << ACTION_REJECT  << 100;
}

/*! \test Tests the Promise::delayedResolve() and Promise::delayedReject() methods.
 */
void PromiseTest::testDelay()
{
	QFETCH(QString, action);
	QFETCH(int, delay);

	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);

	QVariant data = QVariant::fromValue(QString("foo bar"));
	auto finalPromise = promise->then([=](const QVariant&) {
		if (action == ACTION_RESOLVE)
			return Promise::delayedResolve(data, delay);
		else if (action == ACTION_REJECT)
			return Promise::delayedReject(data, delay);
		else
			return Promise::createRejected("Unexpected action");
	});

	int delayDelta = delay * 0.3;

	deferred->resolve("original data");

	QCOMPARE(finalPromise->state(), Deferred::Pending);

	QTest::qWait(delay - delayDelta + 1);

	if (delay > 0)
	{
		QCOMPARE(finalPromise->state(), Deferred::Pending);

		QTest::qWait(2 * delayDelta);
	}

	if (action == ACTION_RESOLVE)
		QCOMPARE(finalPromise->state(), Deferred::Resolved);
	else if (action == ACTION_REJECT)
		QCOMPARE(finalPromise->state(), Deferred::Rejected);
	else
		QFAIL("Unexpected action");
	QCOMPARE(finalPromise->data(), data);
}

/*! \test Tests the qHash(const QtPromise::Promise::Ptr&, uint) function.
 */
void PromiseTest::testQHash()
{
	Promise::Ptr firstPromise = Promise::createResolved();
	Promise::Ptr secondPromise = Promise::createResolved();

	QCOMPARE(qHash(firstPromise), qHash(firstPromise.data()));
	QVERIFY(qHash(firstPromise) != qHash(secondPromise));
}


}  // namespace Tests
}  // namespace QtPromise


QTEST_MAIN(QtPromise::Tests::PromiseTest)
#include "PromiseTest.moc"


