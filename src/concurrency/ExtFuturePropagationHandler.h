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
#include <memory>
#include <deque>
#include <atomic>

// Qt5
#include <QFuture>
#include <QSemaphore>
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

	static std::shared_ptr<ExtFuturePropagationHandler> s_the_cancel_prop_handler;

	static void InitStaticExtFutureState();


//	using FutureType = ExtFuture<Unit>;
	using FutureType = QFuture<void>;


	/**
	 * Register for a cancel propagation from downstream to upstream.  Threadsafe.
	 * @param downstream
	 * @param upstream
	 */
	void register_cancel_prop_down_to_up(FutureType downstream, FutureType upstream);

	/**
	 * Unregister for a cancel propagation from downstream to upstream.  Threadsafe.
	 * @param downstream
	 * @param upstream
	 */
	void unregister_cancel_prop_down_to_up(FutureType downstream, FutureType upstream);

	/**
	 * Cancels all remaining ExtFutures and waits for the patrol loop to end.
	 * Call this just prior to deleting this object.
	 * @return
	 */
	bool close(bool warn_calling_from_destructor = false);

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

	// Protected data members

	/// Shared mutex because we're highly reader-writer.
	mutable std::mutex m_mutex;
	/// Condition var our main loop will wait on.
	std::condition_variable m_cv;
	std::condition_variable m_cv_complete;

	/**
	 * When we're being destroyed, we can't accept any new futures to watch, so immediately cancel them.
	 */
	std::atomic_bool m_closed {false};

	// Not really a map.  Not really even close to a map.
	using map_type = std::deque<std::tuple<FutureType, FutureType>>;

	map_type m_down_to_up_cancel_map;
	using map_pair_type = decltype(m_down_to_up_cancel_map)::value_type;
	using nonconst_key_map_value_type = std::pair<FutureType, FutureType>;

	QThread* m_patrol_thread {nullptr};

public:
	/// Return a pointer to the singleton.
	static std::shared_ptr<ExtFuturePropagationHandler> IExtFuturePropagationHandler();

};

} /* namespace ExtAsync */

#endif /* SRC_CONCURRENCY_EXTFUTUREPROPAGATIONHANDLER_H_ */
