/*
 * Copyright 2017, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

// Stc C++
#include <string>
#include <vector>

// Qt5
#include <QVariant>

// Libcue.
struct Cdtext;

// Ours.
//#include "Frames.h"
using Frames = qint64;
#include <logic/serialization/ISerializable.h>
#include <future/guideline_helpers.h>
#include <third_party/libcue/libcue.h>
#include "AMLMTagMap.h"

#include "TrackIndex.h"



/**
 * Metadata which applies to a single track on a possibly multi-track media.
 */
class TrackMetadata : public virtual ISerializable
{
public:
	M_GH_RULE_OF_FIVE_DEFAULT_C21(TrackMetadata);
	~TrackMetadata() override = default;

	std::string toStdString() const;

	static std::unique_ptr<TrackMetadata> make_track_metadata(const Track* track_ptr, int track_number);

	/// @name Serialization
	/// @{
	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;
	/// @}


	qint64 m_track_number {0};

	/// Length (? or offset from the beginning of the file?) of the pre-audio gap, in frames.
    Frames m_length_pre_gap {0};
	/// Start of the audio from the beginning of the file, in frames.
	/// Exactly the value returned by Libcue track_get_start().
	/// Corresponds to INDEX 01.
	Frames m_start_frames {0};
	/// Length of the audio from the beginning of the file, in frames.
	/// Exactly the value returned by track_get_length().
	Frames m_length_frames {0};
	/// Length (? or offset from the beginning of the file?) of the post-audio gap, in frames.
	Frames m_length_post_gap {0};

	/// CD-Text "pack type indicators".
	///
	/// @link https://www.gnu.org/software/libcdio/cd-text-format.html
	/// @link https://github.com/lipnitsk/libcue/blob/master/libcue.h
	/// @link https://en.wikipedia.org/wiki/CD-Text
	///
	/// These are the character types, applicable to both Disc and Track sections unless otherwise noted.
	/// See below for binary types.
	/// These text pack types contain a NUL-termintated string.
	/// PTI_UPC_ISRC (0x8e) for the full disk is "is documented by Sony as: UPC/EAN Code (POS Code) of the album.
	///   This field typically consists of 13 characters."  Per the Gnu link above, always ASCII.
	/// For tracks, "ISRC code [which] typically consists of 12 characters" and is always ISO-8859-1 encoded.".
	/// Also per the Gnu link: "MMC calls these information entities “Media Catalog Number” and “ISRC”. The catalog
	///   number consists of 13 decimal digits. ISRC consists of 12 characters: 2 country code [0-9A-Z], 3 owner code [0-9A-Z],
	///   2 year digits (00 to 99), 5 serial number digits (00000 to 99999)."
#define PTI_STR_LIST(X) \
    X(PTI_TITLE) \
    X(PTI_PERFORMER) \
    X(PTI_SONGWRITER) \
    X(PTI_COMPOSER) \
    X(PTI_ARRANGER) \
    X(PTI_MESSAGE) \
	/* PTI_DISC_ID == binary disc identification info.*/ \
	/* PTI_GENRE == binary genre.*/ \
	/* PTI_TOC_INFO1 == binary TOC info. */ \
	/* PTI_TOC_INFO2 == binary second TOC info. */ \
	/* 4 reserved types. */ \
	X(PTI_UPC_ISRC) /* == 0x8e, Double-duty here:
	Per disc == UPC_EAN - UPC/EAN code of the album,
	Per track == ISRC - ISRC Code of each track */

#define PTI_BIN_LIST(X) \
	X(PTI_DISC_ID) /* 0x86 == Disc, per Gnu above: "For pack type 0x86 (Disc Identification) here is how Sony describes this:
						"Catalog Number: (use ASCII Code) Catalog Number of the album"
						So it is not really binary but might be non-printable, and should contain only bytes with
						bit 7 set to zero." */ \
	X(PTI_GENRE) /* 0x87 == Disc, Genre.  Per Gnu link above:
                    "Pack type 0x87 (Genre Identification) contains 2 binary bytes followed by NUL-byte terminated text.
						You can either specify a genre code or the supplementary genre information (without the code)
						or both. Neither is mandatory.

Categories associated with their Big-endian 16-bit value are:

  0x0000: Not Used. Sony prescribes this when no genre applies
  0x0001: Not Defined
  0x0002: Adult Contemporary
  0x0003: Alternative Rock
  0x0004: Childrens' Music
  0x0005: Classical
  0x0006: Contemporary Christian
  0x0007: Country
  0x0008: Dance
  0x0009: Easy Listening
  0x000a: Erotic
  0x000b: Folk
  0x000c: Gospel
  0x000d: Hip Hop
  0x000e: Jazz
  0x000f: Latin
  0x0010: Musical
  0x0011: New Age
  0x0012: Opera
  0x0013: Operetta
  0x0014: Pop Music
  0x0015: Rap
  0x0016: Reggae
  0x0017: Rock Music
  0x0018: Rhythm & Blues
  0x0019: Sound Effects
  0x001a: Spoken Word
  0x001b: World Music

Sony documents report that this field contains:

    Genre information that would supplement the Genre Code, such as “USA Rock music in the 60’s”.

This information is always ASCII encoded. */ \
	X(PTI_TOC_INFO1) /* 0x88 == Disc */  \
	X(PTI_TOC_INFO2) /* 0x89 == Disc */ \
    X(PTI_SIZE_INFO)

    /// @name Declaration of all CD-TEXT string "pack type indicators".
    /// @{
#define X(id) std::string m_ ## id {};
    PTI_STR_LIST(X)
#undef X
    /// @}
//#undef PTI_STR_LIST

    /// @name Declaration of all CD-TEXT binary "pack type indicators".
    /// @todo What type should these really be?
    /// @{
#define X(id) std::string m_ ## id {};
    PTI_BIN_LIST(X)
#undef X
    /// @}
//#undef PTI_BIN_LIST

	/// The track's Pack Type Indicator info as an AMLMTagMap.
	AMLMTagMap m_tm_track_pti;

	/// Track ISRC code.  May be empty.
	std::string m_isrc;

	/// Track filename from cuesheet.
	std::string m_track_filename;

	/// Indexes for this track.
	std::vector<TrackIndex> m_indexes;

	/// Derived info.
	bool m_is_part_of_gapless_set {false};

	Frames last_frame() const { return m_start_frames+m_length_frames; }

    friend QDebug operator<<(QDebug dbg, const TrackMetadata &tm);
};

QDebug operator<<(QDebug dbg, const TrackMetadata& tm);

Q_DECLARE_METATYPE(TrackMetadata);


#endif // TRACKMETADATA_H
