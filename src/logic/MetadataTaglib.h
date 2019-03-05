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

#ifndef METADATATAGLIB_H
#define METADATATAGLIB_H

#include "MetadataAbstractBase.h"
#include "TrackMetadata.h"
#include "CueSheetParser.h"

#if 0

class MetadataTaglib : public MetadataAbstractBase
{
public:
	MetadataTaglib() {};
	~MetadataTaglib() override {};

    bool isFromCache() const override { return false; }

	static std::set<std::string> getNewTags();

    bool read(const QUrl& url) override;

	/// Audio stream properites.
	/// @{
	/// @}


	/// @todo virtual bool hasHiddenTrackOneAudio() const override;

	/// Track metadata.
	/// @{

	///virtual TrackMetadata track(int i) const override;
    Metadata get_one_track_metadata(int track_index) const override;

	/// @}

	/// Embedded art.
	/// @todo virtual int numEmbeddedPictures() const override;
    QByteArray getCoverArtBytes() const override;

private:

    MetadataTaglib* clone_impl() const override;
};

#endif

namespace TagLib
{
	namespace FLAC
	{
		class File;
	}
};

QString get_cue_sheet_from_OggXipfComment(TagLib::FLAC::File* file);
QByteArray getCoverArtBytes(const QUrl& url);

#endif // METADATATAGLIB_H
