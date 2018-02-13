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

void ExtAsyncTest()
{
	qDb() << "TEST START";

	int val = 0;

//	ExtFuture<QString> promise;

	auto future = ExtAsync::run([&](ExtFuture<QString> future) {
		qDb() << "TEST: Running from main run lambda.";
		val++;
M_WARNING("TODO: Shouldn't need this I don't think");
		future.reportFinished(new QString("FINISHED"));
	})
	.then([&](){
		qDb() << "TEST: Running from then()";
		Q_ASSERT(val == 1);
		val++;
	});

	Q_ASSERT(val == 2);

	qDb() << "TEST END";
}
