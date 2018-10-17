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

// Ours
#include "ExtFuture.h"

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

	using map_type = std::multimap<ExtFuture<bool>, ExtFuture<bool>>;
	map_type m_down_to_up_cancel_map;
	using map_pair_type = decltype(m_down_to_up_cancel_map)::value_type;

	QThread* m_patrol_thread {nullptr};

};


} /* namespace ExtAsync */

#endif /* SRC_CONCURRENCY_EXTFUTUREPROPAGATIONHANDLER_H_ */
