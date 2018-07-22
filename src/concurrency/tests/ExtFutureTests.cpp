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

#include "ExtFutureTests.h"

// Std C++
#include <type_traits>
#include <atomic>
#include <functional>

// Qt5
#include <QString>
#include <QTest>
#include <QFutureInterfaceBase> // shhh, we're not supposed to use this.  For calling .reportFinished() on QFuture<>s inside a run().

// Google Test
#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>

// Ours
#include <future/future_type_traits.hpp>
#include "../ExtAsync.h"
#include <tests/TestHelpers.h>
#include "../../future/function_traits.hpp"
