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
 * @file XmlObjects.cpp
 */

#include "XmlObjects.h"

// Qt5
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtXmlPatterns/QXmlFormatter>

// Ours
#include <utils/DebugHelpers.h>
#include "SerializationExceptions.h"


bool run_xquery(const QUrl& xquery_url, const QUrl& source_xml_url, const QUrl& dest_xml_url,
		const std::function<void(QXmlQuery*)>& bind_callback)
{
	// Open the file containing the XQuery (could be in our resources).
	QFile xquery_file(xquery_url.toLocalFile());
	bool status = xquery_file.open(QIODevice::ReadOnly);
	if(!status)
	{
		throw std::runtime_error("Couldn't open input file");
	}

	// Open the output file.
	QFile outfile(dest_xml_url.toLocalFile());
	status = outfile.open(QFile::WriteOnly | QFile::Text);
	if(!status)
	{
		throw std::runtime_error(std::string("Couldn't open output file for writing"));
	}

	// Create the QXmlQuery, bind variables, and load the xquery.

	QXmlQuery the_query;

	// Bind the source XML URL to the variable used in the XQuery file.
	Q_ASSERT(source_xml_url.isValid());
	the_query.bindVariable("input_file_path", QVariant(source_xml_url.toLocalFile()));

	// Call any bind_callback the caller provided.
	bind_callback(&the_query);

	// Read the XQuery as a QString.
	const QString query_string(QString::fromLatin1(xquery_file.readAll()));
	// Set the_query.
	the_query.setQuery(query_string);
	Q_ASSERT(the_query.isValid());

	return run_xquery(the_query, dest_xml_url);
}

bool run_xquery(const QUrl& xquery_url, const QUrl& source_xml_url, QStringList* out_stringlist,
                const std::function<void(QXmlQuery*)>& bind_callback)
{
	// Open the file containing the XQuery (could be in our resources).
	QFile xquery_file(xquery_url.toLocalFile());
	bool status = xquery_file.open(QIODevice::ReadOnly);
	if(!status)
	{
		throw std::runtime_error("Couldn't open xquery source file");
	}

	// Create the QXmlQuery, bind variables, and load the xquery.

	QXmlQuery the_query;

	// Bind the source XML URL to the variable used in the XQuery file.
	Q_ASSERT(source_xml_url.isValid());
	the_query.bindVariable("input_file_path", QVariant(source_xml_url.toLocalFile()));

	// Call any bind_callback the caller provided.
	bind_callback(&the_query);

	// Read in the XQuery as a QString.
	const QString query_string(QString::fromLatin1(xquery_file.readAll()));
	// Set the_query.
	the_query.setQuery(query_string);
	Q_ASSERT(the_query.isValid());

	// Run the prepared query.
	return run_xquery(the_query, source_xml_url, out_stringlist);
}

bool run_xquery(const QUrl& xquery_url, QIODevice* xml_source, QIODevice* xml_sink,
                const std::function<void(QXmlQuery*)>& bind_callback)
{
	// Open the file containing the XQuery (could be in our resources).
	QFile xquery_file(xquery_url.toLocalFile());
	bool status = xquery_file.open(QIODevice::ReadOnly);
	if(!status)
	{
		throw std::runtime_error("Couldn't open xquery source file");
	}

	// Create the QXmlQuery, bind variables, and load the xquery.

	QXmlQuery the_query;

	// Call any bind_callback the caller provided.
	bind_callback(&the_query);

	// Read in the XQuery as a QString.
	const QString query_string(QString::fromLatin1(xquery_file.readAll()));
	// Set the_query.
	// @note This is correct, var binding should be before the setQuery() call.
	the_query.setQuery(query_string);
	Q_ASSERT(the_query.isValid());

	return run_xquery(the_query, xml_source, xml_sink);
}

//bool run_xquery(const QUrl& xquery_url, const QUrl& source_xml_url, QString* out_string);
///// Serialize to a QAbstractXmlReceiver, e.g. QXmlSerializer.
//bool run_xquery(const QUrl& xquery_url, const QUrl& source_xml_url, QAbstractXmlReceiver *callback);
//bool run_xquery(const QUrl& xquery_url, const QUrl& source_xml_url, QXmlResultItems *result);

