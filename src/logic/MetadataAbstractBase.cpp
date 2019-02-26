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

#include "MetadataAbstractBase.h"

// Std C++
#include <map>
#include <string>

// Qt5
#include <QDebug>

// Ours.
#include "MetadataTaglib.h"
#include "utils/Fraction.h"
#include "utils/StringHelpers.h"
#include "utils/DebugHelpers.h"
#include <utils/RegisterQtMetatypes.h>


/**
 * Misc Metadata info:
 *
 * - DISCID (freedb):
 * Shows up in cuesheet as e.g. "REM DISCID D00DA810" (http://wiki.hydrogenaud.io/index.php?title=Cue_sheet, http://wiki.hydrogenaud.io/index.php?title=EAC_CUE_Sheets)
 * https://sound.stackexchange.com/questions/39229/how-is-discid-made-in-cue-sheet-files
 * " As mentioned in the DiscID howto that you can find here : http://ftp.freedb.org/pub/freedb/misc/freedb_howto1.07.zip,
 *   The disc ID is an 8-digit hexadecimal (base-16) number, computed using data from a CD's Table-of-Contents (TOC) in MSF (Minute Second Frame) form.
 *   This document includes a description of the algorithm used to compute the DiscID of a given audio CD."
 */


//AMLM_QREG_CALLBACK([](){
//	qIn() << "Registering AudioFileType";
////	qRegisterMetaTypeStreamOperators<AudioFileType>();
////	qRegisterMetaType<AudioFileType::Type>("AudioFileType::Type");
////	AMLMRegisterQEnumQStringConverters<AudioFileType::Type>();
//
////	QMetaType::registerConverter<AudioFileType, QString>([](const AudioFileType& qenum){
////		return toqstr(qenum);
////	});
////	QMetaType::registerConverter<QString, AudioFileType>([](const QString& str){
////		QVariant var = str;
////		AudioFileType retval = var.value<AudioFileType>();
//////		retval.fromValue(tostdstr(str).c_str());
////		return retval;
////	});
//});


/// Interface name to Taglib name map.
/// @see http://wiki.hydrogenaud.io/index.php?title=Tag_Mapping
static const std::map<std::string, std::string> f_name_normalization_map =
{
	{"track_name", "TITLE"},
	{"track_number", "TRACKNUMBER"},
	{"track_total", ""},
	{"album_name", "ALBUM"},
	{"album_artist", "ALBUMARTIST"},
	{"track_artist", "ARTIST"},
	{"track_performer", "PERFORMER"},
	{"composer_name", "COMPOSER"},
	{"conductor_name", "CONDUCTOR"},
	{"genre", "GENRE"},
	{"media", "MEDIA"},
	{"ISRC", "ISRC"},
	{"catalog", "CATALOGNUMBER"},
	{"comment", "COMMENT"},
};

static std::string reverse_lookup(const std::string& native_key)
{
	for(const auto& entry : f_name_normalization_map)
	{
		if(entry.second == native_key)
		{
			return entry.first;
		}
	}
	return native_key;
}

std::string MetadataAbstractBase::operator[](const std::string& key) const
{
	std::string native_key_string;

	auto it = f_name_normalization_map.find(key);
	if(it != f_name_normalization_map.end())
	{
		// Found it.
		native_key_string = it->second;
	}
	else
	{
		// Didn't find it.
		native_key_string = "";
		return native_key_string;
	}

//	TagLib::StringList stringlist = m_pm[native_key_string];
	std::vector<std::string> stringlist = m_tag_map.equal_range_vector(native_key_string);

//	auto strlist_it = m_tag_map.find(native_key_string);
//	if(strlist_it != m_tag_map.cend())
//	{
//		stringlist = strlist_it->second;
//	}
//	else
//	{
////		qDebug() << "No such key:" << native_key_string;
//	}

	if(stringlist.empty())
	{
		return "";
	}
	else
	{
		/// @todo EXP return stringlist[0].toCString(true);
		return stringlist[0];
	}
}

TagMap MetadataAbstractBase::filled_fields() const
{
	if(hasBeenRead() && !isError())
	{
//qDebug() << "Converting filled_fields to TagMap";
		TagMap retval;
		for(const std::pair<const std::string, std::string>& key_val_pairs : m_tag_map) ///@todo EXP m_pm)
		{
//            qDebug() << "Native Key:" << key_val_pairs.first;
			std::string key = reverse_lookup(key_val_pairs.first); ///@todo EXP.toCString());
//            qDebug() << "Normalized key:" << key;

			if(key.empty() || key.length() == 0)
			{
				// We found an unknown key.
M_WARNING("TODO: Find a better way to track new keys.")
				///f_newly_discovered_keys.insert(key_val_pairs.first); ///@todo EXP .toCString(true));
				continue;
			}

			std::vector<std::string> out_val = m_tag_map.equal_range_vector(key_val_pairs.first);
			// Iterate over the StringList for this key.
//			for(const auto& value : key_val_pairs.second)
//			{
//				/// @todo Not sure what I was doing here.
//				///@todo EXP std::string val_as_utf8 = value.to8Bit(true);
//				//qDebug() << "Value:" << val_as_utf8 << QString::fromUtf8(val_as_utf8.c_str());
//				out_val.push_back(value); ///@todo EXP .toCString(true));
//			}
			retval[key] = out_val;
		}
//        qDebug() << "Returning:" << retval;
		return retval;
	}
	else
	{
		return TagMap();
	}
}

