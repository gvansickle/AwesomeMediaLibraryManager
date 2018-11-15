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
#include <QException>
#include <QtConcurrentRun>

// Ours
#include <utils/DebugHelpers.h>
#include "../ExtFuture.h"
#include "../ExtAsync_traits.h"
#include "../ExtAsyncExceptions.h"

//template <class T>
//class ExtFuture;

namespace ExtAsync
{

} // END namespace ExtAsync


#endif /* SRC_CONCURRENCY_IMPL_EXTASYNC_IMPL_H_ */
