/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
 *
 * This file is part of AwesomeMediaLibraryManager.
 *
 * AwesomeMediaLibraryManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AwesomeMediaLibraryManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AwesomeMediaLibraryManager.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "AsyncTests.h"

#include <QString>

#include "../ExtAsync.h"
//#include "../ExtFuture.h"


AsyncTests::AsyncTests() : QObject()
{

}

static QString delayed_string_func_1()
{
	auto retval = QtConcurrent::run([](){
		// Sleep for a second.
		qDb() << "ENTER, SLEEPING FOR 1 SEC";
		QThread::sleep(1);
		qDb() << "SLEEP COMPLETE";
		return QString("delayed_string_func_1() output");
	});

	return retval;
}

void AsyncTests::ExtFutureThenChainingTest()
{
	qIn() << "START";

	bool ran1 = false;
	bool ran2 = false;
	bool ran3 = false;

	ExtFuture<QString> future = ExtAsync::run(delayed_string_func_1);

	future
	.then([&](ExtFuture<QString> str) -> QString {
		qDb() << "Then1, got str:" << str;
		qDb() << "Then1, str val:" << str.get();
		Q_ASSERT(ran1 == false);
		Q_ASSERT(ran2 == false);
		Q_ASSERT(ran3 == false);
		ran1 = true;
		Q_ASSERT(str.get() == "delayed_string_func_1() output");
		return QString("Then1 OUTPUT");
	})
	.then([&](ExtFuture<QString> str) -> QString {
		qDb() << "Then2, got str:" << str;
		qDb() << "Then2, str val:" << str.get();
		Q_ASSERT(ran1 == true);
		Q_ASSERT(ran2 == false);
		Q_ASSERT(ran3 == false);
		ran2 = true;
		return QString("Then2 OUTPUT");
	})
	.then([&](ExtFuture<QString> str) -> QString {
		qDb() << "Then3, got str:" << str;
		qDb() << "Then3, str val:" << str.get();
		Q_ASSERT(ran1 == true);
		Q_ASSERT(ran2 == true);
		Q_ASSERT(ran3 == false);
		ran3 = true;
		return QString("Then3 OUTPUT");
	}).wait();

	qDb() << "STARING WAIT";
	/// @todo This doesn't wait here, but the attached wait() above does.
	future.wait();
	qDb() << "ENDING WAIT";

	Q_ASSERT(ran1 == true);
	Q_ASSERT(ran2 == true);
	Q_ASSERT(ran3 == true);

	qIn() << "Complete";

}

static ExtFuture<QString> delayed_string_func()
{
	auto retval = QtConcurrent::run([](){
		// Sleep for a second.
		qDb() << "ENTER, SLEEPING FOR 1 SEC";
		QThread::sleep(1);
		qDb() << "SLEEP COMPLETE";
		return QString("HELLO");
	});

	return retval;
}

void AsyncTests::UnwrapTest()
{

//	auto future = QtConcurrent::run(delayed_string_func);
	ExtFuture<ExtFuture<QString>> future = ExtAsync::run(delayed_string_func);
//	ExtFuture<QString> future = ExtAsync::run([&](ExtFuture<QString> future) {
//		qDb() << "TEST: Running from main run lambda.";
//		// Sleep for a second to make sure then() doesn't run before we get to the Q_ASSERT() after this chain.
//		QThread::sleep(1);
//		future.reportResult("Hello1");
//		future.reportResult("Hello2");
//		future.reportFinished(new QString("FINISHED"));
//		qDb() << "TEST: Finished from main run lambda.";
//		return ExtFuture<QString>(); //("FINISHED");
//	});

//	ExtFuture<QString> unwrapped_future = future.unwrap();
}
