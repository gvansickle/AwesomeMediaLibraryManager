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
 * @file
 * Helpers which attempt to make the smart pointer interoperability issues with Qt's
 * various smart pointer classes (even/especially between themselves) more tractable.
 */

#ifndef SRC_UTILS_QTCASTHELPERS_H_
#define SRC_UTILS_QTCASTHELPERS_H_

#include <type_traits>

class QObject;
#include <QPointer>
#include <QSharedPointer>

#include "DebugHelpers.h"

/**
 * Dynamically cast a QSharedPointer<T> to a QPointer<DerivedFromT> (which under the hood is really a QWeakPointer<>).
 */
template <typename T, typename Base>
QPointer<T> qSharedPtrToQPointerDynamicCast(QSharedPointer<Base> ptr_in)
{
	// This unfortunate dance is needed to get a QPointer (which is really a QWeakPointer) to a dynamically-casted
	// derived class, while not losing/screwing up the ref counts.  Hopefully.

    // Make sure what we're trying to do is even possible.
    Q_ASSERT_X(dynamic_cast<T*>(ptr_in.data()) != 0, __PRETTY_FUNCTION__, "pointer is not dynamic_cast<> convertable");

	// Get a QSharedPointer<T> from the QSharedPoiner<Base>.
    QSharedPointer<T> ptr_out = qSharedPointerDynamicCast<T>(ptr_in);

    QPointer<T> retval = ptr_out.data();

	return retval;
}

template <typename T, typename Base>
T* qObjectCast(Base* ptr_in)
{
    static_assert(std::is_base_of<QObject, Base>::value == true, "ptr_in is not derived from QObject");
    static_assert(std::is_base_of<QObject, T>::value == true, "desired return type is not derived from QObject");
//    static_assert(std::is_convertible<Base*, T*>::value == true, "ptr_in is not convertible to desired return type");

    Q_ASSERT(ptr_in != nullptr);

    T* retval = qobject_cast<T*>(ptr_in);

    Q_ASSERT(retval != 0);

    return retval;
}

#endif /* SRC_UTILS_QTCASTHELPERS_H_ */
