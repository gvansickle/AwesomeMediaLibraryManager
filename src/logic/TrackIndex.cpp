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

#include <config.h>

#include "TrackIndex.h"

// Std C++
#include <memory>
#include <string>

// Qt5
#include <QString>

// Libcue
extern "C" {
#include <libcue/cdtext.h>
#include <libcue/cd.h>
#include <libcue/libcue.h>
}

// Ours
#include <utils/TheSimplestThings.h>
#include <logic/serialization/SerializationHelpers.h>
#include "AMLMTagMap.h"
#include <utils/RegisterQtMetatypes.h>
#include <future/InsertionOrderedMap.h>


using strviw_type = QLatin1String;

#define M_TRACK_INDEX_DATASTREAM_FIELDS(X) \
	X(XMLTAG_TRACK_INDEX_NUM, "index_num", m_index_num) \
	X(XMLTAG_TRACK_INDEX_FRAMES, "index_frames", m_index_frames)

/// Strings to use for the tags.
#define X(field_tag, field_tag_str, member_field) static const strviw_type field_tag ( field_tag_str );
	M_TRACK_INDEX_DATASTREAM_FIELDS(X);
#undef X



QVariant TrackIndex::toVariant() const
{
	InsertionOrderedMap<QString, QVariant> map;

#define X(field_tag, field_tag_str, member_field) map_insert_or_die(map, field_tag, member_field);
	M_TRACK_INDEX_DATASTREAM_FIELDS(X)
#undef X

	return map;
}

void TrackIndex::fromVariant(const QVariant& variant)
{
	InsertionOrderedMap<QString, QVariant> map = variant.value<InsertionOrderedMap<QString, QVariant>>();

#define X(field_tag, field_tag_str, member_field) map_read_field_or_warn(map, field_tag, &(member_field));
	M_TRACK_INDEX_DATASTREAM_FIELDS(X)
#undef X
}
