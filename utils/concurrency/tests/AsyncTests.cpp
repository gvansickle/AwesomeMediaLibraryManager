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
		return QString("HELLO");
	});

	return retval;
}

void AsyncTests::BasicTest()
{
	qDb() << "START";

	ExtFuture<QString> future = ExtAsync::run(delayed_string_func_1);
	future
	.then([](QString str){
		qDb() << "Then1, got str:" << str;
		return str;
	})
	.then([](QString str) -> QString {
		qDb() << "Then2, got str:" << str;
		return str;
	})
	.then([](QString str) -> QString {
		qDb() << "Then3, got str:" << str;
		return str;
	});

	future.wait();

	qDb() << "Complete";

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
