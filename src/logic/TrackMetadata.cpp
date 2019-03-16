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

#include <config.h>

#include "TrackMetadata.h"

// Std C++
#include <string>

// Libcue.
#include <libcue/libcue.h>
#include <libcue/cd.h>

/// Ours, Qt5/KF5-related
#include <utils/TheSimplestThings.h>
#include <utils/RegisterQtMetatypes.h>



AMLM_QREG_CALLBACK([](){
	qIn() << "Registering TrackMetadata";
    qRegisterMetaType<TrackMetadata>();
    ;});

Q_DECLARE_METATYPE(std::string);


using strviw_type = QLatin1Literal;

#define M_DATASTREAM_FIELDS(X) \
	X(XMLTAG_TRACK_META_TRACK_NUM, m_track_number) \
	X(XMLTAG_TRACK_META_LEN_PREGAP, m_length_pre_gap) \
	X(XMLTAG_TRACK_META_START_FRAMES, m_start_frames) \
	X(XMLTAG_TRACK_META_LENGTH_FRAMES, m_length_frames) \
	X(XMLTAG_TRACK_META_LENGTH_POST_GAP, m_length_post_gap) \
	X(XMLTAG_TRACK_META_ISRC, m_isrc) \
	X(XMLTAG_TRACK_META_IS_PART_OF_GAPLESS_SET, m_is_part_of_gapless_set)

#define M_DATASTREAM_FIELDS_SPECIAL_HANDLING(X) \
	X(XMLTAG_TRACK_META_INDEXES, m_indexes)

/// Strings to use for the tags.
#define X(field_tag, member_field) static const strviw_type field_tag ( # member_field );
	M_DATASTREAM_FIELDS(X);
	M_DATASTREAM_FIELDS_SPECIAL_HANDLING(X);
#undef X


std::unique_ptr<TrackMetadata> TrackMetadata::make_track_metadata(const Track* track_ptr, int track_number)
{
	auto retval = std::make_unique<TrackMetadata>();

	// The non-CD-Text info.
	retval->m_track_number = track_number;

	// The track's audio data location info, as parsed by libcue.
	auto& tm = *retval;
	tm.m_length_pre_gap = track_get_zero_pre(track_ptr);
	tm.m_start_frames = track_get_start(track_ptr);
	tm.m_length_frames = track_get_length(track_ptr);
	tm.m_length_post_gap = track_get_zero_post(track_ptr);

	// The track's indexes, which should simply duplicate the above.
	for(auto i = 0; i<=99; ++i)
	{
		//qDebug() << "Reading track index:" << i;
		long ti = track_get_index(track_ptr, i);
		TrackIndex track_index;
		track_index.m_index_num = "index" + std::to_string(i);
		track_index.m_index_frames = ti;

		if((ti==-1) && (i>1))
		{
			// qDb() << "Found last index: " << i-1;
			break;
		}
		else
		{
//          qDb() << " Index:" << ti;
			tm.m_indexes.push_back(track_index);
		}
	}

#if 0 /// @todo Do we still need to clean up the last track's length?
	if(tm.m_length_frames < 0)
	{
		// This is the last track.  We have to calculate the length from the total recording time minus the start offset.
		Q_ASSERT(m_length_in_milliseconds > 0);
		tm.m_length_frames = (75.0*double(m_length_in_milliseconds)/1000.0) - tm.m_start_frames;
	}
#endif
	tm.m_isrc = tostdstr(track_get_isrc(track_ptr));

	// Get the per-track CD-Text info.
	const Cdtext* track_cdtext = track_get_cdtext(track_ptr);

	// Get the Pack Type Indicator data.
#define X(id) retval->m_ ## id = tostdstr(cdtext_get( id , track_cdtext ));
	PTI_STR_LIST(X)
#undef X

	return retval;
}


std::string TrackMetadata::toStdString() const
{
	std::string retval;

    retval = M_IDSTR(m_PTI_TITLE) M_IDSTR(m_PTI_PERFORMER) "";
    retval += M_IDSTR(m_track_number) /*M_IDSTR(m_total_track_number)*/ M_IDSTR(m_length_pre_gap) M_IDSTR(m_start_frames) "";
	retval += M_IDSTR(m_length_frames) M_IDSTR(m_length_post_gap) M_IDSTR(m_isrc) "";

	return retval;
}


QVariant TrackMetadata::toVariant() const
{
	QVariantInsertionOrderedMap map;

#define X(field_tag, member_field) map_insert_or_die(map, field_tag, member_field);
	M_DATASTREAM_FIELDS(X);
#undef X

	// Serialize the CD-Text Pack Type Indicator data.
#define X(id) map_insert_or_die(map, # id , m_ ## id);
	PTI_STR_LIST(X)
#undef X

	// m_indexes
#if 0
	QVariantHomogenousList hlist("listname", "entryname");
	hlist = m_indexes;
	map.insert(XMLTAG_TRACK_META_INDEXES, QVariant::fromValue(hlist));
#else
	QVariantInsertionOrderedMap index_map;
	for(const auto& index : m_indexes)
	{
		map_insert_or_die(index_map, toqstr(index.m_index_num), (index.m_index_frames));
	}
	map_insert_or_die(map, XMLTAG_TRACK_META_INDEXES, QVariant::fromValue(index_map));
#endif
	return map;
}

void TrackMetadata::fromVariant(const QVariant& variant)
{
	QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();

#define X(field_tag, member_field) member_field = map.value( field_tag ).value<decltype( member_field )>();
	M_DATASTREAM_FIELDS(X);
#undef X

#define X(id) m_ ## id = map.value( # id ).value<decltype( m_ ## id )>();
	PTI_STR_LIST(X);
#undef X



#if TEMP
	QVariant qvar_hlist = map.value(XMLTAG_TRACK_META_INDEXES);
	Q_ASSERT(qvar_hlist.isValid());
	if(qvar_hlist.isNull())
	{
		return;
	}
	Q_ASSERT(qvar_hlist.canConvert<QVariantHomogenousList>());
	auto hlist = qvar_hlist.value<QVariantHomogenousList>();
	m_indexes.clear();
	for(const auto& val : hlist)
	{
		m_indexes.push_back(val.value<qint64>());
	}
#endif
}


QDebug operator<<(QDebug dbg, const TrackMetadata &tm)
{
    QDebugStateSaver saver(dbg);

#define X(id) dbg << "TrackMetadata(" << #id ":" << tm.m_ ## id << ")\n";
    PTI_STR_LIST(X)
#undef X

    return dbg;
}


QVariant TrackIndex::toVariant() const
{
	QVariantInsertionOrderedMap map;

	map_insert_or_die(map, "index_num", m_index_num);
	map_insert_or_die(map, "index_frames", m_index_frames);

	return map;
}

void TrackIndex::fromVariant(const QVariant& variant)
{

}
