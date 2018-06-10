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

/// Qt5
#include <QRegularExpression>
#include <QUrl>

/// KF5
#include <KF5/KIOCore/KFileItem>

/// Libcue
#include <libcue.h>

/// Ours, Qt5/KF5-related
#include <utils/TheSimplestThings.h>
#include <utils/RegisterQtMetatypes.h>

/// Ours
#include "TrackMetadata.h"  ///< Per-track cue sheet info


std::mutex CueSheet::m_libcue_mutex;

AMLM_QREG_CALLBACK([](){
    qRegisterMetaType<CueSheet>()
    ;});
//static int dummy = (RegisterQTRegCallback([](){qDb() << "HELLO";}), 0);

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
//    {
//        qWr() << "URL has no filename:" << cue_url;
//    }

//    auto kfileitem = new KFileItem(cue_url /*, mimetype = unknown, mode=unknown */);

//    Q_ASSERT(kfileitem);
//    qIn() << "URL Info:" << cue_url;
//    qIn() << "MIME type:" << kfileitem->mimetype();
//    qIn() << "Local?:" << kfileitem->isLocalFile();
//    qIn() << "mostLocalUrl:" << kfileitem->mostLocalUrl();

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

bool CueSheet::parse_cue_sheet_string(const std::string &cuesheet_text, uint64_t length_in_ms)
{
	// Mutex FBO libcue.  Libcue isn't thread-safe.
	std::lock_guard<std::mutex> lock(m_libcue_mutex);

	// libcue (actually flex) can't handle invalid UTF-8.
    Q_ASSERT_X(isValidUTF8(cuesheet_text.c_str()), __func__, "Invalid UTF-8 cuesheet string.");

	// Try to parse the cue sheet we found with libcue.
	Cd* cd = cue_parse_string(cuesheet_text.c_str());

	Q_ASSERT_X(cd != nullptr, __PRETTY_FUNCTION__, "failed to parse cuesheet string");

M_WARNING("TEMP: NEED THE TOTAL LENGTH FOR LAST TRACK LENGTH.");
    /// @todo NEED THE TOTAL LENGTH FOR LAST TRACK LENGTH.
    uint64_t m_length_in_milliseconds = length_in_ms;

    if(cd == nullptr)
    {
        qWr() << "Embedded cue sheet parsing failed.";
        return false;
    }
    else
    {
        // Libcue parsed it, let's extract what we need.

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
            Cdtext* cdt = track_get_cdtext(t);
            TrackMetadata tm;
            tm.m_PTI_TITLE = tostdstr(cdtext_get(PTI_TITLE, cdt));
            tm.m_PTI_PERFORMER = tostdstr(cdtext_get(PTI_PERFORMER, cdt));
            ///@todo There's more we could get here.

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

    dbg << "CueSheet";

    for(auto i : cuesheet.m_tracks)
    {
        dbg << i.first << i.second;
    }

    return dbg;
}
