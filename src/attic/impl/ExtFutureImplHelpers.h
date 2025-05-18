/*
 * Copyright 2018, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef EXTFUTUREIMPLHELPERS_H
#define EXTFUTUREIMPLHELPERS_H

#if EVER_NEEDED_AGAIN

// Std C++
#include <type_traits>
#include <functional>
#include <thread>
#include <future>
#include <condition_variable>
#include <chrono>

// Qt
#include <QFutureWatcher>
#include <QList>
#include <QThreadPool>

// Boost
#include <boost/thread.hpp>

// Ours
#include <utils/DebugHelpers.h>
#include <future/Unit.hpp>
#include <concurrency/ExtAsyncExceptions.h>
//#include "ExtAsync_RunInThread.h"
#include <concurrency/ExtAsync.h>
#include <concurrency/ExtFutureWatcher.h>
#include <concurrency/impl/ManagedExtFutureWatcher_impl.h>

// Generated
//#include "logging_cat_ExtFuture.h"

template <class T>
class ExtFuture;

namespace ExtFuture_detail
{

}; // END ns ExtFuture_detail

#endif // EVER_NEEDED_AGAIN

#endif // EXTFUTUREIMPLHELPERS_H
