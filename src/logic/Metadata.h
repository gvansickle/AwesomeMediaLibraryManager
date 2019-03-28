/*
 * Copyright 2017, 2018, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef LOGIC_METADATA_H
#define LOGIC_METADATA_H

// Std C++
#include <set>

// Ours.
#include <utils/QtHelpers.h>
#include "AMLMTagMap.h"
#include "CueSheet.h"
#include "AudioFileType.h"
#include "TrackMetadata.h"
#include <utils/Fraction.h>
#include <logic/serialization/ISerializable.h>


class Metadata : public virtual ISerializable
{
public:
	M_GH_RULE_OF_FIVE_DEFAULT_C21(Metadata);
	~Metadata() override {};

	/// @name Static Factory Functions
	/// @{

	/// Static factory function for creating a new empty Metadata object.
	static Metadata make_metadata();

	/// Static factory function for creating a new Metadata from the given audio file URL.
	static Metadata make_metadata(QUrl file_url);

	/// Static factory function for creating a new Metadata from the given QVariant tree.
	static Metadata make_metadata(const QVariant& variant);

	/// @}

	static std::set<std::string> getNewTags();


	bool read(const QUrl& url);
	bool hasBeenRead() const;
	bool isError() const;
	/// Return true if the object was read from a cache rather than the actual file.
	/// Intent is that if this returns true, it shouldn't be written back to the cache.
	bool isFromCache() const;

	/// Conversion to bool.  Returns true if Metadata has been read successfully.
	explicit operator bool() const;

	/// @name Meta-metadata.
	/// @{

	std::string GetFiletypeName() const;

	bool hasVorbisComments() const { return m_has_vorbis_comment; }
	bool hasID3v1() const { return m_has_id3v1; }
	bool hasID3v2() const { return m_has_id3v2; }
	bool hasAPE() const { return m_has_ape; }
	bool hasXiphComment() const { return m_has_ogg_xipfcomment; }
	bool hasInfoTag() const { return m_has_riff_info_tag; }
	bool hasDiscCuesheet() const { return !m_tm_cuesheet_disc.empty(); }

	AMLMTagMap tagmap_VorbisComments() const { return m_tm_vorbis_comments; }
	AMLMTagMap tagmap_id3v1() const { return m_tm_id3v1; }
	AMLMTagMap tagmap_id3v2() const { return m_tm_id3v2; }
	AMLMTagMap tagmap_ape() const { return m_tm_ape; }
	AMLMTagMap tagmap_xiph() const { return m_tm_xipf; }
	AMLMTagMap tagmap_InfoTag() const { return m_tm_riff_infotag; }
	AMLMTagMap tagmap_cuesheet_disc() const;
	/// @}

	/// @name Audio stream properites.
	/// @{
	Fraction total_length_seconds() const;
	/// @}

	/// Return the first entry matching the key, or an empty string if no such key.
	std::string operator[](const std::string& key) const;

	/// Overload for const char *'s.
	std::string operator[](const char *key) const { return (*this)[std::string(key)]; }

	/// Return all string metadata as a map.
	TagMap filled_fields() const;

	/// Cue sheet support.
	bool hasCueSheet() const { return m_has_cuesheet; }

	/// @todo bool hasHiddenTrackOneAudio() const { return pImpl->hasHiddenTrackOneAudio(); }

	/// @name Track metadata.
	/// @{

M_TODO("We need a separate AMLMTrack class here.");

	/// Return the number of tracks found in this file.
	int numTracks() const { return m_num_tracks_on_media; }
	/// @todo OBSOLETE/BAD INTERFACE.
	TrackMetadata getThisTracksMetadata() const { return m_tracks.cbegin()->second; }

	bool hasTrackCuesheet() const { return numTracks() < 2; }
	AMLMTagMap tagmap_cuesheet_track() const;


	/// Return the TrackMetadata for the specified track.
	/// @note @a index is 1-based.
	TrackMetadata track(int index) const;
	Metadata get_one_track_metadata(int track_index) const;
	bool hasTrack(int i) const;

	/// @}

	/// Embedded art.
	/// @todo int numEmbeddedPictures() const { return pImpl->numEmbeddedPictures(); }
	QByteArray getCoverArtBytes() const;

	/// @name Serialization
	/// @{
	QTH_DECLARE_FRIEND_QDEBUG_OP(Metadata);
	QTH_FRIEND_QDATASTREAM_OPS(Metadata);

	/// Serialize item and any children to a QVariant.
	QVariant toVariant() const override;
	/// Serialize item and any children from a QVariant.
	void fromVariant(const QVariant& variant) override;

	/// @}

private:

	QUrl m_audio_file_url {};

	AudioFileType::Type m_audio_file_type {AudioFileType::UNKNOWN};

	/// @name Disc/full-file audio properties, obtained via TagLib.
	/// @{

	/// Per TagLib docs, "the most appropriate bit rate for the file in kb/s. For
	/// constant bitrate formats this is simply the bitrate of the file. For variable
	/// bitrate formats this is either the average or nominal bitrate.".
	int64_t m_bitrate_kb_sec {0};

	/// Number of channels of audio.
	int8_t m_num_channels {0};

	/// Sample rate in samples/sec.
	int64_t m_sample_rate {0};

	/// Length of the entire file in ms.
	/// We need this for the CueSheet so we can determine the length of the final track.
	int64_t m_length_in_milliseconds {0};
	/// @}

	bool m_has_cuesheet {false};
	CueSheet m_cuesheet;

	/// @name Cuesheet-derived CD-level info.
	/// @{

	/// @}

	bool m_has_vorbis_comment {false};
	bool m_has_id3v1 {false};
	bool m_has_id3v2 {false};
	bool m_has_ape {false};
	bool m_has_ogg_xipfcomment {false};
	bool m_has_riff_info_tag {false};

	AMLMTagMap m_tm_vorbis_comments;
	AMLMTagMap m_tm_id3v1;
	AMLMTagMap m_tm_id3v2;
	AMLMTagMap m_tm_ape;
	AMLMTagMap m_tm_xipf;
	AMLMTagMap m_tm_riff_infotag;
	AMLMTagMap m_tm_cuesheet_disc {};

	/// The TagMap from the generic "fr.tag()->properties()" call.
	AMLMTagMap m_tm_generic;

	bool m_read_has_been_attempted {false};
	bool m_is_error {false};
	bool m_is_from_cache {false};



	/// @name Track info.
	/// @{

	/// The number of tracks on the audio file this Metadata applies to.
	int m_num_tracks_on_media {0};

	/// Collection of track metadata.  May be empty, may contain multiple entries for a single-file multi-song image.
	std::map<int, TrackMetadata> m_tracks {};

	/// Same as above, but in AMLMTagMap form.
//	AMLMTagMap m_track_amlmtagmaps {};

	/// @}

};

Q_DECLARE_METATYPE(Metadata);

QDataStream &operator<<(QDataStream &out, const Metadata &obj);
QDataStream &operator>>(QDataStream &in, Metadata &obj);


#endif // LOGIC_METADATA_H
