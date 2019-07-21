//
// Created by gary on 7/21/19.
//

#include <config.h>

#include "TrackIndex.h"

// Std C++
#include <memory>
#include <string>

// Libcue
#include <libcue/cdtext.h>
#include <libcue/cd.h>
#include <libcue/libcue.h>

// Ours
#include <utils/TheSimplestThings.h>
#include <logic/serialization/SerializationHelpers.h>
#include "AMLMTagMap.h"
#include <utils/RegisterQtMetatypes.h>

#include "TrackMetadata.h"

QVariant TrackIndex::toVariant() const
{
	QVariantInsertionOrderedMap map;

#define X(field_tag, field_tag_str, member_field) map_insert_or_die(map, field_tag, member_field);
	M_TRACK_INDEX_DATASTREAM_FIELDS(X)
#undef X

	return map;
}

void TrackIndex::fromVariant(const QVariant& variant)
{
	QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();

#define X(field_tag, field_tag_str, member_field) map_read_field_or_warn(map, field_tag, &member_field);
	M_TRACK_INDEX_DATASTREAM_FIELDS(X)
#undef X
}
