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

#include "MetadataTaglib.h"

#include "Metadata.h"

// Std C++
#include <map>
#include <set>
#include <vector>
#include <string>
#include <type_traits>

// Qt5
#include <QUrl>
#include <QDebug>

// TagLib includes.
#if 0
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

/// Ours
#include <logic/CueSheet.h>
#include "utils/DebugHelpers.h"
#include "utils/StringHelpers.h"
#include "TagLibHelpers.h"


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

#if 0
static std::string reverse_lookup(const std::string& native_key)
{
	for(auto entry : f_name_normalization_map)
	{
		if(entry.second == native_key)
		{
			return entry.first;
		}
	}
	return native_key;
}
#endif



std::set<std::string> MetadataTaglib::getNewTags()
{
	return f_newly_discovered_keys;
}

static TagMap PropertyMapToTagMap(TagLib::PropertyMap pm)
{
	TagMap retval;
	for(const auto& key_val_pairs : pm)
	{
		//qDebug() << "Native Key:" << key_val_pairs.first.toCString(true);
		//std::string key = reverse_lookup(key_val_pairs.first.toCString());
		//qDebug() << "Normalized key:" << key;
		std::string key = tostdstr(key_val_pairs.first);

		std::vector<std::string> out_val;
		// Iterate over the StringList for this key.
		for(const auto& value : key_val_pairs.second)
		{
			out_val.push_back(tostdstr(value));
		}
		retval[key] = out_val;
	}
	//qDebug() << "Returning:" << retval;
	return retval;
}


MetadataTaglib::MetadataTaglib() : MetadataAbstractBase()
{

}

MetadataTaglib::~MetadataTaglib()
{

}

static QString get_cue_sheet_from_OggXipfComment(TagLib::FLAC::File* file)
{
	QString retval;

	if(file->properties().contains("CUESHEET"))
	{
		qDebug() << "properties() contains CUESHEET";
		TagLib::StringList strlist = file->properties()["CUESHEET"];
		qDebug() << "CUESHEET strlist num entries:" << strlist.size();
		qDebug() << "CUESHEET strlist entries:" << strlist.toString();
		retval = toqstr(strlist.toString());
	}

//	auto xiph_comment = file->xiphComment();
//	if(xiph_comment != nullptr && !xiph_comment->isEmpty())
//	{
//		qDebug() << "Looking for cue sheet in Ogg::XiphComments, fieldcount:" << xiph_comment->fieldCount();

//		auto flm = xiph_comment->fieldListMap();
//		for(auto e : flm)
//		{
//			qDebug() << "Found key:" << e.first;
//		}
//	}

	return retval;
}

