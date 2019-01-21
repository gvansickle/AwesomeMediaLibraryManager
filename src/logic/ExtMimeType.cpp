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

#include "ExtMimeType.h"

// Ours.
#include <utils/DebugHelpers.h>
#include <utils/RegisterQtMetatypes.h>


AMLM_QREG_CALLBACK([](){
	qIn() << "Registering ExtMimeType";
	qRegisterMetaType<ExtMimeType>();
	qRegisterMetaTypeStreamOperators<ExtMimeType>();
	QMetaType::registerDebugStreamOperator<ExtMimeType>();
	QMetaType::registerConverter<ExtMimeType, QString>([](const ExtMimeType& obj){ return obj.name(); });
});


ExtMimeType::ExtMimeType() : QMimeType()
{
//static int* dummy = (qRegisterMetaTypeStreamOperators<ExtMimeType>(), nullptr);
}

QDebug operator<<(QDebug dbg, const ExtMimeType& obj) // NOLINT(performance-unnecessary-value-param)
{
	dbg << "ExtMimeType(" << obj.name() << ")";
	return dbg;
}

QDataStream& operator<<(QDataStream& out, const ExtMimeType& obj)
{
	out << obj.name();
	return out;
}

QDataStream& operator>>(QDataStream& in, ExtMimeType& obj)
{
	QString as_str;
	in >> as_str;

	QMimeDatabase db;
	obj = db.mimeTypeForName(as_str);

	return in;
}
