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
 * @file AbstractTreeModelWriter.cpp
 */

#include "AbstractTreeModelWriter.h"

// Qt5
#include <QDateTime>

// Ours
#include "AbstractTreeModel.h"
#include "AbstractTreeModelItem.h"
#include "AbstractTreeModelReader.h"
#include "../xml/XmlObjects.h"

static inline QString yesValue() { return QStringLiteral("yes"); }
static inline QString noValue() { return QStringLiteral("no"); }
static inline QString titleElement() { return QStringLiteral("title"); }

AbstractTreeModelWriter::AbstractTreeModelWriter(const AbstractTreeModel* model) : m_tree_model(model)
{
	m_xml_stream_writer.setAutoFormatting(true);
}

AbstractTreeModelWriter::~AbstractTreeModelWriter()
{

}

bool AbstractTreeModelWriter::write_to_iodevice(QIODevice* device)
{
	// Convenience ref.
	auto& xml = m_xml_stream_writer;

	xml.setDevice(device);

	xml.writeStartDocument();

	/// @todo Move out of this class. Start of xspf-specific stuff.
	XmlElement playlist("playlist", [=](XmlElement* e, QXmlStreamWriter* out){
		auto& xml = *out;
		xml.writeDefaultNamespace("http://xspf.org/ns/0/");
		xml.writeAttribute("version", "1");
		xml.writeNamespace("http://amlm/ns/0/", "amlm"); // Our extension namespace.
//		xml.writeAttribute("version", "1");

	// No DTD for xspf.

	/// @todo Playlist metadata here.  Needs to be moved out of this class.
	/// http://www.xspf.org/xspf-v1.html#rfc.section.2.3.1
	/// <title> "A human-readable title for the playlist. xspf:playlist elements MAY contain exactly one."
	xml.writeTextElement("title", "XSPF playlist title goes HERE");

	/// <creator> "Human-readable name of the entity (author, authors, group, company, etc) that authored the playlist. xspf:playlist elements MAY contain exactly one."
	xml.writeTextElement("creator", "XSPF playlist CREATOR GOES HERE");

	/// ...
	/// <date>	"Creation date (not last-modified date) of the playlist, formatted as a XML schema dateTime. xspf:playlist elements MAY contain exactly one.
	///	A sample date is "2005-01-08T17:10:47-05:00".
	xml.writeTextElement("date", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

#if 1

	/// @todo Probably get from derived model class?
//	xml.writeStartElement(m_tree_model->getXmlStreamName());
//	xml.writeAttribute(AbstractTreeModelReader::versionAttribute(), m_tree_model->getXmlStreamVersion());

	m_tree_model->writeModel(out);
#elif 0

	// Write out the top-level items.
	for(long i = 0; i < m_tree_model->rowCount(); ++i)
	{
		QModelIndex qmi = m_tree_model->index(i, 0);
		write_item(m_tree_model->getItem(qmi));
	}
#elif 0
	// Write out the top-level items.
	for(long i = 0; i < m_tree_model->rowCount(); ++i)
	{
		QModelIndex qmi = m_tree_model->index(i, 0);
		QXmlQuery = write(m_tree_model->getItem(qmi));
	}
#endif

	xml.writeEndDocument();
	});
	playlist.write(&xml);
	return true;
}

void AbstractTreeModelWriter::write_item(const AbstractTreeModelItem* item)
{
	// Convenience ref.
	auto& xml = m_xml_stream_writer;

	//item->data();
	QString item_tag_name = "abstracttreemodelitem";

	xml.writeStartElement(item_tag_name);
	xml.writeAttribute("childNumber", QString("%1").arg(item->childNumber()));
	// Write out this item.
	/// @todo Again this should be fobbed off on the derived model/item somehow.  I think.
	for(int col = 0; col < item->columnCount(); ++col)
	{
		xml.writeTextElement(titleElement(), item->data(col).toString());
	}

	// Write out all children.
	for(int i = 0; i < item->childCount(); ++i)
	{
		// Hold on tight, we're going recursive!
		write_item(item->child(i));
	}
	xml.writeEndElement();
}

