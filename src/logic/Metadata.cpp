/*
 * Copyright 2017, 2018, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "Metadata.h"

// Std C++.
#include <memory>
#include <string>
#include <map>

// Qt5
#include <QVariant>
#include <QVariantMap>

// TagLib includes.
#if 1
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>
#include <taglib/audioproperties.h>
#include <taglib/mpegfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/wavfile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/id3v1tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/apetag.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>
#else
#include <tag.h>
#include <fileref.h>
#include <tpropertymap.h>
#include <audioproperties.h>
#include <mpegfile.h>
#include <vorbisfile.h>
#include <wavfile.h>
#include <attachedpictureframe.h>
#include <id3v1tag.h>
#include <id3v2tag.h>
#include <apetag.h>
#include <flacfile.h>
#include <flacpicture.h>
#endif

// Ours.
#include "TagLibHelpers.h"
#include "MetadataTaglib.h"
#include "MetadataFromCache.h"
#include "utils/MapConverter.h"
#include <utils/DebugHelpers.h>
#include <utils/RegisterQtMetatypes.h>
#include "CueSheet.h"


AMLM_QREG_CALLBACK([](){
	qIn() << "Registering Metadata metatypes";
	qRegisterMetaType<Metadata>();
	qRegisterMetaTypeStreamOperators<Metadata>();
//	QMetaType::registerDebugStreamOperator<Metadata>();
//	QMetaType::registerConverter<Metadata, QString>([](const Metadata& obj){ return obj.name(); });
});

QTH_DEFINE_DUMMY_QDEBUG_OP(Metadata);


static std::map<AudioFileType::Type, std::string> f_filetype_to_string_map =
{
	{AudioFileType::UNKNOWN, "unknown"},
	{AudioFileType::FLAC, "FLAC"},
	{AudioFileType::MP3, "MP3"},
	{AudioFileType::OGG_VORBIS, "Ogg Vorbis"},
	{AudioFileType::WAV, "WAV"}
};

static std::set<std::string> f_newly_discovered_keys;

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

/**
 * @link https://xiph.org/vorbis/doc/v-comment.html
 */
