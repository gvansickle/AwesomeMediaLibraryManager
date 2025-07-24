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

#ifndef SRC_UTILS_QTHELPERS_H_
#define SRC_UTILS_QTHELPERS_H_

/// @file

#include <config.h>
#include <QtCore/QSharedPointer>


/**
 * In many/most instances we can't cross the std:: and Qt streams.
 * Uhhhh.......
 */
template <class T>
using SHARED_PTR = QSharedPointer<T>;

template <class T, class... Args>
SHARED_PTR<T> MAKE_SHARED(Args&&... args)
{
	return SHARED_PTR<T>::create(args...);
}

/// Seriously, no Qt unique ptr?
template <class T>
using UNIQUE_PTR = QSharedPointer<T>;

template <class T>
using WEAK_PTR = QWeakPointer<T>;


/**
 * Use the below macros in your header, then put something like this in your .cpp file:
 *
 * #define DATASTREAM_FIELDS(X) \
    X(m_found_url) X(m_found_url_modinfo) X(m_dir_props) X(m_cue_url) X(m_cue_url_modinfo)

    QDebug operator<<(QDebug dbg, const DirScanResult & obj)
    {
        QDebugStateSaver saver(debug);
    #define X(field) << obj.field
        dbg DATASTREAM_FIELDS(X);
    #undef X
        return dbg;
    }

    QDataStream &operator<<(QDataStream &out, const DirScanResult & myObj)
    {
    #define X(field) << myObj.field
        out DATASTREAM_FIELDS(X);
    #undef X
        return out;
    }

    QDataStream &operator>>(QDataStream &in, DirScanResult & myObj)
    {
    #define X(field) >> myObj.field
        return in DATASTREAM_FIELDS(X);
    #undef X
    }
 */

#define IMPL_QTH_DECLARE_QDEBUG_OP(friend_placeholder, classname) \
	friend_placeholder QDebug operator<<(QDebug out, const classname & obj)

//#define IMPL_QTH_DECLARE_QDATASTREAM_OPS(friend_placeholder, classname) \
//    friend_placeholder QDataStream &operator<<(QDataStream &out, const classname & myObj); \
//    friend_placeholder QDataStream &operator>>(QDataStream &in, classname & myObj);

//template <class StreamTypePlusRef, class TheClass>


#define IMPL_QTH_DECLARE_STREAM_OPS(friend_placeholder, outstream_class_plus_ref, instream_class_plus_ref, classname) \
	friend_placeholder outstream_class_plus_ref operator<<(outstream_class_plus_ref out, const classname & myObj); \
    friend_placeholder instream_class_plus_ref operator>>(instream_class_plus_ref in, classname & myObj)

#define IMPL_QTH_DECLARE_QDATASTREAM_OPS(friend_placeholder, classname) \
	IMPL_QTH_DECLARE_STREAM_OPS(friend_placeholder, QDataStream&, QDataStream&, classname)

#define IMPL_QTH_DECLARE_QXMLSTREAM_OPS(friend_placeholder, classname) \
	IMPL_QTH_DECLARE_STREAM_OPS(friend_placeholder, QXmlStreamWriter&, QXmlStreamReader&, classname)

/**
 * QDebug "friender".
 */
#define QTH_DECLARE_FRIEND_QDEBUG_OP(classname) \
    IMPL_QTH_DECLARE_QDEBUG_OP(friend, classname)

/**
 * QDebug output stream operator helper macro.
 */
#define QTH_DECLARE_QDEBUG_OP(classname) \
	IMPL_QTH_DECLARE_QDEBUG_OP(/**/, classname)

/**
 * A dummy debug streaming operator, intended for temp use while developing.
 */
#define QTH_DEFINE_DUMMY_QDEBUG_OP(classname) \
    QDebug operator<<(QDebug dbg, const classname & obj)\
    {\
        QDebugStateSaver saver(dbg);\
        dbg << #classname " (" << "TODO" << ")";\
        return dbg; \
    }

#define QTH_DEFINE_QDEBUG_OP(classname, ...) \
    QDebug operator<<(QDebug dbg, const classname & obj)\
    {\
        QDebugStateSaver saver(dbg);\
        dbg << #classname " (" __VA_ARGS__ << ")";\
        return dbg; \
    }

#define QTH_FRIEND_QDATASTREAM_OPS(classname) \
    IMPL_QTH_DECLARE_QDATASTREAM_OPS(friend, classname)

#define QTH_DECLARE_QDATASTREAM_OPS(classname) \
	IMPL_QTH_DECLARE_QDATASTREAM_OPS(/**/, classname)

/**
 * Call this in your QObject-derived class' constructor to give each object a uniqueified name.
 * @param the_object
 */
#define setNumberedObjectName(the_object) \
    do { static int id = 0; setNumberedObjectNameInternal(the_object, id); } while(0)

void setNumberedObjectNameInternal(QObject* the_object, int& id);

#endif /* SRC_UTILS_QTHELPERS_H_ */
