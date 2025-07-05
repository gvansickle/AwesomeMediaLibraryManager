/*
 * Copyright 2017, 2018, 2019, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/// @file ConnectHelpers.h

// Std C++
#include <vector>

// Std C++ backfill
#include <future/cpp14_concepts.hpp>

// Qt
#include <QAction>
#include <QApplication>
#include <QMetaObject>

// Ours
#include "DebugHelpers.h"

/**
 * Helper template to extract the class type from a pointer-to-member or pointer-to-member-function.
 */
template <typename T>
using class_of_t = typename std::remove_reference<decltype(*std::declval<T>())>::type;

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
 * This template is for lambdas.  Unfortunately:
 * "Qt::UniqueConnections do not work for lambdas, non-member functions and functors; they only apply
 * to connecting to member functions."
 * The overload below applies in the case where UniqueConnection does apply.
 */
template <typename TPMF, typename T=class_of_t<TPMF>*, typename U,  typename Lambda, typename... Args>
	requires (!std::is_member_function_pointer_v<Lambda>)
QMetaObject::Connection connect_or_die(const T t, TPMF tpmf, const U u, Lambda l, Args&&... args)
{
    QMetaObject::Connection retval;

    retval = QObject::connect(t, tpmf, u, l, std::forward<Args>(args)...);
    Q_ASSERT(static_cast<bool>(retval) != false);
	return retval;
}

/**
 * Make a Qt::UniqueConnection connection and assert if the attempt fails.
 * "Qt::UniqueConnections do not work for lambdas, non-member functions and functors; they only apply
 * to connecting to member functions."
 */
template <class TPMF, class T = class_of_t<TPMF>*, class UPMF, class U = class_of_t<UPMF>*>
	requires std::is_member_function_pointer_v<TPMF> && std::is_member_function_pointer_v<UPMF>
QMetaObject::Connection
connect_or_die(const T t, TPMF tpmf, const U u, UPMF upmf, Qt::ConnectionType connection_type = Qt::AutoConnection)
{
    QMetaObject::Connection retval;

	// Assert off the bat if we get null ptrs.
	// Q_CHECK_PTR(t);
	// Q_CHECK_PTR(u);

    retval = QObject::connect(t, tpmf, u, upmf, Qt::ConnectionType(connection_type | Qt::UniqueConnection));
    Q_ASSERT(static_cast<bool>(retval) != false);
	return retval;
}

/**
 * Make a Qt::QueuedConnection | Qt::UniqueConnection connection and assert if the attempt fails.
 * "Qt::UniqueConnections do not work for lambdas, non-member functions and functors; they only apply
 * to connecting to member functions."
 */
template <class TPMF, class T = class_of_t<TPMF>, class UPMF, class U = class_of_t<UPMF>,
          REQUIRES(std::is_member_function_pointer_v<UPMF>)>
QMetaObject::Connection connect_queued_or_die(const T* t, TPMF tpmf, const U* u, UPMF upmf)
{
    QMetaObject::Connection retval;

    retval = QObject::connect(t, tpmf, u, upmf, Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    Q_ASSERT(static_cast<bool>(retval) != false);
	return retval;
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
QMetaObject::Connection connect_blocking_or_die(Sender&& sender, Signal&& signal, Receiver&& receiver, Slot&& slot)
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
	return retval;
}

/**
 * For connecting the @a sender's destroyed() signal.
 * Qt docs re destroyed:
 * "This signal is emitted immediately before the object @a obj is destroyed, after any instances of QPointer have been
 * notified, and cannot be blocked. All the objects' children are destroyed immediately after this signal is emitted."
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
 * Holds any number of connections for the purpose of disconnecting them at some later time.
 */
class Disconnector
{
public:
	explicit Disconnector();
	~Disconnector();

	void addConnection(QMetaObject::Connection connection);
	Disconnector& operator<<(QMetaObject::Connection connection);

	/**
	 * Disconnects and deletes all connections which have been registered.
	 */
	void disconnect();

private:
	std::vector<QMetaObject::Connection> m_connections;
};




#endif // CONNECTHELPERS_H
