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
 * @file ExtFuturePropagationHandler.cpp
 */
#include "ExtFuturePropagationHandler.h"

// Std C++
#include <shared_mutex>

namespace ExtAsync
{

ExtFuturePropagationHandler::ExtFuturePropagationHandler()
{
	m_patrol_thread = QThread::create(&ExtFuturePropagationHandler::patrol_for_cancels, this);
	Q_ASSERT(m_patrol_thread != nullptr);
	m_patrol_thread->start();
}

ExtFuturePropagationHandler::~ExtFuturePropagationHandler()
{

}

void ExtFuturePropagationHandler::register_cancel_prop_down_to_up(ExtFuture<bool> downstream, ExtFuture<bool> upstream)
{
	std::unique_lock write_locker(m_shared_mutex);

	// Add the two futures to the map.
	m_down_to_up_cancel_map.insert({downstream, upstream});
}

void ExtFuturePropagationHandler::patrol_for_cancels()
{
	// Again, yes, I know, there should be a better way to handle this.  There simply isn't.
	// Well, actually there is, but step 1 is to get this functionality to work.

	while(true)
	{
		// Thread-safe step 0: Wait for the possibility of anything to do.
		while(true)
		{
			std::shared_lock read_locker(m_shared_mutex);
			if(m_down_to_up_cancel_map.empty())
			{
				// Nothing to propagate, yield and loop until there is.
				QThread::yieldCurrentThread();
			}
		}

		// Cancel propagation and delete loops.
		{
			// Thread-safe step 1: Look for any canceled ExtFuture<>s and copy them to a local map.

			// This is read-only, and won't invalidate any iterators.

			std::multimap<ExtFuture<bool>, ExtFuture<bool>> canceled_ExtFuture_map;

			{
				std::shared_lock read_locker(m_shared_mutex);

				for(auto it : m_down_to_up_cancel_map)
				{
					if(it.first.isCanceled())
					{
						// Found a canceled key, add it to the local map.
						canceled_ExtFuture_map.insert(it);
					}
				}
			}

			// Threadsafe step 2: Cancel The Future(tm).
			// Propagate the cancels we found to the upstream ExtFuture.
			// We have the copies, so we don't even need a lock here.
			qDb() << "Propagating cancels from" << canceled_ExtFuture_map.size() << "ExtFuture<>s.";
			std::for_each(canceled_ExtFuture_map.begin(), canceled_ExtFuture_map.end(),
						  [](map_pair_type p){
				p.second.cancel();
			});

			// Threadsafe step 3: Remove The Future(tm).
			// Delete the keys which we just propagated the cancels from.
			{
				std::unique_lock write_locker(m_shared_mutex);
				qDb() << "Deleting" << canceled_ExtFuture_map.size() << "canceled ExtFuture<>s.";
				for(auto it = canceled_ExtFuture_map.cbegin(); it != canceled_ExtFuture_map.cend(); ++it)
				{
					m_down_to_up_cancel_map.erase(it->first);
				}
			}

			// Threadsafe step 4: Let the destructor do it.
			// We now don't need anything in canceled_ExtFuture_map, so we can just let it get deleted.
			// The deletions of the contained ExtFuture<>s shouldn't need any synchronization.
		}
	}

}

void ExtFuturePropagationHandler::cancel_all()
{
	// Only need a read lock if we're canceling, since we're not modifying the map struct.
//	std::shared_lock read_locker(m_shared_mutex);

//	for(auto it : m_down_to_up_cancel_map)
//	{
//		(*it).first.cancel();
//	}
}

void ExtFuturePropagationHandler::wait_for_finished_or_canceled()
{

}

} /* namespace ExtAsync */
