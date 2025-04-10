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

/**
 * @file CueSheet
 */

#ifndef SRC_LOGIC_CUESHEET_H_
#define SRC_LOGIC_CUESHEET_H_

#include <config.h>

// Std C++
#include <vector>
#include <memory>
#include <cstdint>
#include <map>
#include <mutex>

// Qt
class QUrl;
#include <QDataStream>

// Ours
#include "TrackMetadata.h"  ///< Per-track cue sheet info
#include <future/guideline_helpers.h>
#include "CueSheetParser.h"
#include <logic/serialization/ISerializable.h>
#include <logic/AMLMTagMap.h>


/**
 * CD cue sheet class.
 * C++ class for abstracing cuesheet info, mostly digested via libcue.
 * @link http://wiki.hydrogenaud.io/index.php?title=Cue_sheet
 * "Cue sheet contents
 *
 * All cue sheets contain the following info:
 *
 * - The name & type of at least one file being indexed (an audio file, normally);
 * - A numbered list of tracks each file corresponds to or contains;
 * - The start point (index 01) for each track, time-wise (MM:SS:FF format).
 *
 * Cue sheets may contain the following additional info:
 *
 * - CD-Text metadata such as performer, title, songwriter for the disc and/or each track;
 * - ISRCs (sound recording IDs to burn)
 * - Special flags for CD burning (e.g. for pre-emphasis)
 * - Gap info (how much silence to insert before or after each track)
 * - Comments (which are used by some programs to store nonstandard metadata like genre, freeDB disc ID, etc.)"
 *
 * @link https://en.wikipedia.org/wiki/Cue_sheet_(computing)
 */
class CueSheet : public virtual ISerializable
{
public:
    M_GH_RULE_OF_FIVE_DEFAULT_C21(CueSheet)
    ~CueSheet() override = default;

    /**
     * Factory function.
     * Given a URL to an audio file, read the cue sheet either from the metadata in
     * the file itself or from a *.cue file in the same directory.
     */
    static std::shared_ptr<CueSheet> read_associated_cuesheet(const QUrl& url, uint64_t total_length_in_ms);

    static std::shared_ptr<CueSheet> TEMP_parse_cue_sheet_string(const std::string& cuesheet_text, uint64_t total_length_in_ms = 0);

    /**
     * Returns the parsed TrackMetadata entries as a std::map.
     */
    std::map<int, TrackMetadata> get_track_map() const;

	/**
	 * Returns the cuesheet info related to the entire disc as an AMLMTagMap.
	 */
	AMLMTagMap asAMLMTagMap_Disc() const;

	/**
	 * Returns the cuesheet info related to the tracks as an AMLMTagMap.
	 */
	std::vector<AMLMTagMap> asAMLMTagMap_Tracks() const;

    /// @name Accessors
    /// @{

    /// Return the total number of tracks reported by this cuesheet.
    uint8_t get_total_num_tracks() const;

    /// @}


	/// @name Serialization
	/// @{
	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;
	/// @}

protected:

    /**
     * Populate the data of this CueSheet by parsing the given cuesheet_text.
     *
     * @return true if parsing succeeded, false otherwise.
     */
    bool parse_cue_sheet_string(const std::string& cuesheet_text, uint64_t total_length_in_ms = 0);

	std::string prep_final_cuesheet_string(const std::string& cuesheet_text) const;

    friend QDebug operator<<(QDebug dbg, const CueSheet &cuesheet);

private:
	/// Mutex for serializing access to libcue.  Libcue isn't thread-safe.
	static std::mutex m_libcue_mutex;

	/// @name File info
    /// @todo More that one file per sheet?

    /// @name Mandatory Info
    /// @{

	/// "CATALOG": The Sony UPC/EAN code /AKA MMC-3 Media Catalog Number of the disc.
	/// @link https://www.gnu.org/software/libcdio/cd-text-format.html#Text-Pack-Types
	/// Mandatory 13 digits long.  This is always ASCII encoded.
	std::string m_disc_catalog_num {};

	/// CD-TEXT Pack type 0x86, "Disc Identification".  Disc, not Track.
	/// @link https://www.gnu.org/software/libcdio/cd-text-format.html#Misc-Pack-Types
	/// "here is how Sony describes this:
	///	  Catalog Number: (use ASCII Code) Catalog Number of the album
	/// So it is not really binary but might be non-printable, and should contain only bytes with bit 7 set to zero."
	/// This is not the same as the CD-TEXT "CATALOG" number, but exactly what it is is not clear.
	/// Also, this shows up in cue sheets as "REM DISCID nnnnnnnn", note the lack of a "_".  Surveys of
	/// my collection show the value to always be a 32-bit hex string.
	std::string m_disc_id {};

	/// @todo: REM DISCNUMBER, TOTALDISCS, DATE
	///
	///
	std::string m_disc_date {};

	uint64_t m_length_in_milliseconds {0};

    /// @}

    /**
     * Total number of tracks.
     * @see https://xiph.org/flac/format.html#metadata_block_cuesheet
     * ""
     */
	std::int32_t m_num_tracks_on_media {0};

    /**
     * Cue sheet-derived information on each track.
	 * @note Map key is the track number, 1 to 99, except see below.
     * @ref https://xiph.org/flac/format.html#cuesheet_track
     * "Track number.
     * - A track number of 0 is not allowed to avoid conflicting with the CD-DA spec, which reserves this for the lead-in.
     * - For CD-DA the number must be 1-99, or 170 for the lead-out;
     * - for non-CD-DA, the track number must for 255 for the lead-out.
     * - It is not required but encouraged to start with track 1 and increase sequentially.
     * - Track numbers must be unique within a CUESHEET."
     */
    std::map<int, TrackMetadata> m_tracks;
};


QDebug operator<<(QDebug dbg, const CueSheet &cuesheet);

QDataStream &operator<<(QDataStream &out, const CueSheet &myObj);
QDataStream &operator>>(QDataStream &in, CueSheet &myObj);

Q_DECLARE_METATYPE(CueSheet);


#endif /* SRC_LOGIC_CUESHEET_H_ */
