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

/// Qt5
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

/// Ours, Qt5/KF5-related
#include <utils/TheSimplestThings.h>
#include <utils/RegisterQtMetatypes.h>


AMLM_QREG_CALLBACK([](){
	qIn() << "Registering ExtUrl, DirScanResult, FileModificationInfo";
	qRegisterMetaType<ExtUrl>();
	qRegisterMetaTypeStreamOperators<ExtUrl>();
    qRegisterMetaType<DirScanResult>();
    qRegisterMetaType<FileModificationInfo>();
});


ExtUrl::ExtUrl(const QUrl& qurl, const QFileInfo* qurl_finfo) : m_url(qurl)
{
	if(qurl_finfo != nullptr)
	{
		m_size = qurl_finfo->size();
		m_last_modified_timestamp = qurl_finfo->lastModified();
		m_metadata_last_modified_timestamp = qurl_finfo->metadataChangeTime();
	}
	else
	{
		LoadModInfo();
	}
}

void ExtUrl::LoadModInfo()
{
	Q_ASSERT(m_url.isValid());

	// Is this a local file?
	if(m_url.isLocalFile())
	{
		// Yes, we can get the mod info fairly cheaply.
		QFileInfo fi(m_url.toLocalFile());
		if(fi.exists())
		{
			// File exists.
			m_size = fi.size();
			m_last_modified_timestamp = fi.lastModified();
			m_metadata_last_modified_timestamp = fi.metadataChangeTime();
		}
	}
}

#define DATASTREAM_FIELDS(X) \
	X(m_url) X(m_size) X(m_last_modified_timestamp) X(m_metadata_last_modified_timestamp)

QDebug operator<<(QDebug dbg, const ExtUrl& obj)
{
#define X(field) << obj.field
	dbg DATASTREAM_FIELDS(X);
#undef X
	return dbg;
}

QDataStream &operator<<(QDataStream &out, const ExtUrl& myObj)
{
#define X(field) << myObj.field
	out DATASTREAM_FIELDS(X);
#undef X
	return out;
}

QDataStream &operator>>(QDataStream &in, ExtUrl& myObj)
{
#define X(field) >> myObj.field
	return in DATASTREAM_FIELDS(X);
#undef X
}

#undef DATASTREAM_FIELDS

DirScanResult::DirScanResult(const QUrl &found_url, const QFileInfo &found_url_finfo)
	: m_media_exturl(found_url, &found_url_finfo)
{
    determineDirProps(found_url_finfo);
}

AbstractTreeModelItem* DirScanResult::toTreeModelItem()
{
    QVector<QVariant> column_data;
    column_data.append(QVariant::fromValue<DirProps>(getDirProps()).toString());
    column_data.append(QVariant::fromValue(getMediaExtUrl().m_url.toDisplayString()));
    column_data.append(QVariant::fromValue(getSidecarCuesheetExtUrl().m_url.toDisplayString()));

    return new AbstractTreeModelItem(column_data);
}

void DirScanResult::determineDirProps(const QFileInfo &found_url_finfo)
{
    // Separate out just the directory part of the URL.
#if 0 // DELETE THIS
    if(false) // local file
    {
        QDir dir_url_qdir = found_url_finfo.dir();
        m_dir_url = QUrl::fromLocalFile(dir_url_qdir.absolutePath());
    }
	else
#endif
	// Works for any URL.
    {
		m_dir_exturl = m_media_exturl.m_url.adjusted(QUrl::RemoveFilename);
    }

    // Is there a sidecar cue sheet?

    // Create the *.cue URL.
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
			m_cue_exturl = possible_cue_url;
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

#define DATASTREAM_FIELDS(X) \
	X(m_dir_exturl) X(m_dir_props) X(m_media_exturl) X(m_cue_exturl)

QDebug operator<<(QDebug dbg, const DirScanResult & obj)
{
#define X(field) << obj.field
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

#undef DATASTREAM_FIELDS