Fraction MetadataAbstractBase::total_length_seconds() const
{
	if(hasBeenRead() && !isError())
	{
		return Fraction(m_length_in_milliseconds, 1000);
	}
	else
	{
		return Fraction(0, 1);
	}
}

TrackMetadata MetadataAbstractBase::track(int i) const
{
	// Incoming index is 1-based.
	Q_ASSERT(i > 0);
//	if(size_t(i) >= m_tracks.size()+1)
//	{
//		qFatal("i: %d, m_tracks.size()+1: %lu", i, m_tracks.size()+1);
//	}

	try
	{
		TrackMetadata retval = m_tracks.at(i);
		return retval;
	}
	catch (...)
	{
		qFatal("No TrackMetadata found for i: %d, m_tracks.size()+1: %lu", i, m_tracks.size()+1);
	}

//	return retval;
}

/// @aside ...uhhhhhhhhh........
using strviw_type = QString;

#define M_DATASTREAM_FIELDS(X) \
	/*X(XMLTAG_AUDIO_FILE_TYPE, m_audio_file_type)*/ \
	X(XMLTAG_HAS_CUESHEET, m_has_cuesheet) \
	X(XMLTAG_HAS_VORBIS_COMMENT, m_has_vorbis_comment) \
	X(XMLTAG_HAS_ID3V1, m_has_id3v1) \
	X(XMLTAG_HAS_ID3V2, m_has_id3v2) \
	X(XMLTAG_HAS_APE, m_has_ape) \
	X(XMLTAG_HAS_OGG_XIPFCOMMENT, m_has_ogg_xipfcomment) \
	X(XMLTAG_HAS_INFO_TAG, m_has_info_tag) \
	X(XMLTAG_AUDIO_FILE_URL, m_audio_file_url) \
	X(XMLTAG_NUM_TRACKS_ON_MEDIA, m_num_tracks_on_media)

#define M_DATASTREAM_FIELDS_MAPS(X) \
	/** TagMaps */ \
	X(XMLTAG_TM_VORBIS_COMMENTS, m_tm_vorbis_comments) \
	X(XMLTAG_TM_ID3V1, m_tm_id3v1) \
	X(XMLTAG_TM_ID3V2, m_tm_id3v2) \
	X(XMLTAG_TM_APE, m_tm_ape) \
	X(XMLTAG_TM_XIPF, m_tm_xipf) \
	X(XMLTAG_TM_INFOTAG, m_tm_infotag) \
	X(XMLTAG_TM_M_TAG_MAP, m_tag_map)

/// Strings to use for the tags.
#define X(field_tag, member_field) static const strviw_type field_tag ( # member_field );
	M_DATASTREAM_FIELDS(X);
	M_DATASTREAM_FIELDS_MAPS(X);
#undef X
//static const strviw_type XMLTAG_TM_M_TAG_MAP("m_tag_map");
static const strviw_type XMLTAG_TRACKS("m_tracks");


QVariant MetadataAbstractBase::toVariant() const
{
	QVariantInsertionOrderedMap map;

#define X(field_tag, member_field)   map.insert( field_tag , QVariant::fromValue<decltype(member_field)>( member_field ) );
	M_DATASTREAM_FIELDS(X);
#undef X

//	QVariant qvar_tm = QVariant::fromValue(m_tag_map);
//	Q_ASSERT(qvar_tm.isValid());
//	map.insert(XMLTAG_TM_M_TAG_MAP, m_tag_map.toVariant());

#define X(field_tag, member_field) map.insert( field_tag , member_field . toVariant());
	M_DATASTREAM_FIELDS_MAPS(X);
#undef X

	// Add the track list to the return value.
	QVariantInsertionOrderedMap qvar_track_map;
	for(const auto& it : m_tracks)
	{
		// Using "track" prefix here because XML tags can't start with numbers.
		qvar_track_map.insert(QString("track%1").arg(it.first, 2, 10, QChar::fromLatin1('0')), it.second.toVariant());
	}

	map.insert(XMLTAG_TRACKS, qvar_track_map);

	return map;
}

void MetadataAbstractBase::fromVariant(const QVariant& variant)
{
	QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();

#define X(field_tag, member_field)   member_field = map.value( field_tag ).value<decltype(member_field)>();
	M_DATASTREAM_FIELDS(X);
#undef X

	QVariant qvar_tm = map.value(XMLTAG_TM_M_TAG_MAP);
	Q_ASSERT(qvar_tm.isValid());

	m_tag_map.fromVariant(qvar_tm);

#define X(field_tag, member_field) member_field . fromVariant(map.value(field_tag));
	M_DATASTREAM_FIELDS_MAPS(X);
#undef X

	// Read in the track list.
	QVariantInsertionOrderedMap qvar_track_map = map.value(XMLTAG_TRACKS).value<QVariantInsertionOrderedMap>();
	for(const auto& it : qvar_track_map)
	{
		// 5 == len("track"). Using "track" prefix here because XML tags can't start with numbers.
		qint64 track_num = std::stoll(tostdstr(it.first).substr(5));
		TrackMetadata tm;
		tm.fromVariant(it.second);
		m_tracks.insert(std::make_pair(track_num, tm));
	}

	m_read_has_been_attempted = true;
	m_is_error = false;
}

#undef M_DATASTREAM_FIELDS
