/*
 * Copyright 2018, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/**
 * @file ExtUrl.cpp
 */

#include "ExtUrl.h"

// Qt
#include <QFileInfo>

// Ours, Qt Support
#include <utils/RegisterQtMetatypes.h>

// Ours
#include <utils/DebugHelpers.h>
#include <logic/serialization/SerializationHelpers.h>


AMLM_QREG_CALLBACK([](){
	qIn() << "Registering ExtUrl";
	qRegisterMetaType<ExtUrl>();
});


ExtUrl::ExtUrl(const QUrl& qurl, const QFileInfo* qurl_finfo) : m_url(qurl)
{
	// Capture modification info, possibly loading it from the filesystem if we don't have it in qurl_finfo.
	load_mod_info(qurl_finfo);
}

#define M_DATASTREAM_FIELDS(X) \
	X(HREF, m_url) \
	X(TS_LAST_REFRESH, m_timestamp_last_refresh) \
	X(SIZE_FILE, m_file_size_bytes) \
	X(TS_CREATION, m_creation_timestamp) \
	X(TS_LAST_MODIFIED, m_last_modified_timestamp) \
	X(TS_LAST_MODIFIED_METADATA, m_metadata_last_modified_timestamp)

/// Strings to use for the tags.
#define X(field_tag, member_field) static const QLatin1String field_tag ( # member_field );
	M_DATASTREAM_FIELDS(X);
#undef X

QVariant ExtUrl::toVariant() const
{
	InsertionOrderedMap<QString, QVariant> map;

    set_map_class_info(this, &map);

	// Add all the fields to the map.
#define X(field_tag, field)   map_insert_or_die(map, field_tag, field);
	M_DATASTREAM_FIELDS(X)
#undef X

	return map;
}

void ExtUrl::fromVariant(const QVariant& variant)
{
	InsertionOrderedMap<QString, QVariant> map = variant.value<InsertionOrderedMap<QString, QVariant>>();
	// qviomap_from_qvar_or_die(&map, variant);

	// Extract all the fields from the map, cast them to their type.
#define X(field_tag, field)    map_read_field_or_warn(map, field_tag, &field);
	M_DATASTREAM_FIELDS(X)
#undef X
}

void ExtUrl::save_mod_info(const QFileInfo* qurl_finfo)
{
	Q_CHECK_PTR(qurl_finfo);

	// Save the last-refresh time.
	/// @todo This is sort of not right, it really should be passed in from the qurl_finfo creator.
	m_timestamp_last_refresh = QDateTime::currentDateTimeUtc();

	if(qurl_finfo != nullptr)
	{
		// Should never be nullptr here.
		m_file_size_bytes = qurl_finfo->size();
		QDateTime dt_filetime_birth = qurl_finfo->fileTime(QFileDevice::FileBirthTime);
		QDateTime dt_finfo_birth = qurl_finfo->birthTime();
		Q_ASSERT(dt_filetime_birth == dt_finfo_birth);
		m_creation_timestamp = qurl_finfo->birthTime();
		m_last_modified_timestamp = qurl_finfo->lastModified();
		m_metadata_last_modified_timestamp = qurl_finfo->metadataChangeTime();
	}
}

void ExtUrl::load_mod_info(const QFileInfo* qurl_finfo)
{
	Q_ASSERT(m_url.isValid());

	if(qurl_finfo != nullptr)
	{
		// We already have the info, save off what we want to keep.
		save_mod_info(qurl_finfo);
	}
	else
	{
		// We don't have the QFileInfo, load it.
		// Is this a local file?
		if(m_url.isLocalFile())
		{
			// Yes, we can get the mod info fairly cheaply.
			QFileInfo fi(m_url.toLocalFile());
			if(fi.exists())
			{
				// File exists.
				save_mod_info(&fi);
			}
		}
	}
}

QDebug operator<<(QDebug dbg, const ExtUrl& obj) // NOLINT(performance-unnecessary-value-param)
{
#define X(unused, field) << obj.field
	dbg M_DATASTREAM_FIELDS(X);
#undef X
	return dbg;
}

QDataStream& operator<<(QDataStream& out, const ExtUrl& myObj)
{
#define X(unused, field) << myObj.field
	out M_DATASTREAM_FIELDS(X);
#undef X
	return out;
}

QDataStream& operator>>(QDataStream& in, ExtUrl& myObj)
{
#define X(unused, field) >> myObj.field
	return in M_DATASTREAM_FIELDS(X);
#undef X
}


#undef M_DATASTREAM_FIELDS
