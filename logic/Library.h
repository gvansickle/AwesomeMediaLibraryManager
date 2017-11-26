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

#ifndef LIBRARY_H
#define LIBRARY_H

#include <QUrl>

#include <deque>

#include "LibraryEntry.h"

class QFileDevice;

class Library
{
public:
    Library();
	virtual ~Library();

	void clear();
	void setRootUrl(const QUrl& url) { rootURL = url; }
	QUrl getRootUrl() { return rootURL; }
	QString getLibraryName() const;

	void addNewEntries(std::vector<LibraryEntry*> entries);
	void removeEntry(int row);
	void insertEntry(int row, LibraryEntry* entry);
	void replaceEntry(int row, LibraryEntry* entry);

	bool areAllEntriesFullyPopulated() const;
	qint64 getNumEntries() const;
	qint64 getNumPopulatedEntries() const;

	/// @name Serialization
	/// @{
	void writeToJson(QJsonObject& jo, bool no_items = false) const;
	void readFromJson(const QJsonObject& jo);

	void serializeToFile(QFileDevice& file) const;
	void deserializeFromFile(QFileDevice& file);
	/// @}

private:
	void addingEntry(LibraryEntry* entry);
	void removingEntry(LibraryEntry* entry);

//private:
public:
    QUrl rootURL;

	std::vector<LibraryEntry*> lib_entries;

	qint64 num_unpopulated {0};
	qint64 num_populated {0};

};

#endif // LIBRARY_H
