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

#include "Library.h"

#include <QDateTime>
#include <QDebug>
#include <QFileDevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

Library::Library()
{

}

Library::~Library()
{

}

void Library::clear()
{
	rootURL = QUrl();
	lib_entries.clear();
	num_unpopulated = 0;
	num_populated = 0;
	///discovered_metadata_keys = []
}

QString Library::getLibraryName() const
{
	if(!rootURL.isValid())
	{
		return "Unknown";
	}
	else
	{
		QString last_dir = rootURL.toString(QUrl::StripTrailingSlash);
		QStringList last_dir_list = last_dir.split('/');
		return last_dir_list.back();
	}
}

void Library::addNewEntries(std::vector<LibraryEntry*> entries)
{
	for(auto e : entries)
	{
		lib_entries.push_back(e);
		addingEntry(e);
	}
}

void Library::removeEntry(int row)
{
	auto old_entry = lib_entries[row];
	removingEntry(old_entry);
	auto it = lib_entries.begin();
	std::advance(it, row);
	delete lib_entries[row];
	lib_entries.erase(it);
}

void Library::insertEntry(int row, LibraryEntry* entry)
{
	addingEntry(entry);
	auto it = lib_entries.begin();
	std::advance(it, row);
	lib_entries.insert(it, entry);
}

void Library::replaceEntry(int row, LibraryEntry* entry)
{
	qDebug() << "Replacing row" << row << ", old/new:" << lib_entries[row] << "/" << entry;
	qDebug() << "isPopulated: old/new:" << lib_entries[row]->isPopulated() << "/" << entry->isPopulated();
	auto old_entry = lib_entries[row];
	addingEntry(entry);
	lib_entries[row] = entry;
	removingEntry(old_entry);
	delete old_entry;
}

bool Library::areAllEntriesFullyPopulated() const
{
	for(auto e : lib_entries)
	{
		if(!e->isPopulated())
		{
			qDebug() << "NOT POPULATED: " << e->getUrl();
			return false;
		}
	}

	return true;
}

qint64 Library::getNumEntries() const
{
	return num_populated + num_unpopulated;
}

qint64 Library::getNumPopulatedEntries() const
{
	return num_populated;
}

void Library::writeToJson(QJsonObject& jo, bool no_items) const
{
	// Write the Library's info.
	jo["write_timestamp_ms"] = QDateTime::currentMSecsSinceEpoch();
	jo["write_timestamp_utc"] = QDateTime::currentDateTimeUtc().toString();
	jo["rootUrl"] = rootURL.toString();
	jo["num_unpopulated"] = num_unpopulated;
	jo["num_populated"] = num_populated;
	jo["len_lib_entries"] = (qint64)lib_entries.size();
	if(!no_items)
	{
		// Collect up the LibraryEntry's.
		QJsonArray array;
		for(auto e : lib_entries)
		{
			QJsonObject qjsonobject;
			e->writeToJson(qjsonobject);
			array.append(qjsonobject);
		}
		//print(array)
		jo["LibraryEntries"] = array;
	}
}

void Library::readFromJson(const QJsonObject& jo)
{
	clear();
	rootURL = QUrl(jo["rootUrl"].toString());
	num_unpopulated = jo["num_unpopulated"].toInt();
	num_populated = jo["num_populated"].toInt();
	int len_lib_entries = jo["len_lib_entries"].toInt();
	// Read in the library entries.
	QJsonArray jsonarray = jo["LibraryEntries"].toArray();
	int len_jsonarray = jsonarray.size();
	if(len_jsonarray != len_lib_entries)
	{
		qCritical() << QString("len_jsonarray(%1) != len_lib_entries(%2)").arg(len_jsonarray).arg(len_lib_entries);
	}
	if(jsonarray.size() > 0)
	{
		for(qint64 i = 0; i<jsonarray.size(); ++i)
		{
			///qDebug() << "Reading LibraryEntry:" << i;
			auto libentryobj = jsonarray[i].toObject();
			auto libentry = new LibraryEntry();
			libentry->readFromJson(libentryobj);
			lib_entries.push_back(libentry);
		}
        ptrdiff_t read_num_lib_entries = lib_entries.size();
		if(read_num_lib_entries != len_lib_entries)
		{
			qCritical() << QString("read_num_lib_entries(%1) != len_lib_entries(%2)").arg(read_num_lib_entries).arg(len_lib_entries);
		}
	}
}

void Library::serializeToFile(QFileDevice& file) const
{
	QJsonObject lib_object;
	writeToJson(lib_object);
	QJsonDocument saveDoc(lib_object);
	file.write(saveDoc.toJson());
}

void Library::deserializeFromFile(QFileDevice& file)
{
	Q_ASSERT(0 == "NOT IMPLEMENTED");
}

void Library::addingEntry(LibraryEntry* entry)
{
	if(entry->isPopulated())
	{
		num_populated++;
	}
	else
	{
		num_unpopulated++;
	}
}

void Library::removingEntry(LibraryEntry* entry)
{
	if(entry->isPopulated())
	{
		num_populated--;
	}
	else
	{
		num_unpopulated--;
	}
}
