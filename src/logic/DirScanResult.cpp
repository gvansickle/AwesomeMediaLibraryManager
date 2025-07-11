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

/// @file

#include "DirScanResult.h"

#include <config.h>

// Qt
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

// Ours, Qt/KF-related
#include <utils/TheSimplestThings.h>
#include <utils/RegisterQtMetatypes.h>

// Ours
// #include "models/ScanResultsTreeModel.h"
#include <utils/EnumFlagHelpers.h>
#include <logic/serialization/SerializationHelpers.h>


AMLM_QREG_CALLBACK([](){
	qIn() << "Registering DirScanResult";
	qRegisterMetaType<DirScanResult>();
	qRegisterMetaType<DirScanResult::DirPropFlags>("DirScanResult::DirPropFlags");
	AMLMRegisterQFlagQStringConverters<DirScanResult::DirPropFlags>();
});


DirScanResult::DirScanResult(const QUrl &found_url, const QFileInfo &found_url_finfo)
	: m_exturl_media(found_url, &found_url_finfo)
{
	determineDirProps(found_url_finfo);
}

#define M_DATASTREAM_FIELDS(X) \
	X(XMLTAG_FLAGS_DIRPROPS, m_flags_dirprops) \
	X(XMLTAG_HAS_SIDECAR_CUESHEET, m_has_sidecar_cuesheet) \
	X(XMLTAG_HAS_EMBEDDED_CUESHEET, m_has_embedded_cuesheet) \
	X(XMLTAG_EXTURL_DIR, m_exturl_dir_url) \
	X(XMLTAG_EXTURL_MEDIA, m_exturl_media) \
	X(XMLTAG_EXTURL_CUESHEET, m_exturl_cuesheet)

/// Strings to use for the tags.
#define X(field_tag, member_field) static constexpr QLatin1String field_tag ( # member_field );
	M_DATASTREAM_FIELDS(X);
#undef X

QVariant DirScanResult::toVariant() const
{
	InsertionOrderedMap<QString, QVariant> map;

	// Set the xml:id.
	map.insert_attributes({{"xml:id", get_prefixed_uuid()}});

	// Add all the fields to the map.
#define X(field_tag, member_field) map_insert_or_die(map, field_tag, member_field);
    M_DATASTREAM_FIELDS(X)
#undef X

	return map;
}

void DirScanResult::fromVariant(const QVariant& variant)
{
	InsertionOrderedMap<QString, QVariant> map = variant.value<InsertionOrderedMap<QString, QVariant>>();

	auto uuid = map.get_attr("xml:id", "");
	set_prefixed_uuid(uuid);

	// Extract all the fields from the map.
#define X(field_tag, member_field) map_read_field_or_warn(map, field_tag, &(member_field));
    M_DATASTREAM_FIELDS(X)
#undef X
}

void DirScanResult::determineDirProps(const QFileInfo &found_url_finfo)
{
    // Separate out just the directory part of the URL.
	// Works for any URL.
	QUrl m_dir_url = m_exturl_media.m_url.adjusted(QUrl::RemoveFilename);
	QFileInfo fi(m_dir_url.toString());
	m_exturl_dir_url = ExtUrl(m_dir_url, &fi);

    // Is there a sidecar cue sheet?

	// Create the URL the *.cue file would have.
	ExtUrl possible_cue_url;
	possible_cue_url = QUrl(m_exturl_media);
	QString cue_url_as_str = possible_cue_url.m_url.toString();
    Q_ASSERT(!cue_url_as_str.isEmpty());
    static QRegularExpression re("\\.[[:alnum:]]+$");
    cue_url_as_str.replace(re, ".cue");
    possible_cue_url = cue_url_as_str;
	Q_ASSERT(possible_cue_url.m_url.isValid());

	// Does the possible cue sheet file actually exist?
    if(true /** @todo local file*/)
    {
		QFileInfo fi(possible_cue_url.m_url.toLocalFile());
        if(fi.exists())
        {
            // It's there.
			// Set the flag and the path relative to the directory (should be just the filename).
			m_exturl_cuesheet = ExtUrl(possible_cue_url.m_url, &fi);
			m_flags_dirprops |= HasSidecarCueSheet;
	        m_has_sidecar_cuesheet = true;
        }
        else
        {
        	m_has_sidecar_cuesheet = false;
        }
    }
    else
    {
        Q_ASSERT_X(0, "dirprops", "NOT IMPLEMENTED: Non-local determination of sidecar cue files.");
    }
}

QVector<ExtUrl> DirScanResult::otherMediaFilesInDir(const QFileInfo& finfo)
{
    // Get the parent directory.
    auto dir = finfo.dir();
Q_ASSERT(0);
// M_WARNING("TODO");
    return QVector<ExtUrl>();
}

template <class stream>
stream operator<<(stream s, std::optional<bool> val)
{
	s << val.value_or("unknown");
	return s;
}

QDebug operator<<(QDebug dbg, const DirScanResult & obj) // NOLINT(performance-unnecessary-value-param)
{
#define X(ignore, field) << obj.field
	dbg M_DATASTREAM_FIELDS(X);
#undef X
    return dbg;
}

#if 0
QDataStream &operator<<(QDataStream &out, const DirScanResult & myObj)
{
#define X(field) << myObj.field
	out M_DATASTREAM_FIELDS(X);
#undef X
    return out;
}

QDataStream &operator>>(QDataStream &in, DirScanResult & myObj)
{
#define X(field) >> myObj.field
	return in M_DATASTREAM_FIELDS(X);
#undef X
}
#endif


