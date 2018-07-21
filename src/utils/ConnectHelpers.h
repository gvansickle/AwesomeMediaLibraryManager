/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include <QAction>
#include <QApplication>
#include <QMetaObject>

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

//template <typename T, typename Signal, typename Slot>
//QMetaObject::Connection connect(T* sender_and_receiver, Signal signal, Slot slot, Qt::ConnectionType type = Qt::AutoConnection)
//{
//	return connect(sender_and_receiver, signal, sender_and_receiver, slot, type);
//}

/**
 * Make a connection and assert if the attempt fails.
 * This is the general case, including lambdas.  Unfortunately:
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
template <class T, class U, class TPMF, class UPMF, class ConnectionType>
void connect_or_die(T* t, TPMF tpmf, U* u, UPMF upmf, ConnectionType connection_type = Qt::AutoConnection)
{
    QMetaObject::Connection retval;

    retval = QObject::connect(t, tpmf, u, upmf, connection_type | Qt::UniqueConnection);
    Q_ASSERT(static_cast<bool>(retval) != false);
}

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

/**
 * If @a ptr is not nullptr, runs lambda @a l, passing it ptr as a param.
 * Else @l is never called.
 *
 * @todo Not so much a connect helper, but pretty general-purpose.  Maybe move.
 *
 * @param ptr
 * @param l
 */
template <typename PointerType, typename Lambda>
void with_ptr_or_skip(PointerType ptr, Lambda l)
{
    if(ptr)
    {
        l(ptr);
    }
}

#endif // CONNECTHELPERS_H
