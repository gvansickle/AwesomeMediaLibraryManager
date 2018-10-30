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

// Ours
#include "models/ScanResultsTreeModel.h"

#include <logic/models/AbstractTreeModelWriter.h>


AMLM_QREG_CALLBACK([](){
	qIn() << "Registering DirScanResult";
	qRegisterMetaType<DirScanResult>();
});


DirScanResult::DirScanResult(const QUrl &found_url, const QFileInfo &found_url_finfo)
	: m_media_exturl(found_url, &found_url_finfo)
{
	determineDirProps(found_url_finfo);
}

AbstractTreeModelItem* DirScanResult::toTreeModelItem()
{
//	QVector<QVariant> column_data;
//	column_data.append(QVariant::fromValue<DirProps>(getDirProps()).toString());
//	column_data.append(QVariant::fromValue(getMediaExtUrl().m_url.toDisplayString()));
//	column_data.append(QVariant::fromValue(getSidecarCuesheetExtUrl().m_url.toDisplayString()));

	auto new_item = new ScanResultsTreeModelItem(this);
//	auto new_item = make_default_node(column_data);

#warning "EXPERIMENTAL, REMOVE"
//    QVector<AbstractTreeModelItem *> child_items;
//	AbstractTreeModelItem* child_item = new ScanResultsTreeModelItem({"One", "Two", "Three"}, new_item);

//    child_items.push_back(child_item);

//    new_item->appendChildren(child_items);

	return new_item;
}

XmlElement DirScanResult::toXml() const
{
	/// @todo
	int id = qrand();

	XmlElement retval("dirscanresult",
	/*XmlAttributeList(*/{{"id", id}}/*)*/, // Attribute list
	XmlValue(), // value
	// Child XmlElements.
	{
		XmlElement("flags_dirprops", toqstr(m_dir_props)),
		 m_dir_exturl.toXml().setId("exturl_dir"),
		m_media_exturl.toXml().setId("exturl_media"),
		m_cue_exturl.toXml().setId("exturl_cuesheet")},
				 [=](auto* e, auto* xml){
		;}
	);

	return retval;
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

/**
 * QXmlStreamWriter write operator.
 */
QXmlStreamWriter& operator<<(QXmlStreamWriter& out, const DirScanResult& dsr)
{

//	out.writeStartElement("dirscanresult");
	// Directory URL.

	auto e = dsr.toXml();

	out << e;
	return out;

//	{
//		XmlElement e("dirscanresult",
//					 [=](auto* e, auto* xml){
//			// Append attributes to the outer element.
//			e->setId(1);

//			// Append child elements.
////			e->

//			// Write all the child elements.
//			auto dir_url {dsr.m_dir_exturl.toXml()};
//			auto cue_url {dsr.m_cue_exturl.toXml()};

//			dir_url.setId("dir_exturl");
//			cue_url.setId("cuesheet_url");

//			dir_url.write(xml);
//			cue_url.write(xml);
//			;});//, XmlAttributeList({QXmlStreamAttribute("dir_exturl", dsr.m_dir_exturl)}));
//		e.set_out(&out);
////		XmlElement dir_url(out, "m_dir_exturl", dsr.m_dir_exturl.m_url);
////		out << dsr.m_dir_exturl;
////		out << dsr.m_cue_exturl;
//	}

////	out << dsr.m_dir_exturl;
////	out << dsr.m_cue_exturl;
////	qDb() << dsr.m_dir_exturl;
////	qDb() << dsr.m_cue_exturl;
////	out.writeAttribute("href", exturl.m_url.toString());
////	out.writeTextElement("title", "Media URL");
////	out.writeEndElement();
//	//delete e;
//	return out;
}

#undef DATASTREAM_FIELDS


