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

#ifndef EXTFUTUREPROGRESSINFO_H
#define EXTFUTUREPROGRESSINFO_H

#include <config.h>

#include <QObject>
#include <QString>

/**
 * Class which tries to serialize KJob-like progress info into a QString while still maintaining
 * QFuture's progressText() functionality.
 * @note This will not end well.
 */
class ExtFutureProgressInfo : public QString
{
	/// @note QString derives from nothing, isn't a Q_OBJECT, and does not have any virtual members.

public:

	ExtFutureProgressInfo() : QString() {};
	~ExtFutureProgressInfo() = default;

	QString toQString() const;
	bool fromQString(const QString& str);

};
Q_DECLARE_METATYPE(ExtFutureProgressInfo);

#endif // EXTFUTUREPROGRESSINFO_H
