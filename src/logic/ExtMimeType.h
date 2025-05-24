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

#ifndef EXTMIMETYPE_H
#define EXTMIMETYPE_H

/**
 * @file ExtMimeType.h
 */

// Qt
#include <QObject>
#include <QMimeDatabase>
#include <QMimeType>

// Ours.
#include <utils/QtHelpers.h>

/**
 * A streamable QMimeType.
 *
 * @note QMimeType does not have a virtual destructor, so maybe we really should be doing this differently.
 */
class ExtMimeType : public QMimeType
{
    Q_GADGET

public:
    ExtMimeType();
    ExtMimeType(const QMimeType& other) : QMimeType(other) {}
    ~ExtMimeType() = default;

	QTH_FRIEND_QDATASTREAM_OPS(ExtMimeType);

//    template <class StreamType>
//    friend StreamType& operator<<(StreamType& out, const ExtMimeType& obj)
//    {
//        out << obj.name();
//        return out;
//    }

//    template <class StreamType>
//    friend StreamType& operator>>(StreamType& in, ExtMimeType& obj)
//    {
//        QString as_str;
//        in >> as_str;

//        QMimeDatabase db;
//        obj = db.mimeTypeForName(as_str);

//        return in;
//    }
};

Q_DECLARE_METATYPE(ExtMimeType);

QTH_DECLARE_QDEBUG_OP(ExtMimeType);
QTH_DECLARE_QDATASTREAM_OPS(ExtMimeType);


#endif // EXTMIMETYPE_H
