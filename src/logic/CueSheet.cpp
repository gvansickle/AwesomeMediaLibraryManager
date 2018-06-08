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
#include <QUrl>

/// Ours, Qt5/KF5-related
#include <utils/TheSimplestThings.h>

/// Ours
#include "TrackMetadata.h"  ///< Per-track cue sheet info
#include "CueSheetParser.h"


CueSheet::CueSheet()
{

}

CueSheet::~CueSheet()
{
}

std::unique_ptr<CueSheet> CueSheet::read_associated_cuesheet(const QUrl &url)
{
    auto retval = std::make_unique<CueSheet>();

    // Determine if we have a cue sheet embedded in the given file's metadata,
    // or if we have an associated *.cue file, or neither.


    return retval;
}

bool CueSheet::parse_cue_sheet_string(const std::string &cuesheet_text, uint64_t length_in_ms)
{
    // Try to parse the cue sheet we found with libcue.
    CueSheetParser csp;

M_WARNING("TEMP: NEED THE TOTAL LENGTH FOR LAST TRACK LENGTH.");
    /// @todo NEED THE TOTAL LENGTH FOR LAST TRACK LENGTH.
    uint64_t m_length_in_milliseconds = length_in_ms;

    Cd *cd = csp.parse_cue_sheet_string(cuesheet_text.c_str());
    if(cd == nullptr)
    {
        qWr() << "Embedded cue sheet parsing failed.";
        return false;
    }
    else
    {
        // Libcue parsed it, let's extract what we need.

        m_num_tracks_on_media = cd_get_ntrack(cd);
        //qDebug() << "Num Tracks:" << m_num_tracks;
        if(m_num_tracks_on_media < 2)
        {
            qWarning() << "Num tracks is less than 2:" << m_num_tracks_on_media;
        }
        for(int track_num=1; track_num < m_num_tracks_on_media+1; ++track_num)
        {
            Track* t = cd_get_track(cd, track_num);
            //qDebug() << "Track filename:" << track_get_filename(t);
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
                    qDebug() << "Found last index: " << i-1;
                    break;
                }
                else
                {
                    //qDebug() << " Index:" << ti;
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
            //qDebug() << "Track info:" << tm.toStdString();
            m_tracks[track_num] = tm;
        }

        // Delete the Cd struct.
        // All the other libcue structs we've opened are deleted with it.
        cd_delete(cd);

        // Succeeded.
        return true;
    }
}

