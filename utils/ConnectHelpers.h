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

#endif // CONNECTHELPERS_H
