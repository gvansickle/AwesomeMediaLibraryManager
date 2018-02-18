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

#include "ExtAsync.h"

#include <utils/DebugHelpers.h>

void ExtAsyncTest(QObject* context)
{
	qDb() << "TEST START";

	int val = 0;
	bool tap_ran = false;

//	ExtFuture<QString> promise;

	auto then_func1 = [=](QString str) -> QString {
		qDb() << "Then1";
//		Q_ASSERT(tap_ran);
		return QString("Then1");
	};

	ExtFuture<QString> future = ExtAsync::run([&](ExtFuture<QString> future) {
		qDb() << "TEST: Running from main run lambda.";
		val = 1;
		// Sleep for a second to make sure then() doesn't run before we get to the Q_ASSERT() after this chain.
		QThread::sleep(1);
		future.reportResult("Hello1");
		future.reportResult("Hello2");
		future.reportFinished(new QString("FINISHED"));
		qDb() << "TEST: Finished from main run lambda.";
//		return QString("FINISHED");
	})
//	.on_result([&](QString str) -> void {
//		qDb() << "TEST: ON_RESULT_CALLED, VALUE:" << str;
//	})
	.tap(context, [=](QString value) -> void {
		qDb() << "TEST: TAP CALLBACK CALLED, VALUE:" << value << "VAL:" << val;
//		Q_ASSERT(val == 1);
//		val = 2;
//		tap_ran = true;
	})
//	.then(context, [=](QString str) -> QString {
//		qDb() << "Then1";
////		Q_ASSERT(tap_ran);
//		return QString("Then1");
//	})
	;
#if 0
	.then([&](ExtFuture<QString>& the_future) -> ExtFuture<QString> {
		// Should only run after .tap() is called, but not after the tap callback is called.
		qDb() << "TEST: Running from then()";
		Q_ASSERT(val == 1);
		Q_ASSERT(tap_ran);
		val = 3;
		M_WARNING("LEAK");
		ExtFuture<QString> *retval = new ExtFuture<QString>();
		return *retval;
	});
#endif


#if 0
	.then([&](){
		qDb() << "TEST: Inside second then()";
		Q_ASSERT(val == 3);
	});
#endif


#if 0
	fw->onProgressChange([=](int min, int val, int max, QString text) -> void {
						qDb() << "PROGRESS+TEXT SIGNAL: " << min << val << max << text;
//						Q_EMIT progressChanged(min, val, max, text);
						;})
					.onReportResult([&future](QString s, int index) {
						Q_UNUSED(index);
						/// @note This lambda is called in an arbitrary thread context.
	//					qDebug() << M_THREADNAME() << "RESULT:" << s << index;

						// This is not threadsafe:
						/// WRONG: this->m_current_libmodel->onIncomingFilename(s);

	//					/// EXPERIMENTAL
	//					static int fail_counter = 0;
	//					fail_counter++;
	//					if(fail_counter > 5)
	//					{
	////						future_interface.cancel();
	//						throw QException();
	//					}

						// This is threadsafe.
//						QMetaObject::invokeMethod(this->m_current_libmodel, "onIncomingFilename", Q_ARG(QString, s));
					})
					.setFuture(future);
#endif

	qDb() << "START Waiting on future:" << &future << future;
	future.wait();
	qDb() << "END Waiting on future:" << &future << future;

	// Should get here some time before .then().
//	Q_ASSERT(val == 2);

	qDb() << "TEST END";
}
