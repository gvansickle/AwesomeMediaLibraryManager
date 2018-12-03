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

#include <utils/DebugHelpers.h>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtXmlPatterns/QXmlFormatter>


bool run_xquery(const QUrl& xquery_url, const QUrl& source_xml_url, const QUrl& dest_xml_url)
{
	// Open the file containing the XQuery (could be in our resources).
	QFile xquery_file(xquery_url.toLocalFile());
	xquery_file.open(QIODevice::ReadOnly);

	// Open the output file.
	QFile outfile(dest_xml_url.toLocalFile());
	auto status = outfile.open(QFile::WriteOnly | QFile::Text);

	// Create the QXmlQuery, bind variables, and load the xquery.

	QXmlQuery the_query;

	// Bind the source XML URL to the variable used in the XQuery file.
	Q_ASSERT(source_xml_url.isValid());
	the_query.bindVariable("input_file_path", QVariant(source_xml_url.toLocalFile()));

	/// @todo Probably have a lambda here for the caller to bind more variables.

	// Read the XQuery as a QString.
	const QString query_string(QString::fromLatin1(xquery_file.readAll()));
	// Set the_query.
	the_query.setQuery(query_string);
	Q_ASSERT(the_query.isValid());

	// Formatter when we want to write another file.
	QXmlFormatter formatter(the_query, &outfile);
	formatter.setIndentationDepth(2);

	// Run the query_string.
	bool retval = the_query.evaluateTo(&formatter);

	return retval;
}

bool run_xquery(const QUrl& xquery_url, const QUrl& xml_source_url, QStringList* out_stringlist);
bool run_xquery(const QUrl& xquery_url, const QUrl& xml_source_url, QString* out_string);
bool run_xquery(const QUrl& xquery_url, const QUrl& xml_source_url, QIODevice *target);
/// Serialize to a QAbstractXmlReceiver, e.g. QXmlSerializer.
bool run_xquery(const QUrl& xquery_url, const QUrl& xml_source_url, QAbstractXmlReceiver *callback);
bool run_xquery(const QUrl& xquery_url, const QUrl& xml_source_url, QXmlResultItems *result);


bool run_xquery(const QXmlQuery& xquery, const QUrl& xml_source_url, const QUrl& dest_xml_url);
bool run_xquery(const QXmlQuery& xquery, const QUrl& xml_source_url, QStringList* out_stringlist);
bool run_xquery(const QXmlQuery& xquery, const QUrl& xml_source_url, QString* out_string);
bool run_xquery(const QXmlQuery& xquery, const QUrl& xml_source_url, QIODevice *target);
bool run_xquery(const QXmlQuery& xquery, const QUrl& xml_source_url, QAbstractXmlReceiver *callback);
bool run_xquery(const QXmlQuery& xquery, const QUrl& xml_source_url, QXmlResultItems *result);



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



