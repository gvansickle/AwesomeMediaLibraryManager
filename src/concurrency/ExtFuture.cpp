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

// Ours
#include "ExtFuturePropagationHandler.h"

//namespace ExtAsync
//{

///**
// * Monitors ExtFuture<>s for cancelation and propagates it up the .then() chain.
// */
//static std::shared_ptr<ExtFuturePropagationHandler> s_the_cancel_prop_handler;// {nullptr};

//};

//template<typename T>
//void ExtFuture<T>::InitStaticExtFutureState()
//{
//	ExtAsync::s_the_cancel_prop_handler = ExtAsync::ExtFuturePropagationHandler::make_handler();
//}

//template<typename T>
//std::shared_ptr<ExtAsync::ExtFuturePropagationHandler> ExtFuture<T>::IExtFuturePropagationHandler()
//{
//	Q_ASSERT(static_cast<bool>(ExtAsync::s_the_cancel_prop_handler) == true);
//	return ExtAsync::s_the_cancel_prop_handler;
//}

/// @name Explicit instantiations to try to get compile times down.
template class ExtFuture<Unit>;
template class ExtFuture<bool>;
template class ExtFuture<int>;
template class ExtFuture<long>;
template class ExtFuture<std::string>;
template class ExtFuture<double>;
template class ExtFuture<QString>;
template class ExtFuture<QByteArray>;




