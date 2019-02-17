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

#include "MetadataFromCache.h"

#include "Metadata.h"

#include "utils/DebugHelpers.h"
#include "utils/MapConverter.h"

#include <QJsonObject>

MetadataFromCache::MetadataFromCache()
{

}

MetadataFromCache::~MetadataFromCache()
{

}

void MetadataFromCache::readFromJson(const QJsonObject& jo)
{
	///qDebug() << "Reading Metadata from Json:" << jo.toVariantMap();
	m_tag_map = MapConverter::VarMapToTagMap(jo.toVariantMap());
	///qDebug() << "Reading Metadata from Json:" << m_tag_map;
}

QVariant MetadataFromCache::toVariant() const
{
	Q_ASSERT_X(0, __func__, "NOT IMPLEMENTED");
	return QVariant();
}

void MetadataFromCache::fromVariant(const QVariant& variant)
{
	QVariantMap map = variant.toMap();
	m_tag_map = MapConverter::VarMapToTagMap(map);
}

Metadata MetadataFromCache::get_one_track_metadata(int track_index) const
{
	Q_ASSERT(0);
    return Metadata();
}

MetadataFromCache* MetadataFromCache::clone_impl() const
{
	return new MetadataFromCache(*this);
}

QByteArray MetadataFromCache::getCoverArtBytes() const
{
	QByteArray retval;

	/// @todo Demand load?
	qWarning() << "MetadataFromCache: No cover art available";

	return retval;
}
