/*
 * Copyright 2017. 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
#include <QtCore>

// Libcue.
struct Cdtext;

// Ours.
//#include "Frames.h"
using Frames = qint64;
#include <logic/serialization/ISerializable.h>
#include <future/guideline_helpers.h>


/**
 * Metadata which applies to a single track in a possibly multi-track media.
 */
class TrackMetadata : public ISerializable
{
//    Q_GADGET

public:
	M_GH_RULE_OF_FIVE_DEFAULT_C21(TrackMetadata);

	std::string toStdString() const;

	static std::unique_ptr<TrackMetadata> make_track_metadata(const Cdtext* cdtext);

	/// @name Serialization
	/// @{
	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;
	/// @}

	qint64 m_track_number {0};

    Frames m_length_pre_gap {0};
	Frames m_start_frames {0};
	Frames m_length_frames {0};
	Frames m_length_post_gap {0};

	/// CD-Text "pack type indicators".
	/// @link https://github.com/lipnitsk/libcue/blob/master/libcue.h
	/// These are the character types, see below for binary types.
#define PTI_STR_LIST(X) \
    X(PTI_TITLE) \
    X(PTI_PERFORMER) \
    X(PTI_SONGWRITER) \
    X(PTI_COMPOSER) \
    X(PTI_ARRANGER) \
    X(PTI_MESSAGE) \
    X(PTI_UPC_ISRC)

    // PTI_DISC_ID == binary disc identification info.
    // PTI_GENRE == binary genre.
#define PTI_BIN_LIST(X) \
    X(PTI_DISC_ID) \
    X(PTI_GENRE) \
    X(PTI_TOC_INFO1) \
    X(PTI_TOC_INFO2) \
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

	/// ISRC code.  May be empty.
	std::string m_isrc;

	/// Indexes.
	/// -1 means there was no such index in the cue sheet.
	std::vector<qint64> m_indexes;

	/// Derived info.
	bool m_is_part_of_gapless_set {false};

	Frames last_frame() const { return m_start_frames+m_length_frames; }

    friend QDebug operator<<(QDebug dbg, const TrackMetadata &tm);
};

Q_DECLARE_METATYPE(TrackMetadata);

// Qt5 already declares this.
//Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(std::vector);

QDebug operator<<(QDebug dbg, const TrackMetadata &tm);

#endif // TRACKMETADATA_H
