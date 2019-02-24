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


static std::map<AudioFileType::Type, std::string> f_filetype_to_string_map =
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

static const QString XMLTAG_METADATA_ABSTRACT_BASE_PIMPL {"metadata_abstract_base_pimpl"};


Metadata Metadata::make_metadata(const QVariant& variant)
{
	Q_ASSERT(variant.isValid());
#if 0
M_TODO("What's going on here?");
/// Contains only "metadata_abstract_base_pimpl", which contains only "base_class_variant".
	QVariantMap map = variant.toMap();

	QVariant pimpl_qvar= map.value(XMLTAG_METADATA_ABSTRACT_BASE_PIMPL);
	Q_ASSERT(pimpl_qvar.isValid());

//	Q_ASSERT(map.canConvert<MetadataFromCache>());
	MetadataFromCache retval;
	retval.fromVariant(pimpl_qvar);
	retval.m_read_has_been_attempted = true;
	retval.m_is_error = false;
	return retval;
#else
	QVariantMap map = variant.toMap();
	Metadata retval = make_metadata();
	retval.fromVariant(variant);
	return retval;
#endif
}

std::set<std::string> Metadata::getNewTags()
{
	return MetadataTaglib::getNewTags();
}

std::string Metadata::GetFiletypeName() const
{
	return f_filetype_to_string_map[pImpl->m_audio_file_type];
}

void Metadata::writeToJson(QJsonObject& jo) const
{
	Q_ASSERT(pImpl);
//	qDebug() << "Writing Metadata to QJsonObject:" << jo;
#warning "TEMP FOR XML CONVERSION"
//	jo["metadata"] = QJsonObject::fromVariantMap(MapConverter::TagMaptoVarMap(pImpl->m_tag_map));
//	jo["metadata"] = QJsonObject::fromVariantMap(pImpl->m_tag_map.toVariant());
}


QVariant Metadata::toVariant() const
{
	QVariantMap map;
	/// @todo
//	map.insert("metadata_tagtree", MapConverter::TagMaptoVarMap(pImpl->m_tag_map));
//	map.insert("m_tag_map", m_tag_map);
	map.insert(XMLTAG_METADATA_ABSTRACT_BASE_PIMPL, pImpl->toVariant());

	return map;
}

void Metadata::fromVariant(const QVariant& variant)
{
	QVariantMap map = variant.toMap();

	QVariant pimpl_qvar= map.value(XMLTAG_METADATA_ABSTRACT_BASE_PIMPL);

	Q_ASSERT(pimpl_qvar.isValid());

	pImpl->fromVariant(pimpl_qvar);
}

QDataStream &operator<<(QDataStream &out, const Metadata &obj)
{
	Q_ASSERT(0);
	/// @todo
//	out << MapConverter::TagMaptoVarMap(obj.pImpl->m_tag_map);
	return out;
}

QDataStream &operator>>(QDataStream &in, Metadata &obj)
{
	Q_ASSERT(0);
	/// @todo

	QVariantMap tag_map;
	using tag_map_var_type = std::map<std::string, std::vector<std::string>>;
	tag_map_var_type tag_map_std;
///	in >> tag_map_std;

//	obj.pImpl->m_tag_map = tag_map_std;

	return in;
}
