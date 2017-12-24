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

#ifndef LIBRARYENTRYMIMEDATA_H
#define LIBRARYENTRYMIMEDATA_H

#include <QMimeData>

#include <QDataStream>
#include <QIODevice>
#include <QDebug>

#include <memory>
#include <vector>

class LibraryEntry;

class LibraryEntryMimeData : public QMimeData
{
	Q_OBJECT

public:
	LibraryEntryMimeData();

	std::vector<std::shared_ptr<LibraryEntry>> lib_item_list;

private:
	Q_DISABLE_COPY(LibraryEntryMimeData)
};

class MimeDataDumper : public QObject
{
	Q_OBJECT

public:
	MimeDataDumper(QObject* parent) : QObject(parent) {}

	void dumpMimeData(const QMimeData* md)
	{
		QByteArray e = md->data("application/x-qabstractitemmodeldatalist");
		QDataStream stream(&e, QIODevice::ReadOnly);

		while(!stream.atEnd())
		{
			int row, col;
			QMap<int, QVariant> roleDataMap;

			stream >> row >> col >> roleDataMap;

			qDebug() << "row:" << row << "col:" << col << "roleDataMap:" << roleDataMap;
		}
	}
};

#endif // LIBRARYENTRYMIMEDATA_H
