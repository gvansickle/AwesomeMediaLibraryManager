
#include <QtTest>
#include <QtDebug>
#include <QWeakPointer>
#include <QScopedPointer>
#include "PromiseSitter.h"


namespace QtPromise
{
namespace Tests
{

const QString ACTION_RESOLVE = "resolve";
const QString ACTION_REJECT = "reject";
const QString ACTION_NOTIFY = "notify";
const QString ACTION_REMOVE = "remove";

/*! Unit tests for the PromiseSitter class.
 *
 * \author jochen.ulrich
 */
class PromiseSitterTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void testAddContainsRemove();
	void testPromiseLifetime_data();
	void testPromiseLifetime();
	void testGlobalInstance_data();
	void testGlobalInstance();
	void testContextObjects_data();
	void testContextObjects();
	void testDestructor();

private:
	struct PromiseSpies
	{
		PromiseSpies(Promise::Ptr promise);

		QSignalSpy resolved;
		QSignalSpy rejected;
		QSignalSpy notified;
	};

};


//####### Helper #######

PromiseSitterTest::PromiseSpies::PromiseSpies(Promise::Ptr promise)
	: resolved(promise.data(), &Promise::resolved),
	  rejected(promise.data(), &Promise::rejected),
	  notified(promise.data(), &Promise::notified)
{
}

//####### Tests #######

/*! \test Tests the PromiseSitter::add(), PromiseSitter::contains()
 * and PromiseSitter::remove() methods.
 */
void PromiseSitterTest::testAddContainsRemove()
{
	PromiseSitter sitter;

	Deferred::Ptr deferred = Deferred::create();
	QWeakPointer<Promise> promiseWPointer;

	Promise::Ptr promise = Promise::create(deferred);
	promiseWPointer = promise;

	sitter.add(promise);
	QVERIFY(sitter.contains(promise));
	QVERIFY(sitter.remove(promise));
	QVERIFY(!sitter.contains(promise));
	QVERIFY(!sitter.remove(promise));

	sitter.add(promise);
	Promise::Ptr samePromise = promise;
	QVERIFY(sitter.contains(samePromise));

	deferred->resolve();

	QTRY_VERIFY(!sitter.contains(promise));
	QVERIFY(!sitter.contains(samePromise));
	QVERIFY(!promiseWPointer.isNull());
}

/*! Provides the data for the testPromiseLifetime() test.
 */
void PromiseSitterTest::testPromiseLifetime_data()
{
	QTest::addColumn<QString>("action");

	QTest::newRow("resolve") << ACTION_RESOLVE;
	QTest::newRow("reject") << ACTION_REJECT;
	QTest::newRow("notify") << ACTION_NOTIFY;
}

/*! \test Tests the releasing of Promises from the PromiseSitter.
 */
void PromiseSitterTest::testPromiseLifetime()
{
	QFETCH(QString, action);

	PromiseSitter sitter;

	Deferred::Ptr deferred = Deferred::create();
	QScopedPointer<PromiseSpies> spies;
	QWeakPointer<Promise> promiseWPointer;
	bool chainedActionTriggered = false;
	{
		Promise::Ptr promise = Promise::create(deferred);
		sitter.add(promise);
		promiseWPointer = promise;
		spies.reset(new PromiseSpies{promise});

		sitter.add(promise->always([&chainedActionTriggered](const QVariant&) {
			chainedActionTriggered = true;
		}));

		// Our Promise::Ptr goes out of scope
	}

	QVERIFY2(!promiseWPointer.isNull(), "Promise was destroyed although added to the sitter");

	QString data = "foo bar";
	if (action == ACTION_RESOLVE)
		deferred->resolve(data);
	else if (action == ACTION_REJECT)
		deferred->reject(data);
	else if (action == ACTION_NOTIFY)
		deferred->notify(data);
	else
		QFAIL("Invalid value for \"action\"");

	if (action == ACTION_NOTIFY)
		QVERIFY(!promiseWPointer.isNull());
	else
		/* When the promise is resolved or rejected,
		 * it will not be deleted immediatelly since one
		 * cannot delete an object in a slot attached to one
		 * of its signals.
		 */
		QTRY_VERIFY(promiseWPointer.isNull());

	// Verify if actions have been triggered
	if (action == ACTION_RESOLVE)
	{
		QCOMPARE(spies->resolved.count(), 1);
		QCOMPARE(spies->resolved.at(0).at(0).toString(), data);
	}
	else if (action == ACTION_REJECT)
	{
		QCOMPARE(spies->rejected.count(), 1);
		QCOMPARE(spies->rejected.at(0).at(0).toString(), data);
	}
	else if (action == ACTION_NOTIFY)
	{
		QCOMPARE(spies->notified.count(), 1);
		QCOMPARE(spies->notified.at(0).at(0).toString(), data);
	}

	if (action == ACTION_NOTIFY)
		QVERIFY(!chainedActionTriggered);
	else
		QVERIFY(chainedActionTriggered);

	// Avoid warning
	if (deferred->state() == Deferred::Pending)
		deferred->resolve();
}

/*! Provides the data for the testGlobalInstance() test.
 */
void PromiseSitterTest::testGlobalInstance_data()
{
	QTest::addColumn<Deferred::Ptr>("deferred");
	QTest::addColumn<Promise::Ptr>("promise");

	Deferred::Ptr deferred = Deferred::create();
	Promise::Ptr promise = Promise::create(deferred);
	QTest::newRow("testGlobalInstance") << deferred << promise;
	PromiseSitter::instance()->add(promise);
}

/*! \test Tests the global instance of the PromiseSitter.
 */
void PromiseSitterTest::testGlobalInstance()
{
	QFETCH(Deferred::Ptr, deferred);
	QFETCH(Promise::Ptr, promise);

	QVERIFY(PromiseSitter::instance()->contains(promise));

	deferred->resolve("data");
	QTRY_VERIFY(!PromiseSitter::instance()->contains(promise));
}

void PromiseSitterTest::testContextObjects_data()
{
	QTest::addColumn<int>("contextObjectCount");
	QTest::addColumn<int>("destroyIndex");

	//                                // contextObjectCount // destroyIndex
	QTest::newRow("single object")    << 1                  << 0;
	QTest::newRow("multiple objects") << 3                  << 1;
}

/*! \test Tests the releasing of Promises from the PromiseSitter
 * when their context object is deleted.
 */
void PromiseSitterTest::testContextObjects()
{
	QFETCH(int, contextObjectCount);
	QFETCH(int, destroyIndex);

	Q_ASSERT(destroyIndex < contextObjectCount);

	QScopedPointer<PromiseSitter> sitter{new PromiseSitter};

	Deferred::Ptr deferred = Deferred::create();
	QWeakPointer<Promise> promiseWPointer;

	QVector<QSharedPointer<QObject>> contextObjs;
	QVector<const QObject*> rawContextObjs;

	for (int i=0; i < contextObjectCount; ++i)
	{
		QSharedPointer<QObject> contextObj{new QObject};
		contextObjs << contextObj;
		rawContextObjs << contextObj.data();
	}

	// Scope for promise
	{
		Promise::Ptr promise = Promise::create(deferred);
		if (contextObjectCount == 1)
			sitter->add(promise, rawContextObjs.first());
		else
			sitter->add(promise, rawContextObjs);
		promiseWPointer = promise;
	}

	QVERIFY2(!promiseWPointer.isNull(), "Promise was destroyed although added to the sitter");

	contextObjs[destroyIndex].reset();

	// PromiseSitter should now drop the reference
	QTRY_VERIFY(promiseWPointer.isNull());

	// Prevent warning
	deferred->resolve();
}

/*! \test Tests \link PromiseSitter::~PromiseSitter() the destructor \endlink of the PromiseSitter.
 */
void PromiseSitterTest::testDestructor()
{
	QScopedPointer<PromiseSitter> sitter{new PromiseSitter};

	Deferred::Ptr deferred = Deferred::create();
	QWeakPointer<Promise> promiseWPointer;

	QScopedPointer<QObject> contextObject{new QObject};

	// Scope for promise
	{
		Promise::Ptr promise = Promise::create(deferred);
		sitter->add(promise, contextObject.data());
		promiseWPointer = promise;
	}

	QVERIFY2(!promiseWPointer.isNull(), "Promise was destroyed although added to the sitter");

	sitter.reset();

	// Promise should now have gone out of scope
	QTRY_VERIFY(promiseWPointer.isNull());

	// Deleting the context object afterwards should not have any effect
	contextObject.reset();

	// Prevent warning
	deferred->resolve();
}


}  // namespace Tests
}  // namespace QtPromise


QTEST_MAIN(QtPromise::Tests::PromiseSitterTest)
#include "PromiseSitterTest.moc"


