/*
 * Copyright 2017, 2018, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef CONNECTHELPERS_H
#define CONNECTHELPERS_H

// Boost
#include <boost/callable_traits.hpp>
namespace ct = boost::callable_traits;

// Std C++ backfill
#include <future/cpp14_concepts.hpp>

// Qt5
#include <QAction>
#include <QApplication>
#include <QMetaObject>

// Ours
#include "DebugHelpers.h"

template <typename T, typename F>
QMetaObject::Connection connect_trig(QAction* sender, const T* receiver, F slot, Qt::ConnectionType type = Qt::AutoConnection)
{
  return QAction::connect(sender, &QAction::triggered, receiver, slot, type);
}

template <typename Sender, typename Receiver, typename Slot>
QMetaObject::Connection connect_clicked(Sender* sender, const Receiver* receiver, Slot slot, Qt::ConnectionType type = Qt::AutoConnection)
{
  return Sender::connect(sender, &Sender::clicked, receiver, slot, type);
}

/**
 * Make a connection and assert if the attempt fails.
 * This template is for the general case, including lambdas.  Unfortunately:
 * "Qt::UniqueConnections do not work for lambdas, non-member functions and functors; they only apply
 * to connecting to member functions."
 * The overload below tries to catch the case where UniqueConnection does apply.
 */
template <typename... Args>
void connect_or_die(Args&&... args)
{
    QMetaObject::Connection retval;

    retval = QObject::connect(std::forward<Args>(args)...);
    Q_ASSERT(static_cast<bool>(retval) != false);
}

/**
 * Make a Qt::UniqueConnection connection and assert if the attempt fails.
 * "Qt::UniqueConnections do not work for lambdas, non-member functions and functors; they only apply
 * to connecting to member functions."
 */
template <class TPMF, class T = ct::class_of_t<TPMF>, class UPMF, class U = ct::class_of_t<UPMF>,
          REQUIRES(std::is_member_function_pointer_v<UPMF>)>
void connect_or_die(const T* t, TPMF tpmf, const U* u, UPMF upmf, Qt::ConnectionType connection_type = Qt::AutoConnection)
{
    QMetaObject::Connection retval;

	// Assert off the bat if we get null ptrs.
	Q_CHECK_PTR(t);
	Q_CHECK_PTR(u);

    retval = QObject::connect(t, tpmf, u, upmf, Qt::ConnectionType(connection_type | Qt::UniqueConnection));
    Q_ASSERT(static_cast<bool>(retval) != false);
}

/**
 * Make a Qt::QueuedConnection | Qt::UniqueConnection connection and assert if the attempt fails.
 * "Qt::UniqueConnections do not work for lambdas, non-member functions and functors; they only apply
 * to connecting to member functions."
 */
template <class TPMF, class T = ct::class_of_t<TPMF>, class UPMF, class U = ct::class_of_t<UPMF>,
          REQUIRES(std::is_member_function_pointer_v<UPMF>)>
void connect_queued_or_die(const T* t, TPMF tpmf, const U* u, UPMF upmf)
{
    QMetaObject::Connection retval;

    retval = QObject::connect(t, tpmf, u, upmf, Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    Q_ASSERT(static_cast<bool>(retval) != false);
}

//inline static
//void connect_or_die(const QObject *sender, const QMetaMethod &signal,
//                    const QObject *receiver, const QMetaMethod &method,
//                    Qt::ConnectionType type = Qt::AutoConnection)
//{
//    QMetaObject::Connection retval;

//    retval = QObject::connect(sender, signal, receiver, method, Qt::ConnectionType(type | Qt::UniqueConnection));
//    Q_ASSERT(static_cast<bool>(retval) != false);
//}

/**
 * Make a blocking signal-slot connection and assert if the attempt fails.
 * Checks if the sender and receiver are in the same thread and makes either a
 * Qt::DirectConnection if same or Qt::BlockingQueuedConnection if different.
 */
template <typename Sender, typename Signal, typename Receiver, typename Slot>
void connect_blocking_or_die(Sender&& sender, Signal&& signal, Receiver&& receiver, Slot&& slot)
{
    QMetaObject::Connection retval;

    auto sthread = sender->thread();
    auto rthread = receiver->thread();
    Qt::ConnectionType connection_type;

    if(sthread == rthread)
    {
    	// Same thread.
    	connection_type = Qt::DirectConnection;
    }
    else
    {
    	// Different threads.
    	connection_type = Qt::BlockingQueuedConnection;
    }
    qDb() << "Connecting" << sender << "and" << receiver << "with connection type:" << connection_type;

    retval = QObject::connect(std::forward<Sender>(sender), std::forward<Signal>(signal),
    		std::forward<Receiver>(receiver), std::forward<Slot>(slot), connection_type);
    Q_ASSERT(static_cast<bool>(retval) != false);
}

/**
 * For connecting the @a sender's destroyed() signal.
 * Qt5 docs re destroyed:
 * "This signal is emitted immediately before the object obj is destroyed, and can not be blocked.
 * All the objects's children are destroyed immediately after this signal is emitted."
 */
template <typename Sender>
QMetaObject::Connection connect_destroyed_debug(Sender* sender, Qt::ConnectionType type = Qt::AutoConnection)
{
    QMetaObject::Connection retval;

    retval = Sender::connect(sender, &Sender::destroyed, /*MainWindow::instance()*/qApp,
                             [=](QObject* the_qobj){
                            qDb() << "RECEIVED DESTROYED SIGNAL. OBJECT:" << the_qobj;
                            ;},
                            type);
    return retval;
}

#endif // CONNECTHELPERS_H
