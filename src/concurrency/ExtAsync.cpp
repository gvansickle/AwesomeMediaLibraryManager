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
 * @file static_assert()s for various ExtAsync<> properties.
 */

#include "ExtAsync.h"

#include <type_traits>
#include <future/future_type_traits.hpp>

#include "ExtAsync_traits.h"
#include <utils/DebugHelpers.h>

/**
 * - Cancellation and Exceptions
 *
 * Per std::experimental::shared_future::then() at @link https://en.cppreference.com/w/cpp/experimental/shared_future/then
 * "Any value returned from the continuation is stored as the result in the shared state of the returned future object.
 *  Any exception propagated from the execution of the continuation is stored as the exceptional result in the shared
 *  state of the returned future object."
 *
 * Per @link https://software.intel.com/en-us/node/506075 (tbb), referring to task_group_context objects:
 * "Exceptions propagate upwards. Cancellation propagates downwards. The opposition interplays to cleanly stop a nested
 * computation when an exception occurs."
 */

/// ExtFuture<> Concept checks.
static_assert(IsExtFuture<ExtFuture<int>>, "");
static_assert(NonNestedExtFuture<ExtFuture<int>>, "");
static_assert(!NonNestedExtFuture<ExtFuture<ExtFuture<int>>>, "");
static_assert(NestedExtFuture<ExtFuture<ExtFuture<int>>>, "");
static_assert(!NestedExtFuture<ExtFuture<int>>, "");
static_assert(!IsExtFuture<int>, "");

/// ExtFuture<T> sanity checks.
// From http://en.cppreference.com/w/cpp/experimental/make_ready_future:
// "If std::decay_t<T> is std::reference_wrapper<X>, then the type V is X&, otherwise, V is std::decay_t<T>."
static_assert(std::is_same_v<decltype(make_ready_future(4)), ExtFuture<int> >);
int v;
static_assert(!std::is_same_v<decltype(make_ready_future(std::ref(v))), ExtFuture<int&> >);
/// @todo
//    static_assert(std::is_same_v<decltype(make_ready_future()), ExtFuture<Unit> >, "");
static_assert(!std::is_same_v<QFuture<long>, ExtFuture<long>>);
//static_assert(std::is_convertible_v<QFuture<long>, ExtFuture<long>>);
static_assert(std::is_convertible_v<ExtFuture<long>, QFuture<long>>);

static_assert(std::is_copy_constructible_v<ExtFuture<long>>);
/// Should not be "really" move constructable or assignable, but not sure how to check that ATM.
/// Should be is_move_constructible_v via the copy constructor taking "const T&".
/// @link  https://en.cppreference.com/w/cpp/types/is_move_constructible
static_assert(std::is_move_constructible_v<ExtFuture<long>>);
static_assert(std::is_move_assignable_v<ExtFuture<long>>);
/// Should not be trivially anything, i.e. a memmove() is sufficient to copy/move/assign.
static_assert(!std::is_trivially_copy_constructible_v<ExtFuture<long>>);
static_assert(!std::is_trivially_copy_assignable_v<ExtFuture<long>>);
static_assert(!std::is_trivially_move_constructible_v<ExtFuture<long>>);
static_assert(!std::is_trivially_move_assignable_v<ExtFuture<long>>);
