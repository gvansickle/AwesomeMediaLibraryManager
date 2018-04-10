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

#ifndef METADATA_H
#define METADATA_H

#include "MetadataAbstractBase.h"

class QJsonObject;
class MetadataFromCache;

class Metadata
{
private:
	std::unique_ptr<MetadataAbstractBase> pImpl;

public:
	Metadata() : pImpl(nullptr) {}
	Metadata(const MetadataAbstractBase& derived) : pImpl(derived.clone()) {}
	~Metadata() {}

	/// Copy constructor.
	Metadata(const Metadata& other) : pImpl((other.pImpl)? other.pImpl->clone() : nullptr) {}
	/// Assignment operator.
	Metadata& operator=(const Metadata& other)
	{
		if(&other != this)
		{
			pImpl = std::move(other.pImpl->clone());
		}
		return *this;
	}

	/// @name Static Factory Functions
	/// @{

	/// Static factory function for creating a new empty Metadata object.
	static Metadata make_metadata();

	/// Static factory function for creating a new Metadata from the given audio file URL.
	static Metadata make_metadata(QUrl file_url);

	/// Static factory function for creating a new Metadata from the given QJsonObject.
	static Metadata make_metadata(const QJsonObject& jo);

	/// @}

	static std::set<std::string> getNewTags();

	///
	/// "Redirectors".
	///

	///bool read(QUrl url) { return pImpl && pImpl->read(url); }
	bool hasBeenRead() const { return pImpl && pImpl->hasBeenRead(); }
	bool isError() const { return pImpl && pImpl->isError(); }
	bool isFromCache() const { return pImpl && pImpl->isFromCache(); }

	/// Conversion to bool.  Returns true if Metadata has been read successfully.
	operator bool() const { return hasBeenRead() && !isError(); }

	/// @name Meta-metadata.
	/// @{

	std::string GetFiletypeName() const;

	bool hasVorbisComments() const { return pImpl->m_has_vorbis_comment; }
	bool hasID3v1() const { return pImpl->m_has_id3v1; }
	bool hasID3v2() const { return pImpl->m_has_id3v2; }
	bool hasAPE() const { return pImpl->m_has_ape; }
	bool hasXiphComment() { return pImpl->m_has_ogg_xipfcomment; }
	bool hasInfoTag() { return pImpl->m_has_info_tag; }

	TagMap tagmap_VorbisComments() { return pImpl ? pImpl->m_tm_vorbis_comments : TagMap() ; }
	TagMap tagmap_id3v1() { return pImpl ? pImpl->m_tm_id3v1 : TagMap() ; }
	TagMap tagmap_id3v2() { return pImpl ? pImpl->m_tm_id3v2 : TagMap() ; }
	TagMap tagmap_ape() { return pImpl ? pImpl->m_tm_ape : TagMap() ; }
	TagMap tagmap_xiph() { return pImpl ? pImpl->m_tm_xipf : TagMap() ; }
	TagMap tagmap_InfoTag() { return pImpl ? pImpl->m_tm_infotag : TagMap() ; }
	/// @}

	/// Audio stream properites.
	Fraction total_length_seconds() const { return pImpl->total_length_seconds(); }

	/// Return the first entry matching the key, or an empty string if no such key.
	std::string operator[](const std::string& key) const { Q_ASSERT(pImpl); return pImpl->operator [](key); }

	/// Overload for const char *'s.
	std::string operator[](const char *key) const { return (*this)[std::string(key)]; }

	/// Return all string metadata as a map.
	TagMap filled_fields() const { return pImpl->filled_fields(); }

	/// Cue sheet support.
	bool hasCueSheet() const { return pImpl->hasCueSheet(); }

	/// @todo bool hasHiddenTrackOneAudio() const { return pImpl->hasHiddenTrackOneAudio(); }

	/// @name Track metadata.
	/// @{

	/// Return the number of tracks found in this file.
	int numTracks() const { return pImpl->numTracksOnMedia(); }
	TrackMetadata getThisTracksMetadata() const { return pImpl->getThisTracksMetadata(); }

	/// Return the TrackMetadata for the specified track.
	/// @note @a index is 1-based.
	TrackMetadata track(int index) const { return pImpl->track(index); }
	Metadata get_one_track_metadata(int track_index) const { return pImpl->get_one_track_metadata(track_index); }

	/// @}

	/// Embedded art.
	/// @todo int numEmbeddedPictures() const { return pImpl->numEmbeddedPictures(); }
	QByteArray getCoverArtBytes() const { Q_ASSERT(pImpl); return pImpl->getCoverArtBytes(); }

	/// @name Serialization
	/// @{
	void writeToJson(QJsonObject& jo) const;
	/// @}

};


QDataStream &operator<<(QDataStream &out, const Metadata &myObj);
QDataStream &operator>>(QDataStream &in, Metadata &myObj);


#endif // METADATA_H
