/*
 * Copyright 2018, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/// @file

#include <config.h>

#include "CueSheet.h"

// Std C++
#include <regex>
#include <string>
#include <string_view>

// Qt
#include <QRegularExpression>
#include <QStringLiteral>
#include <QUrl>
#include <QFile>

/// @todo Looks like VS2017 headers are broken here.  libcue.h includes <stdio.h> outside the extern "C",
/// and apparently MS's stdio.h isn't C++-safe.
extern "C" {
// Libcue
#include <libcue/libcue.h>
#include <libcue/cd.h>
}

// Ours, Qt/KF-related
#include <utils/TheSimplestThings.h>
#include <utils/RegisterQtMetatypes.h>
#include <utils/EnumFlagHelpers.h>

// Ours
#include "TrackMetadata.h"  ///< Per-track cue sheet info
#include <logic/serialization/SerializationHelpers.h>


/**
 * Same disc ripped with two different rippers:
 *
 * Bad embedded cue sheet:
 * @verbatim
 *METADATA block #2
  type: 4 (VORBIS_COMMENT)
  is last: false
  length: 201
  vendor string: reference libFLAC 1.3.1 20141125
  comments: 9
    comment[0]: ARTIST=Squeeze
    comment[1]: TITLE=Goodbye Girl
    comment[2]: ALBUM=Greatest Hits
    comment[3]: DATE=1992
    comment[4]: TRACKNUMBER=02
    comment[5]: GENRE=Unknown
    comment[6]: DISCNUMBER=1
    comment[7]: TOTALDISCS=1
    comment[8]: TOTALTRACKS=20
 * @endverbatim
 *
 * Good embedded cue sheet:
 * @verbatim
 * METADATA block #2
  type: 4 (VORBIS_COMMENT)
  is last: false
  length: 8825
  vendor string: reference libFLAC 1.3.1 20141125
  comments: 33
    comment[0]: CUESHEET=REM DISCID 1911F314
PERFORMER "Squeeze"
TITLE "Greatest Hits"
CATALOG 0082839718127
REM DATE 1992
REM DISCNUMBER 1
REM TOTALDISCS 1
REM COMMENT "CUERipper v2.1.6 Copyright (C) 2008-13 Grigory Chudov"
FILE "Squeeze - Greatest Hits.flac" WAVE
  TRACK 01 AUDIO
    PERFORMER "Squeeze"
    TITLE "Take Me, I'm Yours"
    ISRC GBAAM7801003
    INDEX 01 00:00:00
  TRACK 02 AUDIO
    PERFORMER "Squeeze"
    TITLE "Goodbye Girl"
  [...]
 * @endverbatim
 */

/// Mutex for serializing access to libcue, which is not threadsafe.
std::mutex CueSheet::m_libcue_mutex;

AMLM_QREG_CALLBACK([](){
	qIn() << "Registering CueSheet";
    qRegisterMetaType<CueSheet>();
	AMLMRegisterQEnumQStringConverters<CueSheet::Origin>();
	});

using strviw_type = QLatin1String;

#define M_SERDES_FIELDS_GENERAL(X) \
	X(XMLTAG_ORIGIN, m_origin)

#define M_DATASTREAM_FIELDS_DISC(X) \
	X(XMLTAG_DISC_CATALOG_NUM, m_disc_catalog_num) \
	X(XMLTAG_DISC_ID, m_disc_id) \
	X(XMLTAG_DISC_DATE, m_disc_date) \
	/** @todo Need to come up with an insert-as-std:string, this is a integral value. */\
	X(XMLTAG_DISC_NUM_TRACKS_ON_MEDIA, m_disc_num_tracks)

#define M_DATASTREAM_FIELDS_TRACK(X) \
//	X(XMLTAG_TRACK_META_LENGTH_POST_GAP, m_length_post_gap) \
//	X(XMLTAG_TRACK_META_ISRC, m_isrc) \
//	X(XMLTAG_TRACK_META_IS_PART_OF_GAPLESS_SET, m_is_part_of_gapless_set)

#define M_DATASTREAM_FIELDS_SPECIAL_HANDLING(X) \
	/** @todo See above */\
	/*X(XMLTAG_DISC_NUM_TRACKS_ON_MEDIA, m_disc_num_tracks)*/ \
	X(XMLTAG_TRACK_METADATA, m_tracks)

/// Strings to use for the tags.
#define X(field_tag, member_field) static constexpr strviw_type field_tag ( # member_field );
	M_SERDES_FIELDS_GENERAL(X);
	M_DATASTREAM_FIELDS_DISC(X);
	M_DATASTREAM_FIELDS_SPECIAL_HANDLING(X);
#undef X