static const std::map<std::string, std::vector<std::string>> f_vorbis_comment_normalization_map =
{
	{"album_name", {"ALBUM"}},
	{"album_artist", {"ALBUMARTIST"}},
	{"track_name", {"TITLE"}},
	{"track_subtitle", {"SUBTITLE"}},
	{"genre", {"GENRE"}},
	{"mood", {"MOOD"}},
	{"style", {"STYLE"}},
	{"disc_total", {"DISCTOTAL", "TOTALDISCS"}},
	{"disc_number", {"DISCNUMBER"}},
	{"track_number", {"TRACKNUMBER"}},
	{"track_total", {"TRACKTOTAL", "TOTALTRACKS"}},
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



std::set<std::string> Metadata::getNewTags()
{
	return f_newly_discovered_keys;
}

static AMLMTagMap PropertyMapToAMLMTagMap(TagLib::PropertyMap pm)
{
	AMLMTagMap retval;
	for(const auto& key_val_pairs : pm)
	{
		std::string key = tostdstr(key_val_pairs.first);

		std::vector<std::string> out_val;
		// Iterate over the StringList for this key.
		for(const auto& value : key_val_pairs.second)
		{
			auto sstr = tostdstr(value);
			retval.insert({key, sstr});
		}
	}
	qDb() << "Returning:" << retval;
	return retval;
}


Metadata Metadata::make_metadata()
{
	return Metadata();
}

Metadata Metadata::make_metadata(QUrl file_url)
{
	Metadata retval;
	retval.read(file_url);
	return retval;
}

static constexpr QLatin1String XMLTAG_METADATA_ABSTRACT_BASE_PIMPL {"metadata_abstract_base_pimpl"};


Metadata Metadata::make_metadata(const QVariant& variant)
{
	Q_ASSERT(variant.isValid());
#if 0
M_TODO("What's going on here?");
/// Contains only "metadata_abstract_base_pimpl", which contains only "base_class_variant".
	QVariantMap map = variant.toMap();

	QVariant pimpl_qvar= map.value(XMLTAG_METADATA_ABSTRACT_BASE_PIMPL);
	Q_ASSERT(pimpl_qvar.isValid());

//	Q_ASSERT(map.canConvert<MetadataFromCache>());
	MetadataFromCache retval;
	retval.fromVariant(pimpl_qvar);
	retval.m_read_has_been_attempted = true;
	retval.m_is_error = false;
	return retval;
#else
	QVariantInsertionOrderedMap map; //= variant.value<QVariantInsertionOrderedMap>();
	qviomap_from_qvar_or_die(&map, variant);
	Metadata retval = make_metadata();
	retval.fromVariant(variant);
	return retval;
#endif
}

/**
 * Read the metadata associated with the given URL with TagLib.
 */
bool Metadata::read(const QUrl& url)
{
	// String for storing an embedded cuesheet if we have one.
	std::string cuesheet_str;

	m_audio_file_url = url;

	QString url_as_local = url.toLocalFile();

	// Open a TagLib FileRef on the file.
	// Use "Accurate" mode for reading audio property info.
	TagLib::FileRef fr { openFileRef(url_as_local, /*read audio props:*/true, TagLib::AudioProperties::Accurate) };
	if(fr.isNull())
	{
		qWr() << "Unable to open file" << url_as_local << "with TagLib";
		m_is_error = true;
		m_read_has_been_attempted = true;
		return false;
	}

	//
	// Read the AudioProperties.
	//
	TagLib::AudioProperties* audio_properties;
	audio_properties = fr.audioProperties();
	if(audio_properties != nullptr)
	{
		// Got some audio properties.
		m_bitrate_kb_sec = audio_properties->bitrate();
		m_num_channels = audio_properties->channels();
		m_length_in_milliseconds = audio_properties->lengthInMilliseconds();
		m_sample_rate = audio_properties->sampleRate();
	}
	else
	{
		qWr() << "AudioProperties was null";
	}


	//
	// Tags
	//
//	TagLib::PropertyMap pm = fr.tag()->properties();
//	for(const auto& cit : pm)
//	{
//		auto key = cit.first;
//		for(const auto& val : cit.second)
//		{
//			qDb() << "PROPERTYMAP:" << key << val;
//		}
//	}

	// Downcast the FileRef to whatever type it really is.
	if (TagLib::MPEG::File* file = dynamic_cast<TagLib::MPEG::File*>(fr.file()))
	{
		m_audio_file_type = AudioFileType::MP3;
		m_has_ape = file->hasAPETag();
		m_has_id3v1 = file->hasID3v1Tag();
		m_has_id3v2 = file->hasID3v2Tag();

		if(m_has_id3v1) { m_tm_id3v1 = PropertyMapToAMLMTagMap(file->ID3v1Tag()->properties()); }
		if(m_has_id3v2) { m_tm_id3v2 = PropertyMapToAMLMTagMap(file->ID3v2Tag()->properties()); }
		if(m_has_ape) { m_tm_ape = PropertyMapToAMLMTagMap(file->APETag()->properties()); }
	}
	else if(TagLib::FLAC::File* file = dynamic_cast<TagLib::FLAC::File*>(fr.file()))
	{
		m_audio_file_type = AudioFileType::FLAC;
		m_has_ogg_xipfcomment = file->hasXiphComment();
		m_has_id3v1 = file->hasID3v1Tag();
		m_has_id3v2 = file->hasID3v2Tag();

		if(m_has_id3v1) m_tm_id3v1 = PropertyMapToAMLMTagMap(file->ID3v1Tag()->properties());
		if(m_has_id3v2) m_tm_id3v2 = PropertyMapToAMLMTagMap(file->ID3v2Tag()->properties());
		if(m_has_ogg_xipfcomment)
		{
			// TagLib has some funky kicks going on here:
			/// @link https://taglib.org/api/classTagLib_1_1FLAC_1_1File.html#a31ffa82b2e168f5625311cbfa030f04f
			// Re ->tag(): "Returns the Tag for this file. This will be a union of XiphComment, ID3v1 and ID3v2 tags."
			// Not sure that's true, but there's also xiphComment():
			// "Returns a pointer to the XiphComment for the file.
			// Note
			// This may return a valid pointer regardless of whether or not the file on disk has a XiphComment. Use hasXiphComment()
			// to check if the file on disk actually has a XiphComment."
			TagLib::Ogg::XiphComment* xipf_comment;
			xipf_comment = file->xiphComment();
			m_tm_xipf = xipf_comment->fieldListMap();

			auto field_count = xipf_comment->fieldCount();
			qDb() << "### XIPH: Field Count:" << field_count << "FieldListMap size:" << m_tm_xipf.size();

			// Extract any CUESHEET embedded in the XiphComment.
			cuesheet_str = get_cue_sheet_from_OggXipfComment(file).toStdString();
		}
	}
	else if(TagLib::Ogg::Vorbis::File* file = dynamic_cast<TagLib::Ogg::Vorbis::File*>(fr.file()))
	{
		m_audio_file_type = AudioFileType::OGG_VORBIS;
		if(auto tag = file->tag())
		{
			m_has_ogg_xipfcomment = true;
			TagLib::Ogg::XiphComment* xipf_comment;
			xipf_comment = file->tag();
			m_tm_xipf = xipf_comment->fieldListMap();
		}
	}
	else if(TagLib::RIFF::WAV::File* file = dynamic_cast<TagLib::RIFF::WAV::File*>(fr.file()))
	{
		// Wav file.
		m_audio_file_type = AudioFileType::WAV;
		m_has_id3v2 = file->hasID3v2Tag();
		m_has_info_tag = file->hasInfoTag();

		if(m_has_info_tag)
		{
			m_tm_infotag = PropertyMapToAMLMTagMap(file->InfoTag()->properties());
		}

		if(m_has_id3v2)
		{
			m_tm_id3v2 = PropertyMapToAMLMTagMap(file->ID3v2Tag()->properties());
		}
	}

	// Read a dictionary mapping of the tags.
	TagLib::Tag* tag = fr.tag();
	if(tag == nullptr)
	{
		qWarning() << "File" << m_audio_file_url << "returned a null tag.";
	}
	else
	{
M_WARNING("BUG: Pulls data from bad cuesheet embeds in FLAC, such as some produced by EAC");
	/// @todo The sidecar cue sheet support will then also kick in, and you get weirdness like a track will have two names.
	/// Need to do some kind of comparison/validity check.

		/// @note TagLib docs: "Exports the tags of the file as dictionary mapping (human readable)
		/// tag names (Strings) to StringLists of tag values. The default implementation in this class
		/// considers only the usual built-in tags (artist, album, ...) and only one value per key."
		TagLib::PropertyMap pm = tag->properties();

		for(const auto& e : pm)
		{
			qDb() << "TagLib properties Property Map:" << e.first << e.second.toString("///");
		}
		m_tag_map = PropertyMapToAMLMTagMap(pm);
	}


	//
	// Cuesheet handling, using libcue.
	//
	std::unique_ptr<CueSheet> cuesheet;
	cuesheet.reset();

	// Did we find an embedded cue sheet?
	if(!cuesheet_str.empty())
	{
		cuesheet = CueSheet::TEMP_parse_cue_sheet_string(cuesheet_str, m_length_in_milliseconds);
		Q_ASSERT(cuesheet);
	}
	else
	{
		// Try to read a possible sidecar cue sheet file.
		cuesheet = CueSheet::read_associated_cuesheet(m_audio_file_url, m_length_in_milliseconds);
	}

	// Did we find a cue sheet?
	if(cuesheet)
	{
		m_has_cuesheet = true;
		m_cuesheet = *cuesheet;

		// Get the disc-level cuesheet info as an AMLMTagMap.
		m_tm_cuesheet_disc = m_cuesheet.asAMLMTagMap_Disc();

		m_num_tracks_on_media = cuesheet->get_total_num_tracks();

		// Copy the cuesheet track info.
		m_tracks = cuesheet->get_track_map();
qDb() << "####### NUM TRACKS:" << m_tracks.size();
		Q_ASSERT(m_tracks.size() > 0);


		// Ok, now do a second pass over the tracks and determine if there are any gapless sets.
		qDebug() << "Scanning for gaplessness...";
		for(int track_num=1; track_num < m_num_tracks_on_media; ++track_num)
		{
//            qDb() << "TRACK:" << track_num << m_tracks[track_num];

			auto next_tracknum = track_num+1;
			TrackMetadata tm1 = m_tracks[track_num];
			TrackMetadata tm2 = m_tracks[next_tracknum];
			///Frames tm2_first_possible_pregap_frame = tm2.m_length_pre_gap >= 0 ? tm2.m_start_frames - tm2.m_length_pre_gap : tm2.m_start_frames;
			Frames gap_frames_1to2 = tm2.m_length_pre_gap;/// - tm1.last_frame();
			if(gap_frames_1to2 < 5 )
			{
				// There's little or no gap.
				qDebug() << "Found a gapless track pair in" << m_audio_file_url << ":" << tm1.toStdString() << tm2.toStdString();
				m_tracks[track_num].m_is_part_of_gapless_set = true;
				m_tracks[next_tracknum].m_is_part_of_gapless_set = true;
			}
		}
	}

	m_is_error = false;
	m_read_has_been_attempted = true;
	return true;
}

bool Metadata::hasBeenRead() const
{
	return m_read_has_been_attempted;
}

bool Metadata::isError() const { return m_is_error; }

bool Metadata::isFromCache() const { return m_is_from_cache; }

Metadata::operator bool() const
{
	return hasBeenRead() && !isError();
}

std::string Metadata::GetFiletypeName() const
{
	return f_filetype_to_string_map[m_audio_file_type];
}

AMLMTagMap Metadata::tagmap_cuesheet_disc() const
{
	// Generate the AMLMTagMap and return it.
	return m_tm_cuesheet_disc;
}

Fraction Metadata::total_length_seconds() const
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

TagMap Metadata::filled_fields() const
{
	if(hasBeenRead() && !isError())
	{
		//qDebug() << "Converting filled_fields to TagMap";
		TagMap retval;
		for(const std::pair<const std::string, std::string>& key_val_pairs : m_tag_map) ///@todo EXP m_pm)
		{
			//            qDebug() << "Native Key:" << key_val_pairs.first;
			std::string key = reverse_lookup(key_val_pairs.first);
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

TrackMetadata Metadata::track(int i) const
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

Metadata Metadata::get_one_track_metadata(int track_index) const
{
	// Start off with a complete duplicate.
	Metadata retval(*this);

	// Now replace the track map with only the entry for this one track.
	//qIn() << "BEFORE:" << retval.m_tracks;
	std::map<int, TrackMetadata> new_track_map;
	auto track_entry = m_tracks.at(track_index);
	new_track_map.insert({track_index, track_entry});

	retval.m_tracks = new_track_map;

	//qIn() << "AFTER:" << retval.m_tracks;

	// Copy any track-specific CDTEXT data to the "top level" metadata.
	M_WARNING("TODO: This could probably be improved, e.g. not merge these in but keep the track info separate");
	if(!track_entry.m_PTI_TITLE.empty())
	{
		//    qIn() << M_NAME_VAL(retval.m_tag_map["TITLE"]);
		qDebug() << "NEW TRACK_NAME:" << track_entry.m_PTI_TITLE;
		//		retval.m_tag_map["TITLE"].push_back(track_entry.m_PTI_TITLE);
		retval.m_tag_map.insert({"TITLE", track_entry.m_PTI_TITLE});
		//    qIn() << M_NAME_VAL(retval.m_tag_map["TITLE"]);
	}
	if(!track_entry.m_PTI_PERFORMER.empty())
	{
		retval.m_tag_map["PERFORMER"].push_back(track_entry.m_PTI_PERFORMER);
	}
	if(!track_entry.m_isrc.empty())
	{
		//		retval.m_tag_map["ISRC"].push_back(track_entry.m_isrc);
		retval.m_tag_map.insert({"ISRC", track_entry.m_isrc});
	}

	return retval;
}

bool Metadata::hasTrack(int i) const
{
	if(m_tracks.find(i) != m_tracks.cend())
	{
		return true;
	}
	else
	{
		return false;
	}
}

QByteArray Metadata::getCoverArtBytes() const
{
	// static function in MetadataTagLib.h/.cpp.
	return getCoverArtBytes();
}

std::string Metadata::operator[](const std::string& key) const
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
		return stringlist[0];
	}
}

using strviw_type = QLatin1Literal;

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
	X(XMLTAG_TM_M_TAG_MAP, m_tag_map) \
	X(XMLTAG_DISC_CUESHEET, m_tm_cuesheet_disc)

/// Strings to use for the tags.
#define X(field_tag, member_field) static const strviw_type field_tag ( # member_field );
	M_DATASTREAM_FIELDS(X);
	M_DATASTREAM_FIELDS_MAPS(X);
#undef X
static const strviw_type XMLTAG_TRACKS("m_tracks");
static const strviw_type XMLTAG_CUESHEET("m_cuesheet");



QVariant Metadata::toVariant() const
{
	QVariantInsertionOrderedMap map;

#define X(field_tag, member_field)   map_insert_or_die(map, field_tag, member_field);
	M_DATASTREAM_FIELDS(X);
#undef X

#define X(field_tag, member_field) map_insert_or_die(map, field_tag, member_field);
//#define X(field_tag, member_field) map.insert( field_tag , member_field.toVariant());
	M_DATASTREAM_FIELDS_MAPS(X);
#undef X

	// Track-level fields.
M_WARNING("TODO: Do we still need this?");

	// Add the track list to the return map.
	QVariantHomogenousList qvar_track_map("m_track", "track");

	for(const auto& it : m_tracks)
	{
//		QString track_num_str = QString("track");
		TrackMetadata tm = it.second;
		list_push_back_or_die(qvar_track_map, tm);
	}

	map_insert_or_die(map, XMLTAG_TRACKS, qvar_track_map);
	map_insert_or_die(map, XMLTAG_CUESHEET, m_cuesheet);

	return map;
}

void Metadata::fromVariant(const QVariant& variant)
{
	QVariantInsertionOrderedMap map;
	qviomap_from_qvar_or_die(&map, variant);

//#define X(field_tag, member_field)   member_field = map.value( field_tag ).value<decltype(member_field)>();
#define X(field_tag, member_field)   map_insert_or_die(map, field_tag, member_field);
	M_DATASTREAM_FIELDS(X);
#undef X

#define X(field_tag, member_field) map_read_field_or_warn(map, field_tag, &member_field);
	M_DATASTREAM_FIELDS_MAPS(X);
#undef X

	m_cuesheet.fromVariant(map.value(XMLTAG_CUESHEET));//, m_cuesheet.toVariant());


	// Read in the track list.
M_WARNING("TODO: Not sure we need this anymore.");
	QVariantInsertionOrderedMap qvar_track_map = map.value(XMLTAG_TRACKS).value<QVariantInsertionOrderedMap>();
	for(const auto& track : qvar_track_map)
	{
		TrackMetadata tm;
		tm.fromVariant(track.second);

		// Should have a track number.
		throwif(tm.m_track_number == 0);
		int track_num = tm.m_track_number;

		m_tracks.insert(std::make_pair(track_num, tm));
	}

	m_read_has_been_attempted = true;
	m_is_error = false;
}

#undef M_DATASTREAM_FIELDS


QDataStream &operator<<(QDataStream &out, const Metadata &obj)
{
	Q_ASSERT(0);
	/// @todo
//	out << MapConverter::TagMaptoVarMap(obj.pImpl->m_tag_map);
	return out;
}

QDataStream &operator>>(QDataStream &in, Metadata &obj)
{
	Q_ASSERT(0);
	/// @todo

	QVariantMap tag_map;
	using tag_map_var_type = std::map<std::string, std::vector<std::string>>;
	tag_map_var_type tag_map_std;
///	in >> tag_map_std;

//	obj.pImpl->m_tag_map = tag_map_std;

	return in;
}
