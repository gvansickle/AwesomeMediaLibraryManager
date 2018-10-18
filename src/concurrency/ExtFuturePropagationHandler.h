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
 * @file ExtFuturePropagationHandler.h
 */
#ifndef SRC_CONCURRENCY_EXTFUTUREPROPAGATIONHANDLER_H_
#define SRC_CONCURRENCY_EXTFUTUREPROPAGATIONHANDLER_H_

// Std C++
#include <shared_mutex>
#include <map>
#include <atomic>

// Qt5
#include <QFuture>
class QThread;

// Ours
//#include "ExtFuture.h"

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
	virtual ~ExtFuturePropagationHandler();

	static std::unique_ptr<ExtFuturePropagationHandler> make_handler();

	/**
	 * Register for a cancel propagation from downstream to upstream.  Threadsafe.
	 * @param downstream
	 * @param upstream
	 */
	void register_cancel_prop_down_to_up(QFuture<void> downstream, QFuture<void> upstream);

	/**
	 * Call this just prior to deleting this object.
	 * @return
	 */
	bool cancel_all_and_wait();

protected:

	/**
	 * The thread function which loops looking for futures registered to this object
	 * which are canceled, and propagates the .cancel() signal.
	 */
	void patrol_for_cancels();

	/**
	 * Cancels all registered futures.  Does not wait, caller must acquire the mutex.
	 */
	void cancel_all();

	void wait_for_finished_or_canceled();

	// Shared mutex because we're highly reader-writer.
	std::shared_mutex m_shared_mutex;

	/**
	 * When we're being destroyed, we can't accept any new futures to watch, so immediately cancel them.
	 */
	std::atomic_bool m_cancel_incoming_futures {false};

	using map_type = std::multimap<QFuture<void>, QFuture<void>>;
	map_type m_down_to_up_cancel_map;
	using map_pair_type = decltype(m_down_to_up_cancel_map)::value_type;
	using nonconst_key_map_value_type = std::pair<map_type::key_type, map_type::mapped_type>;

	QThread* m_patrol_thread {nullptr};

};


} /* namespace ExtAsync */

#endif /* SRC_CONCURRENCY_EXTFUTUREPROPAGATIONHANDLER_H_ */
