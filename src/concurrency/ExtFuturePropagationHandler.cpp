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

// Qt5
#include <QThread>

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
	// @todo This should never have anything to cancel.
	cancel_all_and_wait();
}

// Static
std::unique_ptr<ExtFuturePropagationHandler> ExtFuturePropagationHandler::make_handler()
{
	return std::make_unique<ExtFuturePropagationHandler>();
}

void ExtFuturePropagationHandler::register_cancel_prop_down_to_up(ExtFuture<bool> downstream, ExtFuture<bool> upstream)
{
	std::unique_lock write_locker(m_shared_mutex);

	if(m_cancel_incoming_futures)
	{
		qWr() << "SHUTTING DOWN, CANCELING INCOMING FUTURES:" << downstream << upstream;
		downstream.cancel();
		upstream.cancel();
		return;
	}

	// Add the two futures to the map.
	m_down_to_up_cancel_map.insert({downstream, upstream});
}

bool ExtFuturePropagationHandler::cancel_all_and_wait()
{
	std::unique_lock write_locker(m_shared_mutex);

	m_cancel_incoming_futures = true;

	while(!m_down_to_up_cancel_map.empty())
	{
		qIn() << "Canceling" << m_down_to_up_cancel_map.size() << "future pairs....";
		cancel_all();
		wait_for_finished_or_canceled();
	}

	return true;
}

void ExtFuturePropagationHandler::patrol_for_cancels()
{
	// Again, yes, I know, there should be a better way to handle this.  There simply isn't.
	// Well, actually there is, but step 1 is to get this functionality to work.

	while(true)
	{
		// Thread-safe step 0: Wait for the possibility of anything to do.
		// Yeah yeah, bad, I know.  I'll fix it.
		while(true)
		{
			std::shared_lock read_locker(m_shared_mutex);
			if(m_cancel_incoming_futures)
			{
				// We're being canceled.  Break out of this loop.
				return;
			}
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
						// Found a canceled key, add the entry to the local map.
						canceled_ExtFuture_map.insert(it);
					}
				}
			}

			// Threadsafe step 2: Cancel The Future(tm).
			// Propagate the cancels we found to the upstream ExtFuture.
			// We have the copies, and the futures are threadsafe for .cancel(), so we don't even need a lock here.
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
	for(map_pair_type& val : m_down_to_up_cancel_map)
	{
		// Cancel everything we see, key and value.
		const_cast<map_type::key_type&>(val.first).cancel();
		val.second.cancel();
	}
}

void ExtFuturePropagationHandler::wait_for_finished_or_canceled()
{
	while(!m_down_to_up_cancel_map.empty())
	{
		for(auto it = m_down_to_up_cancel_map.begin(); it != m_down_to_up_cancel_map.end(); )
		{
			auto val = *it;
			// Erase everything that has a canceled or finished key and value.
			/// @todo timeout/error handling
			if((val.first.isFinished() || val.first.isCanceled())
					&& (val.second.isFinished() ||(val.second.isCanceled()))
					)
			{
				// Both canceled, erase them.
				qIn() << "erasing future pair:" << val.first << val.second;
				it = m_down_to_up_cancel_map.erase(it);
			}
		}
	}
}

} /* namespace ExtAsync */
