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
 * @file ExtFuture.cpp
 *
 * Notes:
 *
 * - QFuture<> lost a lot of special member functions on this commit:
 * @link https://git.qt.io/consulting-usa/qtbase-xcb-rendering/commit/9c016cefe9a81de3b01019cb1eb1363b05e3b448
 * That's why there's no real copy constructor etc. defined - it now relies on the compiler-generated
 * (but not = default, those fail) ones.
 *
 * - The QMutex
 * There's a QMutex which lives in the QFutureInterfaceBasePrivate d instance of the QFutureInterfaceBase.
 * It's private, but a pointer to it is avalable via "QMutex *QFutureInterfaceBase::mutex() const".
 * Most of the public QFuture{Interface} interfaces lock this mutex, with the notable exception of the isFinished()/isCanceled()/etc.
 * state query functions, which simply query the bits in an atomic variable.
 *
 */

// Associated header.
#include "ExtFuture.h"

// Std C++
#include <shared_mutex>
#include <map>
#include <algorithm>

// Qt5
#include <QFuture>
#include <QThread>

namespace ExtAsync
{

/**
 * Monitors ExtFuture<>s for cancelation and propagates it up the .then() chain.
 *
 * @note Yeah I know, should be a better way to handle this.  There isn't.
 */
class ExtFuturePropagationHandler
{
public:
	ExtFuturePropagationHandler();
	~ExtFuturePropagationHandler();

	/**
	 * Register for a cancel propagation from downstream to upstream.
	 * @param downstream
	 * @param upstream
	 */
	void register_cancel_prop_down_to_up(ExtFuture<bool> downstream, ExtFuture<bool> upstream);


protected:

	void patrol_for_cancels();

	void cancel_all();

	void wait_for_finished_or_canceled();

	// Shared mutex because we're highly reader-writer.
	std::shared_mutex m_shared_mutex;

	std::multimap<ExtFuture<bool>, ExtFuture<bool>> m_down_to_up_cancel_map;
	using map_pair_type = std::pair<ExtFuture<bool>, ExtFuture<bool>>; //decltype(m_down_to_up_cancel_map)::value_type;

	QThread* m_patrol_thread {nullptr};

};

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
		// Thread-safe step 1: Look for any canceled ExtFuture<>s and copy them to their own map.

		// This is read-only, and won't invalidate any iterators.

		std::multimap<ExtFuture<bool>, ExtFuture<bool>> canceled_ExtFuture_map;

		{
			std::shared_lock read_locker(m_shared_mutex);

			for(auto it : m_down_to_up_cancel_map)
			{
				if(it.first.isCanceled())
				{
					canceled_ExtFuture_map.insert(it);
				}
			}
		}

		// Threadsafe step 2: Cancel The Future(tm).
		// We have the copies, so we don't even need a lock here.
		std::for_each(canceled_ExtFuture_map.begin(), canceled_ExtFuture_map.end(),
					  [](map_pair_type p){
			p.second.cancel();
		});

		// Threadsafe step 3: Remove The Future(tm).
#error "TODO"
	}

}

void ExtFuturePropagationHandler::cancel_all()
{

}

void ExtFuturePropagationHandler::wait_for_finished_or_canceled()
{

}

}

/// @name Explicit instantiations to try to get compile times down.
template class ExtFuture<Unit>;
template class ExtFuture<bool>;
template class ExtFuture<int>;
template class ExtFuture<long>;
template class ExtFuture<std::string>;
template class ExtFuture<double>;
template class ExtFuture<QString>;
template class ExtFuture<QByteArray>;


