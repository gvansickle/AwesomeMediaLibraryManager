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

//	ExtFuture<QString> promise;

	auto future = ExtAsync::run([&](ExtFuture<QString> future) {
		qDb() << "TEST: Running from main run lambda.";
		val = 1;
		// Sleep for a second to make sure then() doesn't run before we get to the Q_ASSERT() after this chain.
		QThread::sleep(1);
M_WARNING("TODO: Shouldn't need this I don't think");
		future.reportFinished(new QString("FINISHED"));
	})
	.tap(context, [&](QString value) -> void {
		qDb() << "TEST: TAP CALLBACK CALLED, VALUE:" << value;
		Q_ASSERT(val == 1);
		val = 2;
	})
	.then([&](){
		// Should only run after .tap() is called, but not after the tap callback is called.
		qDb() << "TEST: Running from then()";
		Q_ASSERT(val == 1);
		val = 3;
	});

	// Should get here some time before .then().
//	Q_ASSERT(val == 2);

	qDb() << "TEST END";
}