// static
std::unique_ptr<CueSheet> CueSheet::make_unique_CueSheet(const QUrl& url, uint64_t total_length_in_ms)
{
	static QRegularExpression re("\\.[[:alnum:]]+$");

	auto retval = std::make_unique<CueSheet>();
    retval.reset();

    // Determine if we have a cue sheet in a sidecar *.cue file.

    // Create the *.cue URL.
    QUrl cue_url = url;
    QString cue_url_as_str = cue_url.toString();
    Q_ASSERT(!cue_url_as_str.isEmpty());
    cue_url_as_str.replace(re, ".cue");
    cue_url = cue_url_as_str;
    Q_ASSERT(cue_url.isValid());

    QFile cuefile(cue_url.toLocalFile());
	// Does the file exist?
	if (!cuefile.exists())
	{
		// File doesn't exist.
		return retval;
	}
	// Try to open it.
	bool status = cuefile.open(QIODevice::ReadOnly);
    if(!status && cuefile.exists())
    {
        qCr() << "Couldn't open cue file:" << cue_url << cuefile.errorString();
        return retval;
    }
    // Read the whole file.
    QByteArray ba = cuefile.readAll();
    if(ba.isEmpty())
    {
        qCr() << "Couldn't read cue file:" << cue_url;
        return retval;
    }

    std::string ba_as_stdstr(ba.cbegin(), ba.cend());

	retval = make_unique_CueSheet(ba_as_stdstr, total_length_in_ms);

    if(!retval)
    {
        qCr() << "Parsing sidecar cuesheet failed:" << cue_url;
    	qCr() << "CUESHEET STRING:" << ba_as_stdstr;
        return retval;
    }
	retval->set_origin(Sidecar);

    return retval;
}

// static
std::unique_ptr<CueSheet> CueSheet::make_unique_CueSheet(const std::string& cuesheet_text, uint64_t total_length_in_ms)
{
    auto retval = std::make_unique<CueSheet>();

    bool parsed_ok = retval->parse_cue_sheet_string(cuesheet_text, total_length_in_ms);

    if(!parsed_ok)
    {
        // Parsing failed, make sure we return an empty ptr.
        qCr() << "Cuesheet parsing failed";
        retval.reset();
    }

    return retval;
}

CueSheet::Origin CueSheet::origin() const
{
	return m_origin;
}

void CueSheet::set_origin(Origin origin)
{
	m_origin = origin;
}

std::map<int, TrackMetadata> CueSheet::get_track_map() const
{
	return m_tracks;
}

AMLMTagMap CueSheet::asAMLMTagMap_Disc() const
{
	AMLMTagMap retval;

	// Disc-level fields.
//#define X(field_tag, member_field) retval.insert(std::make_pair(tostdstr(field_tag), /*QVariant::fromValue*/member_field));
#define X(field_tag, member_field) AMLMTagMap_convert_and_insert(retval, tostdstr(field_tag), member_field);
    M_DATASTREAM_FIELDS_DISC(X)
#undef X
//	retval.insert(std::make_pair(tostdstr(XMLTAG_DISC_NUM_TRACKS_ON_MEDIA), std::to_string(m_disc_num_tracks)));

	return retval;
}

std::vector<AMLMTagMap> CueSheet::asAMLMTagMap_Tracks() const
{
	std::vector<AMLMTagMap> retval;

	Q_ASSERT(0);

	return retval;
}

uint8_t CueSheet::get_total_num_tracks() const
{
	return m_disc_num_tracks;
}

std::string CueSheet::get_album_title() const
{
	return m_disc_album_title;
}

QVariant CueSheet::toVariant() const
{
	InsertionOrderedMap<QString, QVariant> map;

    set_map_class_info(this, &map);

	// CD-level fields.
#define X(field_tag, member_field) map_insert_or_die(map, field_tag, member_field);
    M_SERDES_FIELDS_GENERAL(X)
    M_DATASTREAM_FIELDS_DISC(X)
#undef X

	// Track-level fields

	// Add the track list to the return value.
	QVariantHomogenousList qvar_track_list("m_tracks", "track");

	for(const auto& it : m_tracks)
	{
		TrackMetadata tm = it.second;
		list_push_back_or_die(qvar_track_list, tm);
	}

	// Warn if our num tracks don't match or don't make sense.
	AMLM_WARNIF(m_tracks.size() != m_disc_num_tracks && m_tracks.size() != 1);

	map_insert_or_die(map, XMLTAG_TRACK_METADATA, qvar_track_list);

	return map;
}

