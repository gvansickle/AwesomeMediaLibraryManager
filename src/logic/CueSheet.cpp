/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include <config.h>

#include "CueSheet.h"

// Qt5
#include <QRegularExpression>
#include <QUrl>

// KF5
//#include <KF5/KIOCore/KFileItem>

/// @todo Looks like VS2017 headers are broken here.  libcue.h includes <stdio.h> outside the extern "C",
/// and apparently MS's stdio.h isn't C++-safe.
extern "C" {
// Libcue
#include <libcue/libcue.h>
#include <libcue/cd.h>
}

/// Ours, Qt5/KF5-related
#include <utils/TheSimplestThings.h>
#include <utils/RegisterQtMetatypes.h>

/// Ours
#include "TrackMetadata.h"  ///< Per-track cue sheet info



/**
 * Same disc ripped with two different rippers:
 *
 * Bad embedded cue sheet:
 *
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
 *
 *
 * Good embedded cue sheet:
 *
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
 *
 */

std::mutex CueSheet::m_libcue_mutex;

AMLM_QREG_CALLBACK([](){
	qIn() << "Registering CueSheet";
    qRegisterMetaType<CueSheet>()
    ;});

CueSheet::CueSheet()
{

}

CueSheet::~CueSheet()
{
}

std::unique_ptr<CueSheet> CueSheet::read_associated_cuesheet(const QUrl &url, uint64_t total_length_in_ms)
{
    auto retval = std::make_unique<CueSheet>();
    retval.reset();

    // Determine if we have a cue sheet embedded in the given file's metadata,
    // or if we have an associated *.cue file, or neither.

    // Create the *.cue URL.
    QUrl cue_url = url;
    QString cue_url_as_str = cue_url.toString();
    Q_ASSERT(!cue_url_as_str.isEmpty());
    cue_url_as_str.replace(QRegularExpression("\\.[[:alnum:]]+$"), ".cue");
    cue_url = cue_url_as_str;
    Q_ASSERT(cue_url.isValid());

    // Try to open it.
    QFile cuefile(cue_url.toLocalFile());
    bool status = cuefile.open(QIODevice::ReadOnly);
    if(!status)
    {
        qDb() << "Couldn't open cue file:" << cue_url;
        return retval;
    }
    // Read the whole file.
    QByteArray ba = cuefile.readAll();
    if(ba.isEmpty())
    {
        qDb() << "Couldn't read cue file:" << cue_url;
        return retval;
    }

    std::string ba_as_stdstr(ba.cbegin(), ba.cend());

    retval = TEMP_parse_cue_sheet_string(ba_as_stdstr, total_length_in_ms);

    return retval;
}

std::unique_ptr<CueSheet> CueSheet::TEMP_parse_cue_sheet_string(const std::string &cuesheet_text, uint64_t total_length_in_ms)
{
    auto retval = std::make_unique<CueSheet>();

    bool parsed_ok = retval->parse_cue_sheet_string(cuesheet_text, total_length_in_ms);

    if(!parsed_ok)
    {
        // Parsing failed, make sure we return an empty ptr.
//        return std::make_unique<CueSheet>();
        /// @todo This seems pretty inefficient.
        retval.reset();
    }

    return retval;
}

std::map<int, TrackMetadata> CueSheet::get_track_map() const
{
    return m_tracks;
}

uint8_t CueSheet::get_total_num_tracks() const
{
    return m_num_tracks_on_media;
}

std::string tostdstr(enum DiscMode disc_mode)
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

