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

/**
 * @file ExtUrl.cpp
 */

#include "ExtUrl.h"

// Qt5
#include <QFileInfo>
#include <QXmlStreamWriter>
#include <QtXmlPatterns>

// Ours, Qt5 Support
#include <utils/RegisterQtMetatypes.h>

// Ours
#include "models/AbstractTreeModelWriter.h"
#include <logic/models/ScanResultsTreeModelXMLTags.h>
#include "xml/XmlObjects.h"
#include <utils/DebugHelpers.h>



AMLM_QREG_CALLBACK([](){
	qIn() << "Registering ExtUrl";
	qRegisterMetaType<ExtUrl>();
//	qRegisterMetaTypeStreamOperators<ExtUrl>("ExtUrl");
});


ExtUrl::ExtUrl(const QUrl& qurl, const QFileInfo* qurl_finfo) : m_url(qurl)
{
	// Save mod info, possibly loading it from the filesystem if we don't have it in qurl_finfo.
	LoadModInfo(qurl_finfo);
}

#define DATASTREAM_FIELDS(X) \
	X(href, m_url) \
	X(ts_last_refresh, m_timestamp_last_refresh) \
	X(file_size, m_size) \
	X(ts_creation, m_creation_timestamp) \
	X(ts_last_modified, m_last_modified_timestamp) \
	X(ts_last_modified_metadata, m_metadata_last_modified_timestamp)


QVariant ExtUrl::toVariant() const
{
	QVariantMap map;

	// Add all the fields to the map.
#define X(field_enum_name, field) map.insert( /*ExtUrlTag[*/ # field_enum_name /*]*/ , field );
	DATASTREAM_FIELDS(X)
#undef X
	return map;
}

void ExtUrl::fromVariant(const QVariant& variant)
{
	QVariantMap map = variant.toMap();

	// Extract all the fields from the map, cast them to their type.
#define X(field_name, field) field = map.value( # field_name ).value<decltype( field )>();
	DATASTREAM_FIELDS(X)
#undef X
}

XmlElement ExtUrl::toXml() const
{
	// Mostly elements format.
	XmlElementList el = {
		XmlElement("href", m_url),
		XmlElement("file_size", m_size),
		XmlElement("ts_last_refresh", m_timestamp_last_refresh),
		XmlElement("ts_creation", m_creation_timestamp),
		XmlElement("ts_last_modified", m_last_modified_timestamp),
		XmlElement("ts_last_modified_metadata", m_metadata_last_modified_timestamp)
		};

	XmlElement retval("exturl",
					  XmlAttributeList(),
					  XmlValue(),
					  el,
					  [=](XmlElement* e, QXmlStreamWriter* xml){
//		qDb() << "callback";
	});

	return retval;
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
		m_size = qurl_finfo->size();
		QDateTime dt_filetime_birth = qurl_finfo->fileTime(QFileDevice::FileBirthTime);
		QDateTime dt_finfo_birth = qurl_finfo->birthTime();
		Q_ASSERT(dt_filetime_birth == dt_finfo_birth);
		m_creation_timestamp = qurl_finfo->birthTime();
		m_last_modified_timestamp = qurl_finfo->lastModified();
		m_metadata_last_modified_timestamp = qurl_finfo->metadataChangeTime();
	}
}

void ExtUrl::LoadModInfo(const QFileInfo* qurl_finfo)
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

QDebug operator<<(QDebug dbg, const ExtUrl& obj)
{
#define X(unused, field) << obj.field
	dbg DATASTREAM_FIELDS(X);
#undef X
	return dbg;
}

QDataStream& operator<<(QDataStream& out, const ExtUrl& myObj)
{
#define X(unused, field) << myObj.field
	out DATASTREAM_FIELDS(X);
#undef X
	return out;
}

QDataStream& operator>>(QDataStream& in, ExtUrl& myObj)
{
#define X(unused, field) >> myObj.field
	return in DATASTREAM_FIELDS(X);
#undef X
}



/**
 * QXmlStreamWriter write operator.
 */
//QXmlStreamWriter& operator<<(QXmlStreamWriter& out, const ExtUrl& exturl)
//{
//	auto e = exturl.toXml();
//	e.write(&out);

//	return out;
//}


#undef DATASTREAM_FIELDS