void CueSheet::fromVariant(const QVariant& variant)
{
	InsertionOrderedMap<QString, QVariant> map;
	qviomap_from_qvar_or_die(&map, variant);

	// CD-level fields.
#define X(field_tag, member_field) map_read_field_or_warn(map, field_tag, &(member_field));
    M_SERDES_FIELDS_GENERAL(X)
    M_DATASTREAM_FIELDS_DISC(X)
#undef X

	// Track-level fields
	QVariantHomogenousList qvar_track_list("m_tracks", "track");

	map_read_field_or_warn(map, XMLTAG_TRACK_METADATA, &qvar_track_list);
	// AMLM_WARNIF(qvar_track_list.empty());
#if 0
	list_read_all_fields_or_warn(qvar_track_list, &m_tracks);
#else
	for(const auto& track : std::as_const(qvar_track_list))
	{
		TrackMetadata tm;
		tm.fromVariant(track);

		// Should have a track number.
		throwif(tm.m_track_number == 0);
		int track_num = tm.m_track_number;

		m_tracks.insert(std::make_pair(track_num, tm));
	}
#endif
}

static std::string tostdstr(enum DiscMode disc_mode)
{
	switch(disc_mode)
	{
	case MODE_CD_DA:  /* CD-DA */
		return "CD-DA";
		break;
	case MODE_CD_ROM: /* CD-ROM mode 1 */
		return "CD-ROM mode 1";
		break;
	case MODE_CD_ROM_XA:  /* CD-ROM XA and CD-I */
		return "CD-ROM XA or CD-I";
		break;
	default:
		return "UNKNOWN";
		break;
	}
}

static std::string LibCueHelper_cd_get_catalog(struct Cd *cd)
{
	struct DummyCd
	{
		int mode;
		const char* catalog;
	};

	std::string retval {};

	/// @todo This is gross, pretend you don't see this.
	/// @todo There is no libcue API through which to read this member, so this.
	/// We should probably convert over to the libcue in cuetools here:
	/// @link https://github.com/knight-rider/cuetools
	/// That does have an accessor and last activity was Nov 2018.

	const DummyCd* dummy_cd = (const DummyCd*)cd;

	const char* catalog_cstr = dummy_cd->catalog;
	if(catalog_cstr != nullptr)
	{
		retval = catalog_cstr;
	}

	return retval;
}

bool CueSheet::parse_cue_sheet_string(const std::string& cuesheet_text, uint64_t length_in_ms)
{
	// Mutex FBO libcue.  Libcue isn't thread-safe.
	std::lock_guard<std::mutex> lock(m_libcue_mutex);

	// libcue (actually flex) can't handle invalid UTF-8.
	// if(!QtPrivate::isValidUtf8(cuesheet_text.c_str())) ///< Some invalid
	QString valid = QString::fromUtf8(cuesheet_text); ///< None invalid
	// auto toUtf8 = QStringDecoder(QStringDecoder::Utf8);   ///< Some invalid
    // QString valid = toUtf8(cuesheet_text);
    if(valid.isNull())
    {
        qCr() << "Invalid UTF-8 cuesheet string:";
        qCr() << cuesheet_text.c_str();
        // Q_ASSERT(0);
        return false;
    }

	// Final adjustment of the string to compensate for some variations seen in the wild.
	std::string final_cuesheet_string = preprocess_cuesheet_string(cuesheet_text);

	// Try to parse the cue sheet we found with libcue.
	Cd* cd = cue_parse_string(final_cuesheet_string.c_str());

	Q_ASSERT_X(cd != nullptr, __PRETTY_FUNCTION__, "failed to parse cuesheet string");

    // NEED THE TOTAL LENGTH FOR LAST TRACK LENGTH.
    m_length_in_milliseconds = length_in_ms;

    if(cd == nullptr)
    {
        qWr() << "Cue sheet parsing failed.";
        return false;
    }
    else
    {
        // Libcue parsed the cuesheet text, let's extract what we need.

		// qDb() << "CD_DUMP: ++++++++++++++++++++++++++++++++++++";
		// cd_dump(cd);
  //   	qDb() << "CD_REM DUMP:";
  //   	auto* rem = cd_get_rem(cd);
		// rem_dump(rem);

		//
		// Get disc-level info from the Cd struct.
		//

		// Not a lot of interest there, except the Catalog number and the CD-TEXT.
		m_disc_catalog_num = LibCueHelper_cd_get_catalog(cd);
    	/// @todo Should save this.
		enum DiscMode disc_mode = cd_get_mode(cd);
		qDb() << "Disc Mode:" << toqstr(tostdstr(disc_mode));

		// Get the disc-level CD-TEXT.
	    Cdtext* cdtext = cd_get_cdtext(cd);
		if(cdtext_is_empty(cdtext) == 0)
		{
			qWr() << "No CDTEXT";
		}
		else
		{
			qDb() << "CDTEXT_DUMP (disc):";
			cdtext_dump(cdtext, 0);
		}
	    AMLM_WARNIF(cdtext == nullptr);
		if(cdtext != nullptr)
		{
			auto* disc_id_cstr = cdtext_get(PTI_DISC_ID, cdtext);
			if(disc_id_cstr == nullptr)
			{
				qWr() << "No Cuesheet CD-Text DiscID";
			}
			else
			{
				m_disc_id = disc_id_cstr;
				qDb() << "##################### REM DISC_ID:" << m_disc_id;
			}
			auto* disc_album_title = cdtext_get(PTI_TITLE, cdtext);
			if (disc_album_title == nullptr)
			{
				qWr() << "No Title";
			}
			else
			{
				m_disc_album_title = disc_album_title;
				qDb() << "##################### REM TITLE:" << m_disc_album_title;
			}
			auto* disc_album_performer = cdtext_get(PTI_PERFORMER, cdtext);
			if (disc_album_performer == nullptr)
			{
				qWr() << "No PTI_PERFORMER";
			}
			else
			{
				m_disc_album_performer = disc_album_performer;
			}
		}

		// Get the Cue Sheet's CD-level REM contents.
		Rem* cdrem = cd_get_rem(cd);
		// Pretty much just Date.
	    m_disc_date = tostdstr(rem_get(REM_DATE, cdrem));

	    // Get the number of tracks on the media.
        m_disc_num_tracks = cd_get_ntrack(cd);
		qDb() << "Num Tracks:" << m_disc_num_tracks;

        if(m_disc_num_tracks < 2)
        {
            qWr() << "Num tracks is less than 2:" << m_disc_num_tracks;
        }

		//
		// Per-Track metadata.
        // Iterate over each track and get any info we can.
		//
        for(int track_num=1; track_num < m_disc_num_tracks+1; ++track_num)
        {
			Track* track_ptr = cd_get_track(cd, track_num);

			TrackMetadata tm;

			// Have the TrackMetadata class assemble itself from the cue sheet track_ptr data.
			/// @todo Make use of the unique_ptr<> returned here.
			tm = *TrackMetadata::make_unique_track_metadata(track_ptr, track_num);

            if(tm.m_length_frames < 0)
            {
                // This is the last track.  We have to calculate the length from the total recording time minus the start offset.
                Q_ASSERT(m_length_in_milliseconds > 0);
                tm.m_length_frames = (75.0*double(m_length_in_milliseconds)/1000.0) - tm.m_start_frames;
            }

			// Using .insert() here to detect duplicate track numbers, which shouldn't ever exist per cue sheet specs.
            auto insert_status = m_tracks.insert({track_num, tm});
            if(insert_status.second != true)
            {
                // Found a dup.
            	/// @todo This means something is really broken, should handle this better than just a message.
                qCr() << "DUPLICATE CUESHEET TRACK ENTRIES:" << track_num;/// << tm << *insert_status.first;
            }
        }

        // Delete the Cd struct.
        // All the other libcue structs we've opened are deleted with it.
        cd_delete(cd);

        // Succeeded.
        return true;
	}
}