bool MetadataTaglib::read(const QUrl& url)
{
	// String for storing an embedded cuesheet if we have one.
	std::string cuesheet_str;

	m_audio_file_url = url;

    QString url_as_local = url.toLocalFile();
	// Open the file.
	TagLib::FileRef fr {openFileRef(url_as_local)};
    if(fr.isNull())
    {
        qWarning() << "Unable to open file" << url_as_local << "with TagLib";
        m_is_error = true;
        m_read_has_been_attempted = true;
        return false;
    }


	/// @todo TEST
    /// @note The generic properties do not contain CUESHEETs.
	if(fr.tag()->properties().contains("CUESHEET"))
	{
		qDebug() << "Generic properties() contains CUESHEET";
	}
	else
	{
//		qDebug() << "No generic CUESHEET";
	}

	// Downcast it to whatever type it really is.
	if (TagLib::MPEG::File* file = dynamic_cast<TagLib::MPEG::File*>(fr.file()))
	{
		m_audio_file_type = AudioFileType::MP3;
		m_has_ape = file->hasAPETag();
		m_has_id3v1 = file->hasID3v1Tag();
		m_has_id3v2 = file->hasID3v2Tag();

		if(m_has_id3v1) m_tm_id3v1 = PropertyMapToTagMap(file->ID3v1Tag()->properties());
		if(m_has_id3v2)	m_tm_id3v2 = PropertyMapToTagMap(file->ID3v2Tag()->properties());
		if(m_has_ape) m_tm_ape = PropertyMapToTagMap(file->APETag()->properties());
	}
	else if(TagLib::FLAC::File* file = dynamic_cast<TagLib::FLAC::File*>(fr.file()))
	{
		m_audio_file_type = AudioFileType::FLAC;
		m_has_ogg_xipfcomment = file->hasXiphComment();
		m_has_id3v1 = file->hasID3v1Tag();
		m_has_id3v2 = file->hasID3v2Tag();

		if(m_has_id3v1) m_tm_id3v1 = PropertyMapToTagMap(file->ID3v1Tag()->properties());
		if(m_has_ogg_xipfcomment)
		{
			m_tm_xipf = PropertyMapToTagMap(file->xiphComment()->properties());
			// Extract any CUESHEET embedded in the XiphComment.
			cuesheet_str = get_cue_sheet_from_OggXipfComment(file).toStdString();
		}
		if(m_has_id3v2) m_tm_id3v2 = PropertyMapToTagMap(file->ID3v2Tag()->properties());
	}
	else if(TagLib::Ogg::Vorbis::File* file = dynamic_cast<TagLib::Ogg::Vorbis::File*>(fr.file()))
	{
		m_audio_file_type = AudioFileType::OGG_VORBIS;
		if(auto tag = file->tag())
		{
			m_has_ogg_xipfcomment = true;
			m_tm_xipf = PropertyMapToTagMap(tag->properties());
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
			m_tm_infotag = PropertyMapToTagMap(file->InfoTag()->properties());
		}

		if(m_has_id3v2)
		{
			m_tm_id3v2 = PropertyMapToTagMap(file->ID3v2Tag()->properties());
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
#if 1
		auto pm = tag->properties();

//        for(auto e : pm)
//        {
//            qDb() << "TagLib properties Property Map:" << e.first << e.second.toString("///");
//        }
		m_tag_map = PropertyMapToTagMap(pm);

M_WARNING("BUG: THIS IS COMING BACK WITH ONE ENTRY");

//        qDb() << m_tag_map;
#endif
    }

	// Read the AudioProperties.
	TagLib::AudioProperties* audio_properties;
	audio_properties = fr.audioProperties();
	if(audio_properties != nullptr)
	{
		m_length_in_milliseconds = audio_properties->lengthInMilliseconds();
		//qDebug() << "Length in ms" << m_length_in_milliseconds;
	}
	else
	{
		qWarning() << "AudioProperties was null";
	}


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
        m_num_tracks_on_media = cuesheet->get_total_num_tracks();

        /// @todo MAYBE TEMP?
//        qDb() << "CUESHEET:" << *cuesheet;
        // Copy the cuesheet track info.
        m_tracks = cuesheet->get_track_map();
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

#if TODO
bool MetadataTaglib::hasHiddenTrackOneAudio() const
{
M_WARNING("TODO: Handle hidden track-one audio")
	Q_ASSERT(0);
	return true;
}
#endif

Metadata MetadataTaglib::get_one_track_metadata(int track_index) const
{
	// Start off with a complete duplicate.
	MetadataTaglib retval(*this);

	// Now replace the track map with only the entry for this one track.
//qIn() << "BEFORE:" << retval.m_tracks;
	std::map<int, TrackMetadata> new_track_map;
	auto track_entry = m_tracks.at(track_index);
	new_track_map.insert({track_index, track_entry});

	retval.m_tracks = new_track_map;

//qIn() << "AFTER:" << retval.m_tracks;

	// Copy any track-specific CDTEXT data to the "top level" metadata.
M_WARNING("TODO: This could probably be improved, e.g. not merge these in but keep the track info separate")
	if(track_entry.m_PTI_TITLE.size() > 0)
	{
    qIn() << M_NAME_VAL(retval.m_tag_map["TITLE"]);
		qDebug() << "NEW TRACK_NAME:" << track_entry.m_PTI_TITLE;
		retval.m_tag_map["TITLE"].push_back(track_entry.m_PTI_TITLE);
    qIn() << M_NAME_VAL(retval.m_tag_map["TITLE"]);
	}
	if(track_entry.m_PTI_PERFORMER.size() > 0)
	{
		retval.m_tag_map["PERFORMER"].push_back(track_entry.m_PTI_PERFORMER);
	}
	if(track_entry.m_isrc.size() > 0)
	{
		retval.m_tag_map["ISRC"].push_back(track_entry.m_isrc);
	}

	return retval;
}

#if TODO
int MetadataTaglib::numEmbeddedPictures() const
{
	Q_ASSERT(0);
    return 0;
}
#endif

static QByteArray getCoverArtBytes_ID3(TagLib::ID3v2::Tag* tag)
{
	const TagLib::ID3v2::FrameList& frameList = tag->frameList("APIC");
	if (!frameList.isEmpty())
	{
		qDebug() << "Found" << frameList.size() << "embedded pictures.";
		const auto* frame = (TagLib::ID3v2::AttachedPictureFrame*)frameList.front();
		return QByteArray(frame->picture().data(), frame->picture().size());
	}

	return QByteArray();
}

static QByteArray getCoverArtBytes_APE(TagLib::APE::Tag* tag)
{
	const TagLib::APE::ItemListMap& listMap = tag->itemListMap();
	if (listMap.contains("COVER ART (FRONT)"))
	{
		const TagLib::ByteVector nullStringTerminator(1, 0);
		TagLib::ByteVector item = listMap["COVER ART (FRONT)"].value();
		const int pos = item.find(nullStringTerminator);	// Skip the filename.
		if (pos != -1)
		{
			const TagLib::ByteVector& pic = item.mid(pos + 1);
			return QByteArray(pic.data(), pic.size());
		}
	}

	return QByteArray();
}

static QByteArray getCoverArtBytes_FLAC(TagLib::FLAC::File* file)
{
	const TagLib::List<TagLib::FLAC::Picture*>& picList = file->pictureList();
	if (!picList.isEmpty())
	{
		// Just grab the first image.
		const TagLib::FLAC::Picture* pic = picList[0];
		// Create a copy of the bytes.  ::fromRawData() doesn't do that.
		return QByteArray(pic->data().data(), pic->data().size());
	}

	return QByteArray();
}

QByteArray MetadataTaglib::getCoverArtBytes() const
{
	QByteArray retval;

	// Open the file ref.
    QString url_as_local = m_audio_file_url.toLocalFile();

    TagLib::FileRef fr {openFileRef(url_as_local)};
    if(fr.isNull())
    {
        qWarning() << "Unable to open file" << url_as_local << "with TagLib";
        return retval;
    }

	// Downcast it to whatever type it really is.
	if (TagLib::MPEG::File* file = dynamic_cast<TagLib::MPEG::File*>(fr.file()))
	{
		if (file->ID3v2Tag())
		{
            retval = getCoverArtBytes_ID3(file->ID3v2Tag());
		}
		if (retval.isEmpty() && file->APETag())
		{
            retval = getCoverArtBytes_APE(file->APETag());
		}
	}
	else if (TagLib::FLAC::File* file = dynamic_cast<TagLib::FLAC::File*>(fr.file()))
	{
        retval = getCoverArtBytes_FLAC(file);
		if (retval.isEmpty() && file->ID3v2Tag())
		{
            retval = getCoverArtBytes_ID3(file->ID3v2Tag());
		}
	}

	if(retval.size() > 0)
	{
		qDebug() << "Found pic data, size:" << retval.size();
	}
	else
	{
		qDebug() << "Found no pic data";
	}

	return retval;
}

MetadataTaglib* MetadataTaglib::clone_impl() const
{
	return new MetadataTaglib(*this);
}

