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

#include <utils/DebugHelpers.h>

Library::Library()
{

}

Library::~Library()
{

}

void Library::clear()
{
	m_root_url = QUrl();
	m_lib_entries.clear();
	m_num_unpopulated = 0;
	m_num_populated = 0;
	///discovered_metadata_keys = []
}

QString Library::getLibraryName() const
{
	if(!m_root_url.isValid())
	{
		return "Unknown";
	}
	else
	{
		QString last_dir = m_root_url.toString(QUrl::StripTrailingSlash);
		QStringList last_dir_list = last_dir.split('/');
		return last_dir_list.back();
	}
}

void Library::addNewEntries(std::vector<std::shared_ptr<LibraryEntry>> entries)
{
	for(auto& e : entries)
	{
		m_lib_entries.push_back(e);
		addingEntry(e.get());
	}
}

void Library::removeRow(int row)
{
	removeRows(row, 1);
}

void Library::removeRows(int row, int count)
{
	// Do the accounting.
	for(auto i = row; i<=(row+count-1); ++i)
	{
		auto old_entry = m_lib_entries[row];
		removingEntry(old_entry.get());
	}

	// Get an iterator to the first row we're going to erase.
	auto first = m_lib_entries.begin();
	std::advance(first, row);
	// ..and one to the last-plus-one row.
	auto last_plus_one = first;
	std::advance(last_plus_one, count);

	// erase the entries.
	m_lib_entries.erase(first, last_plus_one);
}

void Library::insertEntry(int row, std::shared_ptr<LibraryEntry> entry)
{
	addingEntry(entry.get());
	auto it = m_lib_entries.begin();
	std::advance(it, row);
	m_lib_entries.insert(it, entry);
}

void Library::replaceEntry(int row, std::shared_ptr<LibraryEntry> entry)
{
	//qDebug() << "Replacing row" << row << ", old/new:" << m_lib_entries[row] << "/" << entry;
//	qDebug() << "isPopulated: old/new:" << m_lib_entries[row]->isPopulated() << "/" << entry->isPopulated();
	auto old_entry = m_lib_entries[row];
	addingEntry(entry.get());
	m_lib_entries[row] = entry;
	removingEntry(old_entry.get());
	//delete old_entry;
}

std::shared_ptr<LibraryEntry> Library::operator[](size_t index) const
{
	return m_lib_entries.at(index);
}


bool Library::areAllEntriesFullyPopulated() const
{
	for(const auto& e : m_lib_entries)
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
	return m_num_populated + m_num_unpopulated;
}

qint64 Library::getNumPopulatedEntries() const
{
	return m_num_populated;
}

void Library::writeToJson(QJsonObject& jo, bool no_items) const
{
	// Write the Library's info.
	jo["write_timestamp_ms"] = QDateTime::currentMSecsSinceEpoch();
	jo["write_timestamp_utc"] = QDateTime::currentDateTimeUtc().toString();
	jo["rootUrl"] = m_root_url.toString();
	jo["m_num_unpopulated"] = m_num_unpopulated;
	jo["m_num_populated"] = m_num_populated;
	jo["len_lib_entries"] = (qint64)m_lib_entries.size();
	if(!no_items)
	{
		// Collect up the LibraryEntry's.
		QJsonArray array;
		for(const auto& e : m_lib_entries)
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
	m_root_url = QUrl(jo["rootUrl"].toString());
	m_num_unpopulated = jo["m_num_unpopulated"].toInt();
	m_num_populated = jo["m_num_populated"].toInt();
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
			auto libentry = std::make_shared<LibraryEntry>();
			libentry->readFromJson(libentryobj);
			m_lib_entries.push_back(libentry);
		}
		ptrdiff_t read_num_lib_entries = m_lib_entries.size();
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
	Q_ASSERT(nullptr == "NOT IMPLEMENTED");
}

/// @aside ...uhhhhhhhhh........
using strviw_type = QString;
static const strviw_type XMLTAG_WRITE_TIMESTAMP_MS("write_timestamp_ms");
static const strviw_type XMLTAG_WRITE_TIMESTAMP_UTC("write_timestamp_utc");
static const strviw_type XMLTAG_LIBRARY_ROOT_URL("library_root_url");
static const strviw_type XMLTAG_NUM_UNPOP("m_num_unpopulated");
static const strviw_type XMLTAG_NUM_POP("m_num_populated");
static const strviw_type XMLTAG_NUM_LIBRARY_ENTRIES("num_lib_entries");
static const strviw_type XMLTAG_LIBRARY_ENTRIES("library_entries");
//static const std::string_view XMLTAG_LIBRARY("library");

QVariant Library::toVariant() const
{
	QVariantInsertionOrderedMap map;

	// Write the Library's info.
	map.insert(XMLTAG_WRITE_TIMESTAMP_MS, QDateTime::currentMSecsSinceEpoch());
	map.insert(XMLTAG_WRITE_TIMESTAMP_UTC, QDateTime::currentDateTimeUtc()/*.toString()*/);
	map.insert(XMLTAG_LIBRARY_ROOT_URL, m_root_url);
	map.insert(XMLTAG_NUM_UNPOP, m_num_unpopulated);
	map.insert(XMLTAG_NUM_POP, m_num_populated);
	map.insert(XMLTAG_NUM_LIBRARY_ENTRIES, (qint64)m_lib_entries.size());
	if(!m_lib_entries.empty())
	{
		// Serialize the LibraryEntry's.
		QVariantList list;
		for(const auto& e : m_lib_entries)
		{
			list.append(e->toVariant());
		}
		map.insert(XMLTAG_LIBRARY_ENTRIES, list);
	}

	return QVariant::fromValue(map);
}

void Library::fromVariant(const QVariant& variant)
{
	QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();

	m_root_url = map.value(XMLTAG_LIBRARY_ROOT_URL).value<QUrl>();
	m_num_unpopulated = map.value(XMLTAG_NUM_UNPOP).value<qint64>();
	m_num_populated = map.value(XMLTAG_NUM_POP).value<qint64>();
	qint64 num_lib_entries = map.value(XMLTAG_NUM_LIBRARY_ENTRIES).value<qint64>();

	QVariantList list = map.value(XMLTAG_LIBRARY_ENTRIES).toList();
	for(const QVariant& e : list)
	{
		std::shared_ptr<LibraryEntry> libentry = std::make_shared<LibraryEntry>();
		libentry->fromVariant(e);
		m_lib_entries.push_back(libentry);
	}
	Q_ASSERT(m_lib_entries.size() == num_lib_entries);
}

void Library::addingEntry(const LibraryEntry* entry)
{
	if(entry->isPopulated())
	{
		m_num_populated++;
	}
	else
	{
		m_num_unpopulated++;
	}
}

void Library::removingEntry(const LibraryEntry* entry)
{
	if(entry->isPopulated())
	{
		m_num_populated--;
	}
	else
	{
		m_num_unpopulated--;
	}
}
