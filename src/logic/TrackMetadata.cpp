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
#include <memory>

// Libcue.
extern "C" {
#include <libcue/libcue.h>
#include <libcue/cd.h>
#include <libcue/cdtext.h>
} // END extern C

/// Ours, Qt5/KF5-related
#include <utils/TheSimplestThings.h>
#include <utils/RegisterQtMetatypes.h>

#include "AMLMTagMap.h"
#include <logic/serialization/SerializationHelpers.h>




AMLM_QREG_CALLBACK([](){
	qIn() << "Registering TrackMetadata";
    qRegisterMetaType<TrackMetadata>();
    ;});

//Q_DECLARE_METATYPE(std::string);


using strviw_type = QLatin1String;


std::unique_ptr<TrackMetadata> TrackMetadata::make_track_metadata(const Track* track_ptr, int track_number)
{
	auto retval = std::make_unique<TrackMetadata>();

	auto& tm = *retval;


	// The non-CD-Text info.
	tm.m_track_number = track_number;

	tm.m_track_filename = track_get_filename(track_ptr);
	tm.m_isrc = tostdstr(track_get_isrc(track_ptr));

	// The track's audio data location info, as parsed by libcue.
	tm.m_length_pre_gap = track_get_zero_pre(track_ptr);
	tm.m_start_frames = track_get_start(track_ptr);
	tm.m_length_frames = track_get_length(track_ptr);
	tm.m_length_post_gap = track_get_zero_post(track_ptr);

	// The track's indexes, which should simply duplicate the above.
	for(auto i = 0; i<=99; ++i)
	{
		//qDebug() << "Reading track index:" << i;
		long ti = track_get_index(track_ptr, i);

		if((ti==-1) && (i>1))
		{
			// Found the last index.
			break;
		}
		else
		{
			TrackIndex track_index;
			track_index.m_index_num = std::to_string(i);
			track_index.m_index_frames = ti;
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


	// Get the per-track CD-Text info.
	const Cdtext* track_cdtext = track_get_cdtext(track_ptr);

	if(track_cdtext != nullptr)
	{
		// Get the track's Pack Type Indicator info as an AMLMTagMap.
		for(int pti = Pti::PTI_TITLE; pti < Pti::PTI_END; pti++)
		{
			const char* tcdt_value = cdtext_get((Pti)pti, track_cdtext);
			if(tcdt_value != nullptr)
			{
				std::string key_str = cdtext_get_key(pti, 1);
				tm.m_tm_track_pti.insert(key_str, tcdt_value);
			}
		}
	}

	if(tm.m_tm_track_pti.find("TITLE") != tm.m_tm_track_pti.cend())
	{
		qDb() << "TRACK CDTEXT INFO:" << toqstr(tm.m_tm_track_pti.find("TITLE")->second);
	}

	// Get the Pack Type Indicator data.
#define X(id) retval->m_ ## id = tostdstr(cdtext_get( id , track_cdtext ));
	PTI_STR_LIST(X)
#undef X

M_TODO("REPLACE THE ABOVE");
//	/// @todo Get the track's Pack Type Indicator info as an AMLMTagMap.
//#define X(id) tm.m_tm_track_pti.insert( # id, tostdstr(cdtext_get( id , track_cdtext )));
//	PTI_STR_LIST(X)
//#undef X


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

#define M_DATASTREAM_FIELDS(X) \
	X(XMLTAG_TRACK_META_TRACK_NUM, m_track_number) \
	X(XMLTAG_TRACK_META_LEN_PREGAP, m_length_pre_gap) \
	X(XMLTAG_TRACK_META_START_FRAMES, m_start_frames) \
	X(XMLTAG_TRACK_META_LENGTH_FRAMES, m_length_frames) \
	X(XMLTAG_TRACK_META_LENGTH_POST_GAP, m_length_post_gap) \
	X(XMLTAG_TRACK_META_ISRC, m_isrc) \
	X(XMLTAG_TRACK_META_IS_PART_OF_GAPLESS_SET, m_is_part_of_gapless_set) \
	X(XMLTAG_TRACK_PTI_VALUES, m_tm_track_pti)

#define M_DATASTREAM_FIELDS_SPECIAL_HANDLING(X) \
	X(XMLTAG_TRACK_META_INDEXES, m_indexes)

/// Strings to use for the tags.
#define X(field_tag, member_field) static const strviw_type field_tag ( # member_field );
	M_DATASTREAM_FIELDS(X);
	M_DATASTREAM_FIELDS_SPECIAL_HANDLING(X);
#undef X


QVariant TrackMetadata::toVariant() const
{
	InsertionOrderedMap<QString, QVariant> map;

	// Set some extra class info to the attributes.
//	set_map_class_info(this, &map);

	// Set the xml:id.
	map.insert_attributes({{"xml:id", get_prefixed_uuid()}});

#define X(field_tag, member_field) map_insert_or_die(map, field_tag, member_field);
	M_DATASTREAM_FIELDS(X);
#undef X

	// m_indexes
	QVariantHomogenousList index_list("m_indexes", "index");
	for(const TrackIndex& index : m_indexes)
	{
		list_push_back_or_die(index_list, index);
	}

	map_insert_or_die(map, XMLTAG_TRACK_META_INDEXES, index_list);

	return map;
}

void TrackMetadata::fromVariant(const QVariant& variant)
{
	InsertionOrderedMap<QString, QVariant> map = variant.value<InsertionOrderedMap<QString, QVariant>>();

	auto uuid = map.get_attr("xml:id", {});
	if(uuid.empty())
	{
		qWr() << "UUID EMPTY";
	}
	else
	{
		set_prefixed_uuid(uuid);
	}

#define X(field_tag, member_field) map_read_field_or_warn(map, field_tag, & (member_field) );
	M_DATASTREAM_FIELDS(X);
#undef X

M_TODO("REMOVE");
#define X(id) m_ ## id = map.value( # id ).value<decltype( m_ ## id )>();
	PTI_STR_LIST(X);
#undef X

	// Load the index list.
	QVariantHomogenousList index_list("m_indexes", "index");
	map_read_field_or_warn(map, XMLTAG_TRACK_META_INDEXES, &index_list);

	// Read the m_indexes TrackIndex'es out of the list.
	// This is a QList<QVariant> where the qvar holds QVariantInsertionOrderedMap's.
	for(const QVariant& qvar_index_entry : qAsConst(index_list))
	{
		Q_ASSERT(qvar_index_entry.isValid());
		Q_ASSERT((qvar_index_entry.canConvert<InsertionOrderedMap<QString, QVariant>>()));

		InsertionOrderedMap<QString, QVariant> qvmap_index_entry = qvar_index_entry.value<InsertionOrderedMap<QString, QVariant>>();
		TrackIndex ti;
		ti.fromVariant(qvmap_index_entry);
		m_indexes.push_back(ti);
	}

	if(index_list.size() != m_indexes.size())
	{
		qWr() << "m_indexes size mismatch:" << index_list.size() << "!=" << m_indexes.size();
	}
}


QDebug operator<<(QDebug dbg, const TrackMetadata &tm)
{
    QDebugStateSaver saver(dbg);

#define X(id) dbg << "TrackMetadata(" << #id ":" << tm.m_ ## id << ")\n";
    PTI_STR_LIST(X)
#undef X

    return dbg;
}

