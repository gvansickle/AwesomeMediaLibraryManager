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

#ifndef TRACKMETADATA_H
#define TRACKMETADATA_H

/// Stc C++
#include <string>
#include <vector>

/// Qt5
#include <QtCore>

// Cue Sheet Frame == 1/75th of a second.
using Frames = long;

/**
 * Metadata which applies to a single track in a possibly multi-track media.
 */
class TrackMetadata
{
public:
	TrackMetadata();

	std::string toStdString() const;

	long m_track_number {0};

    Frames m_length_pre_gap {0};
	Frames m_start_frames {0};
	Frames m_length_frames {0};
	Frames m_length_post_gap {0};

	/// cdtext "pack type indicators".
#define PTI_STR_LIST \
    X(PTI_TITLE) \
    X(PTI_PERFORMER) \
    X(PTI_SONGWRITER) \
    X(PTI_COMPOSER) \
    X(PTI_ARRANGER) \
    X(PTI_MESSAGE) \
    X(PTI_UPC_ISRC)

#define PTI_BIN_LIST \
    X(PTI_DISC_ID) \
    X(PTI_GENRE) \
    X(PTI_TOC_INFO1) \
    X(PTI_TOC_INFO2) \
    X(PTI_SIZE_INFO)

#define X(id) std::string m_ ## id ;
    PTI_STR_LIST
#undef X

	// PTI_DISC_ID == binary disc identification info.
	// PTI_GENRE == binary genre.

	/// ISRC code.  May be empty.
	std::string m_isrc;

	/// Indexes.
	/// -1 means there was no such index in the cue sheet.
	std::vector<long> m_indexes;

	/// Derived info.
	bool m_is_part_of_gapless_set {false};

	Frames last_frame() const { return m_start_frames+m_length_frames; }

    friend QDebug operator<<(QDebug dbg, const TrackMetadata &tm);
};

Q_DECLARE_METATYPE(TrackMetadata);

QDebug operator<<(QDebug dbg, const TrackMetadata &tm);

#endif // TRACKMETADATA_H
