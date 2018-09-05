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

#include <QMimeDatabase>
#include <QMimeType>

/**
 * A streamable QMimeType.
 */
class ExtMimeType : public QMimeType
{
    Q_GADGET

public:
    ExtMimeType() = default;
    ExtMimeType(const QMimeType& other) : QMimeType(other) {}
    ~ExtMimeType() = default;

    template <class StreamType>
    friend StreamType& operator<<(StreamType& out, const ExtMimeType& obj)
    {
        out << obj.name();
        return out;
    }

    template <class StreamType>
    friend StreamType& operator>>(StreamType& in, ExtMimeType& obj)
    {
        QString as_str;
        in >> as_str;

        QMimeDatabase db;
        obj = db.mimeTypeForName(as_str);

        return in;
    }
};

#endif // EXTMIMETYPE_H
