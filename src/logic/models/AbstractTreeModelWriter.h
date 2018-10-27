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
 * @file AbstractTreeModelWriter.h
 */
#ifndef SRC_LOGIC_MODELS_ABSTRACTTREEMODELWRITER_H_
#define SRC_LOGIC_MODELS_ABSTRACTTREEMODELWRITER_H_

// Qt5
#include <QXmlStreamWriter>
class QIODevice;

// Ours
//#include "AbstractTreeModel.h"
//#include "AbstractTreeModelItem.h"
class AbstractTreeModel;
class AbstractTreeModelItem;

/**
 *
 */
class AbstractTreeModelWriter
{
public:
	explicit AbstractTreeModelWriter(const AbstractTreeModel* model);
	virtual ~AbstractTreeModelWriter();

	bool write_to_iodevice(QIODevice* device);

private:

	void write_item(const AbstractTreeModelItem* item);

	QXmlStreamWriter m_xml_stream_writer;

	/// @todo This should be a shared pointer, or we shouldn't store it.
	const AbstractTreeModel* m_tree_model;
};



class XmlAttributeList : public QXmlStreamAttributes
{
public:
	XmlAttributeList(/*std::initializer_list*/QVector<QXmlStreamAttribute> initlist) : QXmlStreamAttributes()
	{
		this->append(initlist);
	}
};

/**
 * RAII/convenience class for XML elements.
 */
class XmlElement
{
public:
	// No attributes.
	explicit XmlElement(QXmlStreamWriter& out, QString tagname) : m_out(out), m_tagname(tagname)
	{
		// Tag name
		out.writeStartElement(m_tagname);
	};
	explicit XmlElement(QXmlStreamWriter& out, QString tagname, QXmlStreamAttributes attrs)
		: m_out(out), m_tagname(tagname), m_attributes(attrs)
	{
		out.writeStartElement(m_tagname);
		out.writeAttributes(m_attributes);
	};
	virtual ~XmlElement() { m_out.writeEndElement(); };
protected:
	QXmlStreamWriter& m_out;
	QString m_tagname;
	QXmlStreamAttributes m_attributes;
};

#endif /* SRC_LOGIC_MODELS_ABSTRACTTREEMODELWRITER_H_ */
