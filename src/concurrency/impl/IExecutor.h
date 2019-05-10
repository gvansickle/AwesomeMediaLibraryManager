/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
 * @file IExecutor.h
 */
#ifndef SRC_CONCURRENCY_IMPL_IEXECUTOR_H_
#define SRC_CONCURRENCY_IMPL_IEXECUTOR_H_

// Std C++.
#include <functional>


class IWorktype
{
public:

};

/**
 * Pseudo-C++YY thread::executor interface.
 * See Boost's take on this, which I'm vaguely following here.
 * @link https://www.boost.org/doc/libs/1_70_0/doc/html/thread/synchronization.html#thread.synchronization.executors.ref.executor
 */
class IExecutor
{
public:
	// Per Boost, this is a couple typedefs separated from Callable, which is void(void).
	template<class T = std::function<void()>;
	using work = T;

	IExecutor() = default;
	virtual ~IExecutor() = default;

	/**
	 * Submit work to be scheduled
	 */
	virtual void submit() = 0;
};



#endif /* SRC_CONCURRENCY_IMPL_IEXECUTOR_H_ */
