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

#include <config.h>

/*
 *
 */
class QtHelpers
{
public:
	QtHelpers();
	virtual ~QtHelpers();
};

//template<class StreamType, class T>
//StreamType& operator<<(StreamType& outstream, const T& obj)
//{
//    return outstream << obj;
//}

#define IMPL_QTH_DECLARE_QDEBUG_OP(friend_placeholder, classname) \
	friend_placeholder QDebug operator<<(QDebug out, const classname & obj);

#define IMPL_QTH_DECLARE_QDATASTREAM_OPS(friend_placeholder, classname) \
	friend_placeholder QDataStream &operator<<(QDataStream &out, const classname & myObj); \
	friend_placeholder QDataStream &operator>>(QDataStream &in, classname & myObj);

///

#define QTH_DECLARE_QDEBUG_OP(classname) \
	IMPL_QTH_DECLARE_QDEBUG_OP(/**/, classname)

#define QTH_FRIEND_QDEBUG_OP(classname) \
	IMPL_QTH_DECLARE_QDEBUG_OP(friend, classname)

#define QTH_DECLARE_QDATASTREAM_OPS(classname) \
	IMPL_QTH_DECLARE_QDATASTREAM_OPS(/**/, classname)

#define QTH_FRIEND_QDATASTREAM_OPS(classname) \
	IMPL_QTH_DECLARE_QDATASTREAM_OPS(friend, classname)

#endif /* SRC_UTILS_QTHELPERS_H_ */
