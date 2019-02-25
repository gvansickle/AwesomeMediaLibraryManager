/*
 * Copyright 2017, 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "TrackMetadata.h"

// Std C++
#include <string>

/// Ours, Qt5/KF5-related
#include <utils/TheSimplestThings.h>
#include <utils/RegisterQtMetatypes.h>


AMLM_QREG_CALLBACK([](){
	qIn() << "Registering TrackMetadata";
    qRegisterMetaType<TrackMetadata>();
    ;});

Q_DECLARE_METATYPE(std::string);

TrackMetadata::TrackMetadata()
{

}

std::string TrackMetadata::toStdString() const
{
	std::string retval;

    retval = M_IDSTR(m_PTI_TITLE) M_IDSTR(m_PTI_PERFORMER) "";
    retval += M_IDSTR(m_track_number) /*M_IDSTR(m_total_track_number)*/ M_IDSTR(m_length_pre_gap) M_IDSTR(m_start_frames) "";
	retval += M_IDSTR(m_length_frames) M_IDSTR(m_length_post_gap) M_IDSTR(m_isrc) "";

	return retval;
}

using strviw_type = QLatin1String;

#define M_DATASTREAM_FIELDS(X) \
	X(XMLTAG_TRACK_META_TRACK_NUM, m_track_number) \
	X(XMLTAG_TRACK_META_LEN_PREGAP, m_length_pre_gap) \
	X(XMLTAG_TRACK_META_START_FRAMES, m_start_frames) \
	X(XMLTAG_TRACK_META_LENGTH_FRAMES, m_length_frames) \
	X(XMLTAG_TRACK_META_LENGTH_POST_GAP, m_length_post_gap) \
	X(XMLTAG_TRACK_META_ISRC, m_isrc) \
	/*X(XMLTAG_TRACK_META_INDEXES, m_indexes)*/ \
	X(XMLTAG_TRACK_META_IS_PART_OF_GAPLESS_SET, m_is_part_of_gapless_set)

/// Strings to use for the tags.
#define X(field_tag, member_field) static constexpr strviw_type field_tag ( # member_field );
	M_DATASTREAM_FIELDS(X);
#undef X

QVariant TrackMetadata::toVariant() const
{
	QVariantInsertionOrderedMap map;

#define X(field_tag, member_field) map.insert( field_tag , QVariant::fromValue<decltype(member_field)>( member_field ) );
	M_DATASTREAM_FIELDS(X);
#undef X

	return map;
}

void TrackMetadata::fromVariant(const QVariant& variant)
{
	QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();

#define X(field_tag, member_field) member_field = map.value( field_tag ).value<decltype( member_field )>();
	M_DATASTREAM_FIELDS(X);
#undef X
}


QDebug operator<<(QDebug dbg, const TrackMetadata &tm)
{
    QDebugStateSaver saver(dbg);

#define X(id) dbg << "TrackMetadata(" << #id ":" << tm.m_ ## id << ")\n";
    PTI_STR_LIST
#undef X

    return dbg;
}
