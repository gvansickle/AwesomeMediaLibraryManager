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

#ifndef SRC_LOGIC_CUESHEET_H_
#define SRC_LOGIC_CUESHEET_H_

#include <config.h>

/// Std C++
#include <vector>
#include <memory>
#include <cstdint>

/// Qt5
class QUrl;

//#include "TrackMetadata.h"  ///< Per-track cue sheet info
class TrackMetadata;

/**
 * CD cue sheet class.
 * @link http://wiki.hydrogenaud.io/index.php?title=Cue_sheet
 */
class CueSheet
{
public:
	CueSheet();
	virtual ~CueSheet();

    /**
     * Factory function.
     * Given a URL to an audio file, read the cue sheet either from the metadata in
     * the file itself or from a *.cue file in the same directory.
     */
	static std::unique_ptr<CueSheet> read_associated_cuesheet(const QUrl& url);

protected:

    /**
     * Populate the data of this CueSheet by parsing the given cuesheet_text.
     *
     * @return true if parsing succeeded, false otherwise.
     */
    bool parse_cue_sheet_string(const std::string& cuesheet_text, uint64_t total_length_in_ms = 0);

private:

    // File info
    /// @todo More that one file per sheet?

    /// @name Mandatory Info
    /// @{

    /**
     * "Name and type of at least one file being indexed"
     */
    /// @todo


    /// @}

    /**
     * Total number of tracks.
     * @see https://xiph.org/flac/format.html#metadata_block_cuesheet
     * ""
     */
    uint8_t m_num_tracks_on_media;

    /**
     * Cue sheet information on each track.
     */
    std::vector<TrackMetadata> m_tracks;

};

#endif /* SRC_LOGIC_CUESHEET_H_ */
