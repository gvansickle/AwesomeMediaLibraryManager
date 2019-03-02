/*
 * Copyright 2017, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

// Object header
#include "LibraryEntry.h"

// Std C++
#include <string>

// Qt5
#include <QDataStream>
#include <QUrlQuery>

// TagLib
#include <tag.h>
#include <fileref.h>
#include <tpropertymap.h>

// Ours
#include "AMLMApp.h"
#include <utils/StringHelpers.h>
#include <utils/QtHelpers.h>
#include <utils/RegisterQtMetatypes.h>
#include <logic/models/ScanResultsTreeModelXMLTags.h>
#include <utils/Fraction.h>
#include <src/future/InsertionOrderedMap.h>
#include "utils/MapConverter.h"
#include "utils/DebugHelpers.h"
#include "TrackMetadata.h"
#include "ntp.h"

#define LIBRARY_ENTRY_MAGIC_NUMBER 0x98542123
#define LIBRARY_ENTRY_VERSION 0x01

AMLM_QREG_CALLBACK([](){
    qIn() << "Registering LibraryEntry metatypes";
    qRegisterMetaType<LibraryEntry>();
    });


LibraryEntry::LibraryEntry(const QUrl &url)
{
	this->m_url = url;
}

std::shared_ptr<LibraryEntry> LibraryEntry::fromUrl(const QUrl &fileurl)
{
    std::shared_ptr<LibraryEntry> retval = std::make_shared<LibraryEntry>(fileurl);

	return retval;
}

std::vector<std::shared_ptr<LibraryEntry>> LibraryEntry::populate(bool force_refresh)
{
    // Populate the metadata.  Assumption is that all we have before calling this is the url.
    // returns a list of LibraryEntry's, or this if m_url was not a multi-track file.

	std::vector<std::shared_ptr<LibraryEntry>> retval;

	// Some sanity checks first.
	if(!m_url.isValid())
	{
        qWr() << "Invalid URL:" << m_url;
		return retval;
	}
    if(!force_refresh && isPopulated() )
	{
		// Nothing to do.
		qDebug() << "Already populated.";
		return retval;
	}

    // Get the MIME type.
    auto mdb = amlmApp->mime_db();
    m_mime_type = mdb->mimeTypeForUrl(m_url);

	// Try to read the metadata of the file.
	Metadata file_metadata = Metadata::make_metadata(m_url);
	if(!file_metadata)
	{
		// Read failed.
		qWarning() << "Can't get metadata for file '" << m_url << "'";
		auto new_entry = std::make_shared<LibraryEntry>(*this);
		new_entry->m_is_populated = true;
		new_entry->m_is_error = true;
		retval.push_back(new_entry);
	}
	else
	{
		if(!file_metadata.hasCueSheet())
		{
			// Couldn't load a cue sheet, this is probably a single-song file.
//			qDebug() << "No cuesheet for file" << this->m_url;

			auto new_entry = std::make_shared<LibraryEntry>(*this);
			new_entry->m_metadata = file_metadata;
			new_entry->m_length_secs = file_metadata.total_length_seconds();
			new_entry->m_is_subtrack = false;
			new_entry->m_is_populated = true;
			new_entry->m_is_error = false;
			retval.push_back(new_entry);
		}
		else
		{
			// We did get a cue sheet, could be more than one track in this file.

			///qDebug() << "Track numbers:" << "???";

			for(int tn = 1; tn <= file_metadata.numTracks(); ++tn)
			{
				TrackMetadata sheet_track = file_metadata.track(tn);
				qDebug() << "Track number:" << sheet_track.m_track_number;

				// Have the file_metadata object "split" itself and give us just the metadata as if there
				// was only this track in the file.
				Metadata track_metadata = file_metadata.get_one_track_metadata(tn);

				/// @todo Scan indexes here?

				m_offset_secs = Fraction(double(sheet_track.m_start_frames)/(75.0), 1);
				if(sheet_track.m_length_frames > 0)
				{
					// Not the last track.
					m_length_secs = Fraction(double(sheet_track.m_length_frames)/(75.0), 1);
				}
				else
				{
					// Must be the last track, so there's no cue entry for the start of the next track,
					// so we have to calculate the length manually.
					/// @note We no longer have to do this here, this is done in the Metadata class on cue sheet read.
					Q_ASSERT(0);
				}


				/// Create the new entry.
				auto new_entry = std::make_shared<LibraryEntry>(*this);
				new_entry->m_track_number = tn;
//M_WARNING("THIS IS ALWAYS 0")
                new_entry->m_total_track_number = file_metadata.numTracks();
                new_entry->m_metadata = track_metadata;
				new_entry->m_offset_secs = m_offset_secs;
				new_entry->m_length_secs = m_length_secs;
				new_entry->m_is_subtrack = (file_metadata.numTracks() > 1);
				new_entry->m_is_populated = true;
				new_entry->m_is_error = false;

//                qDb() << "LIBENTRY:" << tn << new_entry->getAllMetadata();

				retval.push_back(new_entry);
			}
		}
	}

	return retval;
}

std::shared_ptr<LibraryEntry> LibraryEntry::refresh_metadata()
{
	std::shared_ptr<LibraryEntry> retval = nullptr;

	// Some sanity checks first.
	if(!m_url.isValid())
	{
		// Nothing to do.
		qDebug() << "Invalid URL or already populated.";
		return retval;
	}

	// Try to read the metadata of the file.
	Metadata file_metadata = Metadata::make_metadata(m_url);
	if(!file_metadata)
	{
		// Read failed.
		qWarning() << "Can't get metadata for file" << m_url;
		auto new_entry = std::make_shared<LibraryEntry>(*this);
		new_entry->m_is_populated = true;
		new_entry->m_is_error = true;
		retval = new_entry;
	}
	else
	{
		if(!file_metadata.hasCueSheet())
		{
			// Couldn't load a cue sheet, this is probably a single-song file.
			qDebug() << "No cuesheet for file" << this->m_url;

			auto new_entry = std::make_shared<LibraryEntry>(*this);
			new_entry->m_metadata = file_metadata;
			new_entry->m_length_secs = file_metadata.total_length_seconds();
			new_entry->m_is_subtrack = false;
			new_entry->m_is_populated = true;
			new_entry->m_is_error = false;
			retval = new_entry;
		}
		else
		{
			/// Multitrack file.
M_WARNING("TODO MULTITRACK METADATA REFRESH")
			Q_ASSERT(0);
		}
	}
	return retval;
}

bool LibraryEntry::isFromSameFileAs(const LibraryEntry *other) const
{
	// Are the two entries from the same file?
	if(m_url == other->m_url)
	{
		return true;
	}
	else
	{
		return false;
	}
}


bool LibraryEntry::hasNoPregap() const
{
	if(isPopulated() && !isError())
	{
		if(isSubtrack())
		{
			if(m_metadata)
			{
				if(m_track_number < 1)
				{
//					qCritical() << "TRACK NO LESS THAN 1:" << m_track_number;
					return false;
				}
				TrackMetadata tm = m_metadata.track(m_track_number);
				if(tm.m_is_part_of_gapless_set)
				{
//					qDebug() << "Track is part of gapless set:" << m_track_number << "in file:" << m_url;
					return true;
				}
			}
		}
	}

	return false;
}

QUrl LibraryEntry::getM2Url() const
{
	if(isPopulated())
	{
		if(isSubtrack())
		{
			auto decurl = m_url;
			QUrlQuery query;
			query.addQueryItem("track_name", getMetadata("track_name").at(0));
			auto ntpfrag = ntp(double(m_offset_secs), double(m_offset_secs+m_length_secs));
			auto keyval = ntpfrag.toKeyValPair();
			query.addQueryItem(QString::fromStdString(keyval.key), QString::fromStdString(keyval.value));
			decurl.setFragment(query.toString());
			return decurl;
		}
		else
		{
			return m_url;
		}
	}
	else
	{
		qWarning() << "Item" << m_url << "not populated, No M2Url available.";
		return QUrl();
    }
}

//void LibraryEntry::writeToJson(QJsonObject& jo) const
//{
//	jo["m_url"] = m_url.toString();
//	jo["m_is_populated"] = isPopulated();
//	jo["m_is_error"] = m_is_error;
//	jo["m_is_subtrack"] = m_is_subtrack;
//	jo["m_offset_secs"] = m_offset_secs.toQString();
//	jo["m_length_secs"] = m_length_secs.toQString();
//
//	M_WARNING("/// @todo This is always null.");
//	QString str;
//	QTextStream ts(&str);
//	//	ts << m_mime_type;
//	jo["m_mime_type"] = *ts.string();
//
//	if(isPopulated())
//	{
//		M_WARNING("TODO: Don't write out in the has-cached-metadata case");
//		m_metadata.writeToJson(jo);
//	}
//}

//void LibraryEntry::readFromJson(QJsonObject& jo)
//{
//	m_url = QUrl(jo["m_url"].toString());
//	m_is_populated = jo["m_is_populated"].toBool(false);
//	m_is_error = jo["m_is_error"].toBool(false);
//	m_is_subtrack = jo["m_is_subtrack"].toBool(false);
//	m_offset_secs = Fraction(jo["m_offset_secs"].toString("0/1"));
//	m_length_secs = Fraction(jo["m_length_secs"].toString("0/1"));
//	QString str;
//	QTextStream ts(&str);
//	str = jo["m_mime_type"].toString();
////	ts >> m_mime_type;
//	// Metadata might not have been written.
//	//metadata_jval: QJsonValue = jo.value("metadata")
//	QJsonObject metadata_jval = jo["metadata"].toObject();
//	if(!metadata_jval.empty())
//	{
//		///qDebug() << "Found metadata in JSON";
//		m_metadata = Metadata::make_metadata(metadata_jval);
//		m_is_populated = true;
//	}
//	else
//	{
//		qWarning() << "Found no metadata in JSON for" << m_url;
//		m_metadata = Metadata::make_metadata();
//		m_is_error = true;
//		m_is_populated = false;
//	}
//	return;
//}

#define M_DATASTREAM_FIELDS(X) \
	X(XMLTAG_URL, m_url) \
	X(XMLTAG_IS_POPULATED, m_is_populated) \
	X(XMLTAG_IS_ERROR, m_is_error) \
	X(XMLTAG_MIME_TYPE, m_mime_type) \
	X(XMLTAG_IS_SUBTRACK, m_is_subtrack) \
	X(XMLTAG_TRACK_NUMBER, m_track_number) \
	X(XMLTAG_TOTAL_TRACK_NUMBER, m_total_track_number) \
	X(XMLTAG_PRE_GAP_OFFSET_SECS, m_pre_gap_offset_secs) \
	X(XMLTAG_OFFSET_SECS, m_offset_secs) \
	X(XMLTAG_LENGTH_SECS, m_length_secs)

using strviw_type = QLatin1Literal;

/// Strings to use for the tags.
#define X(field_tag, member_field) static const strviw_type field_tag ( # member_field );
	M_DATASTREAM_FIELDS(X);
#undef X

QVariant LibraryEntry::toVariant() const
{
	QVariantInsertionOrderedMap map;

	// Insert field values into the QVariantMap.
#define X(field_tag, member_field)   map.insert( field_tag , QVariant::fromValue<decltype(member_field)>( member_field ) );
	M_DATASTREAM_FIELDS(X);
#undef X

	if(isPopulated())
	{
		map.insert(LibraryEntryTag::METADATA_tagstr, m_metadata.toVariant());
	}

	return QVariant::fromValue(map);
}

void LibraryEntry::fromVariant(const QVariant& variant)
{
	QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();

	// Extract all the fields from the map, cast them to their type.
#define X(field_tag, member_field)   member_field = map.value( field_tag ).value<decltype(member_field)>();
	M_DATASTREAM_FIELDS(X);
#undef X

	/// @todo
	if(isPopulated())
	{
		/// @todo This is badly named. m_metadata the field has a "metadata_abstract_base_pimpl" QVarInsOrderMap inside it.
		QVariant metadata_map = map.value(LibraryEntryTag::METADATA_tagstr);

//		Q_ASSERT(metadata_map.value().canConvert<MetadataFromCache>());

		if(metadata_map.isValid())
		{
			m_metadata = Metadata::make_metadata(metadata_map);
			m_is_error = false;
			m_is_populated = true;
		}
		else
		{
			qWarning() << "Found no/invalid metadata in XML for" << m_url;
			m_metadata = Metadata::make_metadata();
			m_is_error = true;
			m_is_populated = false;
		}
	}

}

#undef M_DATASTREAM_FIELDS

QByteArray LibraryEntry::getCoverImageBytes()
{
	if(isPopulated() && !isError())
	{
		return m_metadata.getCoverArtBytes();
	}
	else
	{
		return QByteArray();
	}
}

QMap<QString, QVariant> LibraryEntry::getAllMetadata() const
{
	QMap<QString, QVariant> retval;

	if(isPopulated())
	{
		TagMap tm = m_metadata.filled_fields();
		QStringList sl;
		for(const auto& entry : tm)
		{
//qDb() << "entry:" << entry;
			sl.clear();
			QString key = QString::fromStdString(entry.first);
			QVariant temp_val = retval[key];
//qDb() << "temp_val:" << temp_val;
            for(const auto& s : entry.second)
			{
//qDb() << "Appending to temp_val:" << s;
				sl.append(QString::fromStdString(s));
			}
//qDb() << "sl:" << sl;
			retval[key] = QVariant::fromValue(sl);
		}
	}

	return retval;
}

QStringList LibraryEntry::getMetadata(QString key) const
{
	if(isPopulated() && !isError())
	{
		std::string str = m_metadata[tostdstr(key)];
		if(str.empty())
		{
			return QStringList();
		}
		else
		{
			return QStringList(toqstr(str));
		}
	}
	else
	{
		return QStringList();
	}
}

//
// QDataStream operators
//

QDataStream& operator<<(QDataStream& out, const LibraryEntry& myObj)
{
	Q_ASSERT(0);
	// Magic number and version.
	out << (quint32)LIBRARY_ENTRY_MAGIC_NUMBER;
	out << (quint32)LIBRARY_ENTRY_VERSION;
	out.setVersion(QDataStream::Qt_5_9);
	// Write the data.
	out << myObj.m_url;
	out << myObj.m_is_populated;
	out << myObj.m_is_error;
	out << myObj.m_is_subtrack;
	out << myObj.m_offset_secs;
	out << myObj.m_length_secs;
    out << myObj.m_mime_type;
//	if(isPopulated())
//	{
//		M_WARNING("TODO: Don't write out in the has-cached-metadata case")
//		m_metadata.writeToJson(jo);
//	}
	return out;
}

QDataStream& operator>>(QDataStream& in, LibraryEntry& myObj)
{
	Q_ASSERT(0);
	// Read and check the header.
	quint32  magic_number;
	in >> magic_number;
	if(magic_number != LIBRARY_ENTRY_MAGIC_NUMBER)
	{
		/// @todo
		return in;
	}
	// Read the version
	quint32 version;
	in >> version;
	if(version != LIBRARY_ENTRY_VERSION)
	{
		// @todo Can't understand the version.
		return in;
	}

	in >> myObj.m_url;
	in >> myObj.m_is_populated;
	in >> myObj.m_is_error;
	in >> myObj.m_is_subtrack;
	in >> myObj.m_offset_secs;
	in >> myObj.m_length_secs;

	return in;
}

