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

// Ours, Qt5 Support
#include <utils/RegisterQtMetatypes.h>

// Ours
#include <utils/DebugHelpers.h>

AMLM_QREG_CALLBACK([](){
	qIn() << "Registering ExtUrl";
	qRegisterMetaType<ExtUrl>();
	qRegisterMetaTypeStreamOperators<ExtUrl>("ExtUrl");
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

/**
 * QXmlStreamWriter write operator.
 */
QXmlStreamWriter& operator<<(QXmlStreamWriter& out, const ExtUrl& exturl)
{
	out.writeStartElement("exturl");
	out.writeAttribute("href", exturl.m_url.toString());
	out.writeTextElement("title", "Media URL");
	out.writeEndElement();
	return out;
}


#undef DATASTREAM_FIELDS
