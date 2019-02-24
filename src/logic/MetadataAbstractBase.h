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

#ifndef METADATAABSTRACTBASE_H
#define METADATAABSTRACTBASE_H

// Std C++
#include <set>
#include <memory>

// Qt5
#include <QUrl>
#include <QObject>

// Ours.
#include <logic/serialization/ISerializable.h>
#include <future/guideline_helpers.h>
#include <src/utils/Fraction.h>
#include <utils/EnumFlagHelpers.h>
#include <utils/QtHelpers.h>
#include <logic/serialization/ExtEnum.h>
#include "TrackMetadata.h"
#include "AudioFileType.h"
#include "AMLMTagMap.h"

// Taglib
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>



class Metadata;
class TrackMetadata;

class MetadataAbstractBase : public virtual ISerializable
{

public:
	M_GH_RULE_OF_ZERO(MetadataAbstractBase);
//	MetadataAbstractBase() = default;
//	virtual ~MetadataAbstractBase() = default;
//	M_GH_POLYMORPHIC_SUPPRESS_COPYING_C67(MetadataAbstractBase);

	// Non-virtual clone() interface, because of no covariance for smart ptrs.
	std::unique_ptr<MetadataAbstractBase> clone() const { return std::unique_ptr<MetadataAbstractBase>(this->clone_impl()); }

	virtual bool read(const QUrl& url) = 0;

	virtual bool hasBeenRead() const { return m_read_has_been_attempted; }
	virtual bool isError() const { return m_is_error; }
	// Return true if the object was read from a cache rather than the actual file.
	// Intent is that if this returns true, it shouldn't be written back to the cache.
	virtual bool isFromCache() const = 0;

	/// Audio stream properites.
	virtual Fraction total_length_seconds() const;

	/// Return the first entry matching the key, or an empty string if no such key.
	/// @deprecated I think.  First entry matching key can be find().
	virtual std::string operator[](const std::string& key) const __attribute__((deprecated));

	/// Return all string metadata as a map.
	virtual TagMap filled_fields() const;

	/// Cue sheet support.
	virtual bool hasCueSheet() const { return m_has_cuesheet; }

	/// @todo virtual bool hasHiddenTrackOneAudio() const = 0;

	/// Track metadata.
	int numTracksOnMedia() const { return m_num_tracks_on_media; }
	/// @todo Bad name, this Metadata may not apply to a single track.
	TrackMetadata getThisTracksMetadata() const { return m_tracks.cbegin()->second; }
	virtual TrackMetadata track(int i) const;
	virtual Metadata get_one_track_metadata(int track_index) const = 0;

	/// Embedded art.
	/// @todo virtual int numEmbeddedPictures() const = 0;
	virtual QByteArray getCoverArtBytes() const = 0;

	/// @name Serialization support.
	/// @{
	QTH_FRIEND_QDATASTREAM_OPS(MetadataAbstractBase);

	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;
	/// @}

public:
	/// @name Data members.
	/// So yeah, this isn't really a very abstract Abstract Base Class....

	AudioFileType::Type m_audio_file_type {AudioFileType::UNKNOWN};

	bool m_has_cuesheet {false};

	bool m_has_vorbis_comment {false};
	bool m_has_id3v1 {false};
	bool m_has_id3v2 {false};
	bool m_has_ape {false};
	bool m_has_ogg_xipfcomment {false};
	bool m_has_info_tag {false};

	TagMap m_tm_vorbis_comments;
	TagMap m_tm_id3v1;
	TagMap m_tm_id3v2;
	TagMap m_tm_ape;
	TagMap m_tm_xipf;
	TagMap m_tm_infotag;

	/// The TagMap from the generic "fr.tag()->properties()" call.
	AMLMTagMap m_tag_map;

	bool m_read_has_been_attempted {false};
	bool m_is_error {false};

protected:
	QUrl m_audio_file_url {};

	long m_length_in_milliseconds {0};

	/// @name Track info.
	/// @{

	/// The number of tracks on the audio file this Metadata applies to.
	int m_num_tracks_on_media {0};

	/// Collection of track metadata.  May be empty, may contain multiple entries for a single-file multi-song image.
	std::map<int, TrackMetadata> m_tracks;

private:
	/// Override this in derived classes.
	virtual MetadataAbstractBase * clone_impl() const = 0;
};

#endif // METADATAABSTRACTBASE_H