/**
 * Run @a xquery, putting results in @a out_stringlist.
 * @a xquery must already have all bindings in place.
 * @a source_xml_url is not used and will probably be removed.
 */
bool run_xquery(const QXmlQuery& xquery, const QUrl& dest_xml_url)
{
	// Open the output file.
	QFile outfile(dest_xml_url.toLocalFile());
	bool status = outfile.open(QFile::WriteOnly | QFile::Text);
	if(!status)
	{
		throw std::runtime_error("Couldn't open output file");
	}

	// Formatter when we want to write out another XML file.
	QXmlFormatter formatter(xquery, &outfile);
	formatter.setIndentationDepth(2);

	// Run the QXmlQuery.
	bool retval = xquery.evaluateTo(&formatter);

	return retval;
}

/**
 * Run @a xquery, putting results in @a out_stringlist.
 * @a xquery must already have all bindings in place.
 * @a source_xml_url is not used and will probably be removed.
 */
bool run_xquery(const QXmlQuery& xquery, const QUrl& source_xml_url, QStringList* out_stringlist)
{
	if(!xquery.isValid())
	{
		throw std::runtime_error("invalid QXmlQuery");
	}

	// Output is going to a QStringList.
	// Run the QXmlQuery.
	bool retval = xquery.evaluateTo(out_stringlist);

	return retval;
}

bool run_xquery(const QXmlQuery& xquery, QIODevice* xml_source, QIODevice* xml_sink)
{
	if(!xquery.isValid())
	{
		throw SerializationException("invalid QXmlQuery");
	}

	QXmlSerializer serializer(xquery, xml_sink);

	bool retval = xquery.evaluateTo(&serializer);

	return retval;
}

//bool run_xquery(const QXmlQuery& xquery, const QUrl& xml_source_url, QString* out_string);
//bool run_xquery(const QXmlQuery& xquery, const QUrl& xml_source_url, QAbstractXmlReceiver *callback);
//bool run_xquery(const QXmlQuery& xquery, const QUrl& xml_source_url, QXmlResultItems *result);



//void XmlElement::append(std::unique_ptr<XmlElement> child)
//{
//	m_child_elements.push_back(std::move(child));
//}

void XmlElement::write(QXmlStreamWriter* out) const
{
	// Write the start element with tag name
	out->writeStartElement(m_tagname);

	// Attributes (id)
	if(!m_id.name().isEmpty())
	{
		out->writeAttribute(m_id);
	}
	// Attributes
	if(!m_attributes.empty())
	{
		out->writeAttributes(m_attributes);
	}
	// The single value.
	if(!m_value.isEmpty() && !m_value.isNull())
	{
		out->writeCharacters(m_value);
	}

	// Do whatever's in the inner scope lambda.
	if(m_inner_scope)
	{
		m_inner_scope(const_cast<XmlElement*>(this), out);
	}

	// Write out any child elements.
	for(auto it = m_child_elements.cbegin(); it != m_child_elements.cend(); ++it)
	{
//		qDb() << "Writing child element";
		it->write(out);
	}

	// End element.
	out->writeEndElement();
}

void XmlElementList::write(QXmlStreamWriter* out) const
{
	// Make sure the elements are written out in the order they were added.
	for(auto& e : *this)
	{
		e.write(out);
	}
}


/// This is super ridiculous.  Can't get a string of a QDateTime with the correct timezone offset.
/// @link https://bugreports.qt.io/browse/QTBUG-26161
/// @link https://bugreports.qt.io/browse/QTBUG-26161?focusedCommentId=227469&page=com.atlassian.jira.plugin.system.issuetabpanels:comment-tabpanel#comment-227469
QString toISOTime(const QDateTime& dt)
{
//	QDateTime local_dt_copy = dt;
//	QDateTime utc = dt.toUTC();
//	utc.setTimeSpec(Qt::LocalTime);
//	int utcoffset = utc.secsTo(dt);
//	local_dt_copy.setUtcOffset(utcoffset);

//	return local_dt_copy.toString(Qt::ISODate);
	return dt.toOffsetFromUtc(dt.offsetFromUtc()).toString(Qt::ISODate);
}

XmlValue::XmlValue(const QDateTime& dt) : QString(toISOTime(dt)) {}



