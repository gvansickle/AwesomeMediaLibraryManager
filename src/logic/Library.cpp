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

// Qt5
#include <QDateTime>
#include <QDebug>
#include <QFileDevice>
#include <QtConcurrent>

// Ours
#include <utils/DebugHelpers.h>
#include <future/preproc.h>
#include <utils/Stopwatch.h>
#include <logic/serialization/SerializationHelpers.h>

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


#define M_DATASTREAM_FIELDS(X) \
	X(XMLTAG_ROOT_URL, m_root_url) \
	X(XMLTAG_NUM_UNPOP, m_num_unpopulated) \
	X(XMLTAG_NUM_POP, m_num_populated)

#define M_DATASTREAM_FIELDS_SPECIAL_HANDLING(X) \
	X(XMLTAG_LIBRARY_ENTRIES, m_lib_entries)


/// Strings to use for the tags.
using strviw_type = QLatin1String;

#define X(field_tag, member_field) static const strviw_type field_tag( # member_field );
	M_DATASTREAM_FIELDS(X);
	M_DATASTREAM_FIELDS_SPECIAL_HANDLING(X);
#undef X
static const strviw_type XMLTAG_WRITE_TIMESTAMP_MS("write_timestamp_ms");
static const strviw_type XMLTAG_WRITE_TIMESTAMP_UTC("write_timestamp_utc");
static const strviw_type XMLTAG_NUM_LIBRARY_ENTRIES("num_lib_entries");


QVariant Library::toVariant() const
{
	InsertionOrderedMap<QString, QVariant> map;

#define X(field_tag, member_field)   map_insert_or_die(map, field_tag, member_field);
	M_DATASTREAM_FIELDS(X);
#undef X

	// Write some derived info re: the Library.
	map_insert_or_die(map, XMLTAG_WRITE_TIMESTAMP_MS, QDateTime::currentMSecsSinceEpoch());
	map_insert_or_die(map, XMLTAG_WRITE_TIMESTAMP_UTC, QDateTime::currentDateTimeUtc()/*.toString()*/);
	map_insert_or_die(map, XMLTAG_NUM_LIBRARY_ENTRIES, static_cast<qint64>(m_lib_entries.size()));
	if(!m_lib_entries.empty())
	{
		// Serialize the LibraryEntry's into an ordered list.
		QVariantHomogenousList list("m_lib_entries", "library_entry");

		// ... in parallel, at least somewhat.
		list_blocking_map_reduce_push_back_or_die(list, m_lib_entries);

		map_insert_or_die(map, XMLTAG_LIBRARY_ENTRIES, list);
	}

qDb() << "EXIT, wrote:" << m_lib_entries.size() << "libentries";

	return map;
}

void Library::fromVariant(const QVariant& variant)
{
	Stopwatch sw("################### Library::fromVariant()");

	InsertionOrderedMap<QString, QVariant> map; // = variant.value<QVariantInsertionOrderedMap>();
	qviomap_from_qvar_or_die(&map, variant);

#define X(field_tag, member_field)   map_read_field_or_warn(map, field_tag, &member_field);
	M_DATASTREAM_FIELDS(X);
#undef X

	// This is a local.
	qint64 num_lib_entries = map.value(XMLTAG_NUM_LIBRARY_ENTRIES).value<qint64>();

	//QVariantHomogenousList list("library_entries", "library_entry");
	//map_read_field_or_warn(map, XMLTAG_LIBRARY_ENTRIES, &list);
	QVariant qvar_list = map.value(XMLTAG_LIBRARY_ENTRIES);
	Q_ASSERT(qvar_list.isValid());
	QVariantHomogenousList list("m_lib_entries", "library_entry");
	list = qvar_list.value<QVariantHomogenousList>();

	// Concurrency.  Vs. the loop we used to have here, we went from 2.x secs to 0.5 secs.
	list_blocking_map_reduce_read_all_entries_or_warn(list, &m_lib_entries);

	qDb() << "NUM LIB ENTRIES:" << m_lib_entries.size() << "VS:" << num_lib_entries;
	AMLM_WARNIF(m_lib_entries.size() != num_lib_entries);
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