bool CueSheet::parse_cue_sheet_string(const std::string &cuesheet_text, uint64_t length_in_ms)
{
	// Mutex FBO libcue.  Libcue isn't thread-safe.
	std::lock_guard<std::mutex> lock(m_libcue_mutex);

	// libcue (actually flex) can't handle invalid UTF-8.
    Q_ASSERT_X(isValidUTF8(cuesheet_text.c_str()), __func__, "Invalid UTF-8 cuesheet string.");

	// Try to parse the cue sheet we found with libcue.
	Cd* cd = cue_parse_string(cuesheet_text.c_str());

	Q_ASSERT_X(cd != nullptr, __PRETTY_FUNCTION__, "failed to parse cuesheet string");

    // NEED THE TOTAL LENGTH FOR LAST TRACK LENGTH.
    m_length_in_milliseconds = length_in_ms;

    if(cd == nullptr)
    {
        qWr() << "Embedded cue sheet parsing failed.";
        return false;
    }
    else
    {
        // Libcue parsed the cuesheet text, let's extract what we need.
//		qDb() << "CD_DUMP:";
//		cd_dump(cd);
		m_disc_catalog = LibCueHelper_cd_get_catalog(cd);
		enum DiscMode disc_mode = cd_get_mode(cd);
		qDb() << "Disc Mode:" << toqstr(tostdstr(disc_mode));
		// Get the Cue Sheet's REM contents.
		Rem* cdrem = cd_get_rem(cd);
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
		auto* disc_id_cstr = cdtext_get(PTI_DISC_ID, cdtext);
		std::string disc_id {};
		if(disc_id_cstr == nullptr)
		{
			qWr() << "No DiscID";
		}
		else
		{
			/// @note Never seem to get here.
			disc_id = disc_id_cstr;
		}
	    qDb() << M_ID_VAL(disc_id);

        m_num_tracks_on_media = cd_get_ntrack(cd);
        qDebug() << "Num Tracks:" << m_num_tracks_on_media;

        if(m_num_tracks_on_media < 2)
        {
            qWr() << "Num tracks is less than 2:" << m_num_tracks_on_media;
        }
        for(int track_num=1; track_num < m_num_tracks_on_media+1; ++track_num)
        {
            Track* t = cd_get_track(cd, track_num);
//            qDebug() << "Track filename:" << track_get_filename(t);
            // Get the CD-Text info.
            Cdtext* cdt = track_get_cdtext(t);

            TrackMetadata tm;
#if 0
#warning "FIXING"
            // get the Pack Type Indicator data.
#define X(id) tm.m_ ## id = tostdstr(cdtext_get( id , cdt ));
            PTI_STR_LIST(X)
#undef X
#else
			tm = *TrackMetadata::make_track_metadata(cdt);
#endif
            for(auto i = 0; i<99; ++i)
            {
                //qDebug() << "Reading track index:" << i;
                long ti = track_get_index(t, i);
                tm.m_indexes.push_back(ti);
                if((ti==-1) && (i>1))
                {
                    qDb() << "Found last index: " << i-1;
                    break;
                }
                else
                {
                    qDb() << " Index:" << ti;
                }
            }

            tm.m_track_number = track_num;
            tm.m_start_frames = track_get_start(t);
            tm.m_length_frames = track_get_length(t);
            if(tm.m_length_frames < 0)
            {
                // This is the last track.  We have to calculate the length from the total recording time minus the start offset.
                Q_ASSERT(m_length_in_milliseconds > 0);
                tm.m_length_frames = (75.0*double(m_length_in_milliseconds)/1000.0) - tm.m_start_frames;
            }
            tm.m_length_pre_gap = track_get_zero_pre(t);
            tm.m_length_post_gap = track_get_zero_post(t);
            tm.m_isrc = tostdstr(track_get_isrc(t));
//            qDb() << "Track info:" << tm.toStdString();
            // Using .insert() here to detect duplicate track numbers, which shouldn't ever exist per cue sheet specs.
            auto insert_status = m_tracks.insert({track_num, tm});
            if(insert_status.second != true)
            {
                // No insertion took place, must have been a dup.
                qWr() << "DUPLICATE CUESHEET TRACK ENTRIES:" << track_num;/// << tm << *insert_status.first;
            }
        }

        // Delete the Cd struct.
        // All the other libcue structs we've opened are deleted with it.
        cd_delete(cd);

        // Succeeded.
        return true;
    }
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
