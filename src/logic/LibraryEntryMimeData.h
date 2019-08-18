/*
 * Copyright 2017,2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
#include <QStringList>

#include <QDataStream>
#include <QIODevice>
#include <QDebug>

#include <memory>
#include <vector>

class LibraryEntry;


class DropTargetInstructions
{
	Q_GADGET
public:
	enum ItemDispositionActionEnumerator
	{
		IDAE_NA,
		IDAE_APPEND,
		IDAE_REPLACE
	};

	Q_ENUM(ItemDispositionActionEnumerator)

	enum PostAddActionEnumerator
	{
		PA_NONE,
		PA_START_PLAYING,
	};
	Q_ENUM(PostAddActionEnumerator)

	/// Which item disposition action to take.
	ItemDispositionActionEnumerator m_action { IDAE_NA };

	/// Whether to start playing or not.
	PostAddActionEnumerator m_start_playing { PA_NONE };
};

class LibraryEntryMimeData : public QMimeData
{
	Q_OBJECT

public:
	LibraryEntryMimeData();

	bool hasFormat(const QString& mimetype) const override;
	QStringList formats() const override;

	/// The LibraryEntry's contained in this MimeData object.
	std::vector<std::shared_ptr<LibraryEntry>> m_lib_item_list;

	/// Instructions to the target on what the user wants it to do with these LibraryEntry's.
	/// Intended for use in a context menu's "Send to and play"/"Append"/"Replace" menu actions.
	DropTargetInstructions m_drop_target_instructions;

private:
	Q_DISABLE_COPY(LibraryEntryMimeData)
             
};

// Not declaring this as a metatype.  Needs a copy constructor, which we have privatized.
// FWIW, QMimeData itself isn't declared as a metatype either.
Q_DECLARE_METATYPE(LibraryEntryMimeData*)

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
