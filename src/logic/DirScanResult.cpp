/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "DirScanResult.h"

#include <config.h>

// Qt5
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

// Ours, Qt5/KF5-related
#include <utils/TheSimplestThings.h>
#include <utils/RegisterQtMetatypes.h>

// Ours
#include "models/ScanResultsTreeModel.h"
#include "models/ScanResultsTreeModelXMLTags.h"
#include <utils/EnumFlagHelpers.h>
#include <logic/xml/ExtEnum.h>

AMLM_QREG_CALLBACK([](){
	qIn() << "Registering DirScanResult";
//	qRegisterMetaType<DirScanResult>();
	qRegisterMetaType<DirScanResult::DirPropFlags>("DirScanResult::DirPropFlags");
	AMLMRegisterQFlagQStringConverters<DirScanResult::DirPropFlags>();
});



DirScanResult::DirScanResult(const QUrl &found_url, const QFileInfo &found_url_finfo)
	: m_media_exturl(found_url, &found_url_finfo)
{
	determineDirProps(found_url_finfo);
}

#define DATASTREAM_FIELDS(X) \
	/*X(flags_dirprops, m_dir_props)*/ \
	X(exturl_dir, m_dir_exturl) \
	X(exturl_media, m_media_exturl) \
	X(exturl_cuesheet, m_cue_exturl)



QVariant DirScanResult::toVariant() const
{
	QVariantMap map;

	// Add all the fields to the map.
	map.insert(DSRTagToXMLTagMap[DSRTag::EXTURL_DIR], m_dir_exturl.toVariant());
	map.insert(DSRTagToXMLTagMap[DSRTag::EXTURL_MEDIA], m_media_exturl.toVariant());
	map.insert(DSRTagToXMLTagMap[DSRTag::EXTURL_CUESHEET], m_cue_exturl.toVariant());

	return map;
}

void DirScanResult::fromVariant(const QVariant& variant)
{
	QVariantMap map = variant.toMap();

	// Extract all the fields from the map, cast them to their type.

	/// @todo Something is still broken here.  This should work, but it doesn't:
//	m_media_exturl = map.value("exturl_media").value<ExtUrl>();

	QVariant exturl_in_variant = map.value(DSRTagToXMLTagMap[DSRTag::EXTURL_MEDIA]);
	m_media_exturl.fromVariant(exturl_in_variant);
	exturl_in_variant = map.value(DSRTagToXMLTagMap[DSRTag::EXTURL_DIR]);
	m_dir_exturl.fromVariant(exturl_in_variant);
	exturl_in_variant = map.value(DSRTagToXMLTagMap[DSRTag::EXTURL_CUESHEET]);
	m_cue_exturl.fromVariant(exturl_in_variant);
}

void DirScanResult::determineDirProps(const QFileInfo &found_url_finfo)
{
    // Separate out just the directory part of the URL.
	// Works for any URL.
	QUrl m_dir_url = m_media_exturl.m_url.adjusted(QUrl::RemoveFilename);
	QFileInfo fi(m_dir_url.toString());
	m_dir_exturl = ExtUrl(m_dir_url, &fi);

    // Is there a sidecar cue sheet?

	// Create the URL the *.cue file would have.
	ExtUrl possible_cue_url;
	possible_cue_url = QUrl(m_media_exturl);
	QString cue_url_as_str = possible_cue_url.m_url.toString();
    Q_ASSERT(!cue_url_as_str.isEmpty());
    cue_url_as_str.replace(QRegularExpression("\\.[[:alnum:]]+$"), ".cue");
    possible_cue_url = cue_url_as_str;
	Q_ASSERT(possible_cue_url.m_url.isValid());

	// Does the possible cue sheet file actually exist?
    if(true /** @todo local file*/)
    {
		QFileInfo fi(possible_cue_url.m_url.toLocalFile());
        if(fi.exists())
        {
            // It's there.
			m_cue_exturl = ExtUrl(possible_cue_url.m_url, &fi);
            m_dir_props |= HasSidecarCueSheet;
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
M_WARNING("TODO");
    return QVector<ExtUrl>();
}

QDebug operator<<(QDebug dbg, const DirScanResult & obj) // NOLINT(performance-unnecessary-value-param)
{
#define X(ignore, field) << obj.field
    dbg DATASTREAM_FIELDS(X);
#undef X
    return dbg;
}

#if 0
QDataStream &operator<<(QDataStream &out, const DirScanResult & myObj)
{
#define X(field) << myObj.field
    out DATASTREAM_FIELDS(X);
#undef X
    return out;
}

QDataStream &operator>>(QDataStream &in, DirScanResult & myObj)
{
#define X(field) >> myObj.field
    return in DATASTREAM_FIELDS(X);
#undef X
}
#endif

