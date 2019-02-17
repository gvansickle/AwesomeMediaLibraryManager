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

// Std C++.
#include <memory>
#include <string>
#include <map>

// Qt5
#include <QJsonObject>
#include <QVariant>
#include <QVariantMap>

// Ours.
#include "MetadataTaglib.h"
#include "MetadataFromCache.h"
#include "utils/MapConverter.h"
#include <utils/DebugHelpers.h>
#include <utils/RegisterQtMetatypes.h>


AMLM_QREG_CALLBACK([](){
	qIn() << "Registering Metadata metatypes";
	qRegisterMetaType<Metadata>();
	qRegisterMetaTypeStreamOperators<Metadata>();
//	QMetaType::registerDebugStreamOperator<Metadata>();
//	QMetaType::registerConverter<Metadata, QString>([](const Metadata& obj){ return obj.name(); });
});


static std::map<AudioFileType, std::string> f_filetype_to_string_map =
{
	{AudioFileType::UNKNOWN, "unknown"},
	{AudioFileType::FLAC, "FLAC"},
	{AudioFileType::MP3, "MP3"},
	{AudioFileType::OGG_VORBIS, "Ogg Vorbis"},
	{AudioFileType::WAV, "WAV"}
};

Metadata::~Metadata() {}

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

Metadata Metadata::make_metadata(const QVariant& variant)
{
	QVariantMap map = variant.toMap();

	QVariantMap metadata_tagtree = map.value("metadata_tagtree").toMap();

//	Q_ASSERT(map.canConvert<MetadataFromCache>());
	MetadataFromCache retval;
	retval.fromVariant(metadata_tagtree);
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

QVariant Metadata::toVariant() const
{
	QVariantMap retval;
	/// @todo
	retval.insert("metadata_tagtree", MapConverter::TagMaptoVarMap(pImpl->m_tag_map));

	return retval;
}

void Metadata::fromVariant(const QVariant& variant)
{
//	return pImpl->fromVariant(variant);
	/// @todo
	QVariantMap map = variant.toMap();
	using tag_map_var_type = std::map<std::string, std::vector<std::string>>;
	QVariantMap tag_map_variant;
	tag_map_variant = map.value("metadata_tagtree").value<QVariantMap>();
	tag_map_var_type tag_map = MapConverter::VarMapToTagMap(tag_map_variant);

	pImpl->m_tag_map = tag_map;
}

QDataStream &operator<<(QDataStream &out, const Metadata &obj)
{
	/// @todo
	out << MapConverter::TagMaptoVarMap(obj.pImpl->m_tag_map);
	return out;
}

QDataStream &operator>>(QDataStream &in, Metadata &obj)
{
	/// @todo

	QVariantMap tag_map;
	using tag_map_var_type = std::map<std::string, std::vector<std::string>>;
	tag_map_var_type tag_map_std;
///	in >> tag_map_std;

	obj.pImpl->m_tag_map = tag_map_std;

	return in;
}
