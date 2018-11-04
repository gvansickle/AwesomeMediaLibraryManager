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
#include <QSemaphore>

// Ours
#include "ExtFuture.h"

namespace ExtAsync
{

static std::shared_ptr<ExtFuturePropagationHandler> s_the_cancel_prop_handler {nullptr};

void ExtFuturePropagationHandler::InitStaticExtFutureState()
{
	ExtAsync::s_the_cancel_prop_handler = ExtAsync::ExtFuturePropagationHandler::make_handler();
}

std::shared_ptr<ExtFuturePropagationHandler> ExtFuturePropagationHandler::IExtFuturePropagationHandler()
{
	Q_ASSERT_X(static_cast<bool>(ExtAsync::s_the_cancel_prop_handler) == true, __PRETTY_FUNCTION__,
			   "Global ExtFuturePropagationHandler not initialized.");
	return ExtAsync::s_the_cancel_prop_handler;
}

ExtFuturePropagationHandler::ExtFuturePropagationHandler()
{
	m_patrol_thread = QThread::create(&ExtFuturePropagationHandler::patrol_for_cancels, this);
	Q_ASSERT(m_patrol_thread != nullptr);
	m_patrol_thread->start();
}

ExtFuturePropagationHandler::~ExtFuturePropagationHandler()
{
	// @todo This should never have anything to cancel.
	cancel_all_and_wait(true);
}

// Static
std::unique_ptr<ExtFuturePropagationHandler> ExtFuturePropagationHandler::make_handler()
{
	return std::make_unique<ExtFuturePropagationHandler>();
}

void ExtFuturePropagationHandler::register_cancel_prop_down_to_up(FutureType downstream, FutureType upstream)
{
	std::unique_lock write_locker(m_shared_mutex);

	if(m_cancel_incoming_futures)
	{
		qWr() << "SHUTTING DOWN, CANCELING INCOMING FUTURES:";// << downstream << upstream;
		downstream.cancel();
		upstream.cancel();
		return;
	}

	// Add the two futures to the map.
	m_down_to_up_cancel_map.push_back({downstream, upstream});
	// << state(downstream) << state(upstream);
	m_num_pairs_to_patrol.release();
	qIn() << "Registered future pair, now have" << m_down_to_up_cancel_map.size() << "pairs registered.";
//	AMLM_ASSERT_EQ(m_num_pairs_to_patrol.available(), m_down_to_up_cancel_map.size());
}

void ExtFuturePropagationHandler::unregister_cancel_prop_down_to_up(ExtFuturePropagationHandler::FutureType downstream, ExtFuturePropagationHandler::FutureType upstream)
{
	std::unique_lock write_locker(m_shared_mutex);

	// Remove all instances of the pair.
	auto erase_me = std::remove_if(m_down_to_up_cancel_map.begin(), m_down_to_up_cancel_map.end(),
								   [=](auto& val){ return val == std::make_tuple(downstream, upstream); });
	// ...and finally erase them all.
	auto num_to_erase = std::distance(erase_me, m_down_to_up_cancel_map.end());
	m_down_to_up_cancel_map.erase(erase_me);
	bool success = m_num_pairs_to_patrol.tryAcquire(num_to_erase);
	Q_ASSERT(success);
}

bool ExtFuturePropagationHandler::cancel_all_and_wait(bool warn_calling_from_destructor)
{
	std::unique_lock write_locker(m_shared_mutex);

	m_cancel_incoming_futures = true;

	if(warn_calling_from_destructor && !m_down_to_up_cancel_map.empty())
	{
		qWr() << "CALLED FROM DESTRUCTOR WITH NON-EMPTY FUTURE LIST:"  << m_down_to_up_cancel_map.size() << "future pairs....";
	}

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

	qIn() << "Starting patrol";

	std::atomic_bool logged_yield_msg {false};

	while(true)
	{
		// Thread-safe step 0: Wait for the possibility of anything to do.
		// This will block until there is.
		m_num_pairs_to_patrol.acquire();


		// Ok, there's something to do.  Lock until the next loop.
		do
		{
			std::unique_lock write_locker(m_shared_mutex);

			if(m_cancel_incoming_futures)
			{
				// Special case, We're being canceled.  Break out of this loop.
				qIn() << "ExtFuturePropagationHandler being canceled...";
				m_num_pairs_to_patrol.release();
				return;
			}
			else
			{
				// One or more ExtFutures wants us to propagate their cancel.
				if(m_down_to_up_cancel_map.empty())
				{
					//				if(!logged_yield_msg)
					//				{
					qWr() << "No entries, nothing to propagate, yielding...";
					logged_yield_msg = true;
					//				}
					// Nothing to propagate, yield and loop until there is.
					QThread::yieldCurrentThread();
					continue;
				}
				else
				{
					// There's an entry, we need to look at it.
					logged_yield_msg = false;
				}
			}

			// Remove any entries with Finished but not Canceled keys.
			// Nothing for us to propagate in this case, and we no longer need to watch The Future(tm).
			auto erase_me_too = std::remove_if(m_down_to_up_cancel_map.begin(), m_down_to_up_cancel_map.end(),
											   [=](auto& val){
				return std::get<0>(val).isFinished() && !std::get<0>(val).isCanceled();
			});
			if((m_down_to_up_cancel_map.end() - erase_me_too) > 0)
			{
				auto num_erased = std::distance(erase_me_too, m_down_to_up_cancel_map.end());
				qIn() << "ERASING" << num_erased << " FINISHED NOT CANCELED PAIRS";
				m_down_to_up_cancel_map.erase(erase_me_too, m_down_to_up_cancel_map.end());
				m_num_pairs_to_patrol.release(num_erased);
				qIn() << "Remaining future pairs:" << m_down_to_up_cancel_map.size();
			}


			// Cancel propagation and delete loops.

			// Look for any Canceled or Finished ExtFuture<>s and copy them to a local map.
			// This is read-only, and won't invalidate any iterators.

			// The local map.
			map_type canceled_ExtFuture_map;

			for(const auto& it : m_down_to_up_cancel_map)
			{
				if(std::get<0>(it).isCanceled() || std::get<0>(it).isFinished())
				{
					// Found a canceled key, add the entry to the local map.
					qIn() << "Propagating Cancel/Finished from" << &std::get<0>(it) << "to" << &std::get<1>(it);
					canceled_ExtFuture_map.push_back(it);
				}
			}

			// Did we find any?
			if(canceled_ExtFuture_map.empty())
			{
				// No, go back to waiting.
				qWr() << "FOUND NO CANCELED FUTURES";
				continue;
			}

			// Step 2: Cancel The Future(tm).
			// Propagate the cancels we found to the upstream ExtFuture.
			// We have the copies, and the futures are threadsafe for .cancel(), so we don't even need a lock here.
			qDb() << "Propagating cancels from" << canceled_ExtFuture_map.size() << "ExtFuture<>s.";
			std::for_each(canceled_ExtFuture_map.begin(), canceled_ExtFuture_map.end(),
						  [](map_pair_type p){
				if(std::get<0>(p).isCanceled())
				{
					std::get<1>(p).cancel();
				}
			});

			// Step 3: Remove The Future(tm).
			// Delete the keys which we just propagated the cancels from.
			qDb() << "Deleting" << canceled_ExtFuture_map.size() << "canceled ExtFuture<>s.";

			// For each pair we want to delete...
			for(auto it = canceled_ExtFuture_map.cbegin(); it != canceled_ExtFuture_map.cend(); ++it)
			{
				// ... remove all instances of it...
				auto erase_me = std::remove_if(m_down_to_up_cancel_map.begin(), m_down_to_up_cancel_map.end(),
											   [=](auto& val){ return val == *it;});
				auto num_erased = std::distance(erase_me, m_down_to_up_cancel_map.end());
				m_num_pairs_to_patrol.release(num_erased);
				// ...and finally erase them all.
				m_down_to_up_cancel_map.erase(erase_me);
			}

			// Threadsafe step 4: Let the destructor do it.
			// We now don't need anything in canceled_ExtFuture_map, so we can just let it get deleted.
			// The deletions of the contained ExtFuture<>s shouldn't need any synchronization.
		} while(0);
	}
}

void ExtFuturePropagationHandler::cancel_all()
{
	for(map_pair_type& val : m_down_to_up_cancel_map)
	{
		// Cancel everything we see, key and value.
		std::get<0>(val).cancel();
		std::get<1>(val).cancel();
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
			if((std::get<0>(val).isFinished() || std::get<0>(val).isCanceled())
					&& (std::get<1>(val).isFinished() ||(std::get<1>(val).isCanceled()))
					)
			{
				// Both canceled, erase them.
				qIn() << "erasing future pair:";// << val.first << val.second;
				it = m_down_to_up_cancel_map.erase(it);
				qIn() << "Future count now:" << m_down_to_up_cancel_map.size();
			}
		}
	}
}

} /* namespace ExtAsync */
