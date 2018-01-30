
#include <QtTest>
#include "Deferred.h"

namespace QtPromise
{
namespace Tests
{

/*! \brief Unit tests for the Deferred class.
 *
 * \author jochen.ulrich
 */
class DeferredTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void testConstructor();
	void testResolve();
	void testReject();
	void testNotify();
	void testQHash();

private:
	struct DeferredSpies
	{
		DeferredSpies(Deferred::Ptr deferred);

		QSignalSpy resolved;
		QSignalSpy rejected;
		QSignalSpy notified;
	};

};

//####### Helpers #######
DeferredTest::DeferredSpies::DeferredSpies(Deferred::Ptr deferred)
	: resolved(deferred.data(), &Deferred::resolved),
	  rejected(deferred.data(), &Deferred::rejected),
	  notified(deferred.data(), &Deferred::notified)
{
}


//####### Tests #######
/*! \test Tests the Deferred::create() method.
 */
void DeferredTest::testConstructor()
{
	Deferred::Ptr deferred = Deferred::create();
	qDebug() << "Deferred:" << deferred.data();

	QVERIFY(!deferred.isNull());
	QCOMPARE(deferred->state(), Deferred::Pending);
	QVERIFY(deferred->data().isNull());
}

/*! \test Tests the Deferred::resolve() method.
 */
void DeferredTest::testResolve()
{
	Deferred::Ptr deferred = Deferred::create();

	DeferredSpies spies(deferred);

	QString value("myValue");

	QVERIFY(deferred->resolve(value));
	QCOMPARE(deferred->data().toString(), value);
	QCOMPARE(deferred->state(), Deferred::Resolved);
	QCOMPARE(spies.resolved.count(), 1);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.resolved.first().first().toString(), value);

	QVERIFY(!deferred->notify(QString("progress")));
	QVERIFY(!deferred->reject(QString("reason")));
	QCOMPARE(deferred->data().toString(), value);
	QCOMPARE(deferred->state(), Deferred::Resolved);
	QCOMPARE(spies.resolved.count(), 1);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 0);
}

/*! \test Tests the Deferred::reject() method.
 */
void DeferredTest::testReject()
{
	Deferred::Ptr deferred = Deferred::create();

	DeferredSpies spies(deferred);

	QString value("myValue");

	QVERIFY(deferred->reject(value));
	QCOMPARE(deferred->data().toString(), value);
	QCOMPARE(deferred->state(), Deferred::Rejected);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 1);
	QCOMPARE(spies.notified.count(), 0);
	QCOMPARE(spies.rejected.first().first().toString(), value);

	QVERIFY(!deferred->notify(QString("progress")));
	QVERIFY(!deferred->resolve(QString("value")));
	QCOMPARE(deferred->data().toString(), value);
	QCOMPARE(deferred->state(), Deferred::Rejected);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 1);
	QCOMPARE(spies.notified.count(), 0);
}

/*! \test Tests the Deferred::notify() method.
 */
void DeferredTest::testNotify()
{
	Deferred::Ptr deferred = Deferred::create();
	qDebug() << "Deferred:" << deferred.data();

	DeferredSpies spies(deferred);

	QString firstValue("myValue");

	QVERIFY(deferred->notify(firstValue));
	QVERIFY(deferred->data().isNull());
	QCOMPARE(deferred->state(), Deferred::Pending);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 1);
	QCOMPARE(spies.notified.at(0).first().toString(), firstValue);

	int secondValue(3);

	QVERIFY(deferred->notify(secondValue));
	QVERIFY(deferred->data().isNull());
	QCOMPARE(deferred->state(), Deferred::Pending);
	QCOMPARE(spies.resolved.count(), 0);
	QCOMPARE(spies.rejected.count(), 0);
	QCOMPARE(spies.notified.count(), 2);
	QCOMPARE(spies.notified.at(1).first().toInt(), secondValue);
}

/*! \test Tests the qHash(const QtPromise::Deferred::Ptr&, uint) function.
 */
void DeferredTest::testQHash()
{
	Deferred::Ptr firstDeferred = Deferred::create();
	Deferred::Ptr secondDeferred = Deferred::create();

	QCOMPARE(qHash(firstDeferred), qHash(firstDeferred.data()));
	QVERIFY(qHash(firstDeferred) != qHash(secondDeferred));
}


}  // namespace Tests
}  // namespace QtPromise



QTEST_MAIN(QtPromise::Tests::DeferredTest)
#include "DeferredTest.moc"