std::string CueSheet::preprocess_cuesheet_string(const std::string& cuesheet_text) const
{
	std::string retval;

	// The best I can figure is that the "REM DISCID nnnnn"'s I'm seeing in a lot of cue sheets is actually
	// 86h "Disc Identification information" as defined by MMC-3/CD-TEXT.  Trouble is, there's no definition there,
	// or anywhere else I can find.  A survey of the cuesheets I have all have the number as a 32-bit hex value.
	std::regex s_REM_DISCID(R"!(^REM\sDISCID)!", std::regex_constants::ECMAScript);

	retval = std::regex_replace(cuesheet_text, s_REM_DISCID, "DISC_ID");

	// Remove any \r's.  libcue can only handle \n newlines.
    std::erase(retval, '\r');

	// Remove any trailing '\n's to avoid this bug in libcue: https://github.com/lipnitsk/libcue/issues/52
	if(retval.ends_with('\n'))
	{
		retval = retval.substr(0, retval.size() - 1);
	}

	return retval;
}


bool operator==(const CueSheet& lhs, const CueSheet& rhs)
{
	/// @todo Make this a comprehensive comparison, maybe switch to <=> operator.
	return lhs.m_origin == rhs.m_origin;
}

QDebug operator<<(QDebug dbg, const CueSheet &cuesheet)
{
    QDebugStateSaver saver(dbg);

    dbg << "CueSheet(";
    dbg << "{\n";
	for(const auto& i : cuesheet.m_tracks)
    {
        dbg << "KEY:" << i.first << "VALUE:" << i.second << "\n";
    }
    dbg << "}\n";
    dbg << ")\n";

    return dbg;
}

QDataStream &operator<<(QDataStream &out, const CueSheet &myObj)
{
    out << myObj.toVariant();
    return out;
}

QDataStream &operator>>(QDataStream &in, CueSheet &myObj)
{
    QVariant var;
    in >> var;
    myObj.fromVariant(var);
    return in;
}
