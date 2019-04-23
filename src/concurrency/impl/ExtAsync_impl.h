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

/**
 * @file ExtAsync_impl.h
 */
#ifndef SRC_CONCURRENCY_IMPL_EXTASYNC_IMPL_H_
#define SRC_CONCURRENCY_IMPL_EXTASYNC_IMPL_H_

#include <config.h>

// Std C++
#include <functional>

// Future Std C++
#include <future/future_type_traits.hpp>
#include <future/function_traits.hpp>
#include <future/cpp14_concepts.hpp>
#include <future/Unit.hpp>

// Qt5
#include <QObject>
#include <QException>
#include <QtConcurrentRun>

// Ours
#include <utils/DebugHelpers.h>
//#include "../ExtFuture.h" ///< Can't include this because it uses context_has_event_loop().
#include "../ExtAsync_traits.h"
#include "../ExtAsyncExceptions.h"

//template <class T>
//class ExtFuture;

namespace ExtAsync
{
	namespace detail
	{
		/**
		 * Helper for checking if the given @a context object has an event loop.
		 * @param obj
		 * @return
		 */
		inline static bool context_has_event_loop(QObject* context)
		{
			if(context == nullptr)
			{
				// No event loop, not even an object.
				return false;
			}
			// If non-null, make sure context has an event loop.
			QThread* ctx_thread = context->thread();
			// Something's broken if we don't have a thread, I think we need to assert in this case.
			Q_ASSERT(ctx_thread != nullptr);
			if(ctx_thread->eventDispatcher() != nullptr)
			{
				// Has an event dispatcher, so has event loop.
				return true;
			}
			return false;
		};
	}
} // END namespace ExtAsync


#endif /* SRC_CONCURRENCY_IMPL_EXTASYNC_IMPL_H_ */
