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

#include "Metadata.h"

#include "MetadataTaglib.h"
#include "MetadataFromCache.h"

#include "utils/MapConverter.h"

#include <QJsonObject>

#include <memory>
#include <string>
#include <map>

static std::map<AudioFileType, std::string> f_filetype_to_string_map =
{
	{AudioFileType::UNKNOWN, "unknown"},
	{AudioFileType::FLAC, "FLAC"},
	{AudioFileType::MP3, "MP3"},
	{AudioFileType::OGG_VORBIS, "Ogg Vorbis"},
	{AudioFileType::WAV, "WAV"}
};

Metadata Metadata::make_metadata()
{
	return Metadata(MetadataTaglib());
}

Metadata Metadata::make_metadata(QUrl file_url)
{
	MetadataTaglib retval;
	retval.read(file_url);
	return retval;
}

Metadata Metadata::make_metadata(const QJsonObject& jo)
{
	MetadataFromCache retval;
	retval.readFromJson(jo);
	retval.m_read_has_been_attempted = true;
	retval.m_is_error = false;
	return retval;
}

std::set<std::string> Metadata::getNewTags()
{
	return MetadataTaglib::getNewTags();
}

std::string Metadata::GetFiletypeName() const
{
	return f_filetype_to_string_map[pImpl->m_file_type];
}

void Metadata::writeToJson(QJsonObject& jo) const
{
	Q_ASSERT(pImpl);
//	qDebug() << "Writing Metadata to QJsonObject:" << jo;
	jo["metadata"] = QJsonObject::fromVariantMap(MapConverter::TagMaptoVarMap(pImpl->m_tag_map));
}

QDataStream &operator<<(QDataStream &out, const Metadata &myObj)
{
	return out;
}

