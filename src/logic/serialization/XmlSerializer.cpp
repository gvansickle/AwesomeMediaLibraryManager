/*
 * Copyright 2018, 2019, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
 * @file XmlSerializer.cpp
 */

#include "XmlSerializer.h"

// Std C++
#include <variant>
// Std C++ from The Future
#include <future/overloaded.h>

// Qt
#include <QFile>
#include <QSaveFile>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QDataStream>
#include <QRegularExpression>

// Ours
#include <utils/DebugHelpers.h>
#include <utils/Stopwatch.h>
#include <future/future_algorithms.h>
#include "ISerializable.h"


void XmlSerializer::save(const ISerializable &serializable, const QUrl &file_url, const QString &root_name,
                         std::function<void(void)> extra_save_actions)
{
	/// @todo file_url Currently only file://'s are supported.

	QString save_file_path = file_url.toLocalFile();
	if(save_file_path.isEmpty())
	{
		Q_ASSERT_X(0, __PRETTY_FUNCTION__, "LOCAL FILE PATH IS EMPTY");
	}

	QSaveFile savefile(save_file_path);

	savefile.open(QIODevice::WriteOnly);

	// XML writing starts here.
	QXmlStreamWriter xmlstream(&savefile);

	xmlstream.setAutoFormatting(true);
	xmlstream.setAutoFormattingIndent(-1);

	// Start document.
	xmlstream.writeStartDocument();

//	if(extra_save_actions)
//	{
//		extra_save_actions();
//	}
	// xmlstream.writeStartElement(str);
	save_extra_start_info(xmlstream);

	writeVariantToStream(root_name, serializable.toVariant(), xmlstream);

	xmlstream.writeEndDocument();

	savefile.commit();
}

bool XmlSerializer::load(ISerializable& serializable, const QUrl &file_url)
{
	Stopwatch sw("###################### XmlSerializer::load()");

	QString load_file_path = file_url.toLocalFile();
	if(load_file_path.isEmpty())
	{
		Q_ASSERT_X(0, __PRETTY_FUNCTION__, "LOCAL FILE PATH IS EMPTY");
	}

	QFile file(load_file_path);
	file.open(QFile::ReadOnly);

#if 0 /// @exp See if reading it all in at once is a win or loss. == It doesn't seem to make a difference.
	QByteArray whole_file = file.readAll();
	if(whole_file.size() == 0)
	{
		qWr() << "##### COULDNT LOAD ENTIRE FILE INTO MEMORY";
		return;
	}
	QXmlStreamReader xmlstream(whole_file);
#else
	QXmlStreamReader xmlstream(&file);
#endif


	/// @todo EXTRA READ INFO NEEDS TO COME FROM CALLER
	// Read the first start element of the document.
	/// @todo Don't just throw it away.
	xmlstream.readNextStartElement();
	qIn() << "First start element tag:" << xmlstream.name();

//	if(xmlstream.readNextStartElement())
//	{
//		qIn() << "First start element tag:" << xmlstream.name() << ", skipping...";
//		xmlstream.skipCurrentElement();
//	}

	if(!m_HACK_SKIP)
	{
//		load_extra_start_info(&xmlstream);
	}

	// Read the first "real" element in the file.
	if(!xmlstream.readNextStartElement())
	{
		// Something went wrong.
		xmlstream.raiseError("Reading first start element failed.");

		/// @todo Handle errors better.
		qWr() << "XML READ ERROR:" << error_string(xmlstream);
	}
	else
	{
		// Stream it all in.
		QVariant qvar = readVariantFromStream(xmlstream);
		serializable.fromVariant(qvar);
	}

	// Reading completed one way or another, check for errors.
	if(xmlstream.hasError())
	{
		qWr() << "#### XML READ ERROR:" << error_string(xmlstream);
	}

	return !xmlstream.error();
}

static const int f_iomap_id = qMetaTypeId<InsertionOrderedMap<QString, QVariant>>();
static const int f_qvarlist_id = qMetaTypeId<QVariantHomogenousList>();
static const int f_serqvarlist_id = qMetaTypeId<SerializableQVariantList>();

void XmlSerializer::writeVariantToStream(const QString &nodeName, const QVariant& variant, QXmlStreamWriter& xmlstream)
{
	xmlstream.writeStartElement(nodeName);
	xmlstream.writeAttribute("type", variant.typeName());

	InnerWriteVariantToStream(variant, &xmlstream);

	xmlstream.writeEndElement();
}

void XmlSerializer::InnerWriteVariantToStream(const QVariant& variant, QXmlStreamWriter* xmlstream)
{
	// Handles the QVariant type dispatch and basically everything between the writing of the node name and type
	// and the end element.

	QMetaType metatype = variant.metaType();
	int metatypeId = metatype.id();

	if(metatypeId == f_iomap_id)
	{
		writeVariantOrderedMapToStream(variant, *xmlstream);
	}
	else if(metatypeId == f_qvarlist_id)
	{
		writeQVariantHomogenousListToStream(variant, *xmlstream);
	}
	else if(metatypeId == f_serqvarlist_id)
	{
		QVariantHomogenousList list = variant.value<QVariantHomogenousList>();
#warning "This looks wrong, on read all lists are coming back a qvarlist"
		writeQVariantHomogenousListToStream(list, *xmlstream);
	}
	else
	{
		switch(metatypeId)
		{
			case QMetaType::QVariantList:
				writeVariantListToStream(variant, *xmlstream);
				break;
			case QMetaType::QVariantMap:
				writeVariantMapToStream(variant, *xmlstream);
				break;
			default:
				writeVariantValueToStream(variant, *xmlstream);
				break;
		}
	}
}


void XmlSerializer::writeQVariantHomogenousListToStream(const QVariant& variant, QXmlStreamWriter& xmlstream)
{
	Q_ASSERT(variant.isValid());
	Q_ASSERT(variant.canConvert<QVariantHomogenousList>());

	QVariantHomogenousList list = variant.value<QVariantHomogenousList>();

//	qDb() << "tags:" << list.get_list_tag() << list.get_list_item_tag();

	auto the_item_tag = list.get_list_item_tag();

	// Stream each QVariant in the list out.
	for(const QVariant& element : list)
	{
		writeVariantToStream(the_item_tag, element, xmlstream);
	}
}

void XmlSerializer::writeVariantListToStream(const QVariant &variant, QXmlStreamWriter& xmlstream)
{
	Q_ASSERT(variant.isValid());
	Q_ASSERT(variant.canConvert<QVariantList>());

	QVariantList list = variant.toList();

	// Stream each QVariant in the list out.
	/// @note tag name will be "item" for each element, not sure we want that.
	for(const QVariant& element : std::as_const(list))
	{
		writeVariantToStream("item", element, xmlstream);
	}
}

void XmlSerializer::writeVariantMapToStream(const QVariant &variant, QXmlStreamWriter& xmlstream)
{
	Q_ASSERT(variant.isValid());
	Q_ASSERT(variant.canConvert<QVariantMap>());

	QVariantMap map = variant.toMap();

	// Stream out each element in the map.
	// In this case, the tag name of each element will have the name of the key.
	QMapIterator<QString, QVariant> i(map);
	while (i.hasNext())
	{
		i.next();
		writeVariantToStream(i.key(), i.value(), xmlstream);
	}
}

void XmlSerializer::writeVariantOrderedMapToStream(const QVariant& variant, QXmlStreamWriter& xmlstream)
{
	Q_ASSERT(variant.isValid());
	Q_ASSERT((variant.canConvert<InsertionOrderedMap<QString, QVariant>>()));

	InsertionOrderedMap<QString, QVariant> omap = variant.value<InsertionOrderedMap<QString, QVariant>>();

	xmlstream.writeAttribute("metatype_id", toqstr(std::to_string(omap.m_id)));
	xmlstream.writeAttribute("class", toqstr(omap.m_class));

	auto attrs = omap.get_attrs();
	for(const auto& it : attrs)
	{
		xmlstream.writeAttribute(toqstr(it.first), toqstr(it.second));
	}

	for(const auto& i : omap)
	{
		writeVariantToStream(i.first, i.second, xmlstream);
	}
}

void XmlSerializer::writeVariantValueToStream(const QVariant &variant, QXmlStreamWriter& xmlstream)
{
	Q_ASSERT(variant.isValid());

	// variant must be convertible to a string.
	if(!variant.canConvert<QString>())
	{
		std::string vartype {variant.typeName()};
        qCr() << "QVariant contents not convertible to a QString:" << M_ID_VAL(variant) << M_ID_VAL(vartype);

        Q_ASSERT(0);
	}

	QString str = variant.toString();

	// See if we need to re-encode it.

	xmlstream.writeCharacters(str);
}



/**
 * XmlSerializer::readVariantFromStream
 *
 * @note On invalid QVariant, skips the current element and tries to continue.
 *
 * @param xmlstream
 * @return
 */
QVariant XmlSerializer::readVariantFromStream(QXmlStreamReader& xmlstream)
{
	QXmlStreamAttributes attributes = xmlstream.attributes();
	QString typeString = attributes.value("type").toString();

	Q_ASSERT(xmlstream.isStartElement());

	QVariant variant;

	variant = InnerReadVariantFromStream(typeString, attributes, xmlstream);

	check_for_stream_error_and_skip(xmlstream);

	return variant;
}

QVariant XmlSerializer::InnerReadVariantFromStream(QString typeString, const QXmlStreamAttributes& attributes, QXmlStreamReader& xmlstream)
{
	QVariant variant;

	// Copy the attributes, removing only "type".
	std::vector<QXmlStreamAttribute> attributes_cp(attributes.cbegin(), attributes.cend());
	std::erase_if(attributes_cp, [](auto& attr){ return attr.qualifiedName() == "type" ? true : false; });

	QMetaType metatype = QMetaType::fromName(typeString.toStdString().c_str());

	if(metatype.id() == f_iomap_id)
	{
		variant = readVariantOrderedMapFromStream(attributes_cp, xmlstream);
	}
	else if(metatype.id() == f_qvarlist_id)
	{

		variant = readHomogenousListFromStream(xmlstream);
	}
	else if(metatype.id() == f_serqvarlist_id)
	{
#warning "Remove"
		log_current_node(xmlstream); qDb() << "serqvarlist";
		variant = readHomogenousListFromStream(xmlstream);
	}
	else
	{
		switch(metatype.id())
		{
			case QMetaType::QVariantList:
				variant = readVariantListFromStream(xmlstream);
				break;
			case QMetaType::QVariantMap:

				variant = readVariantMapFromStream(xmlstream);
				break;
			default:
				variant = readVariantValueFromStream(xmlstream);
				break;
		}
	}


	if(!variant.isValid())
	{
		// Whatever we read, it didn't make it into a QVariant successfully.
		// Report error and try to keep going.
		xmlstream.raiseError("#### Invalid QVariant conversion");
	}
	else if(!xmlstream.isEndElement())
	{
		// Not at an end element, parsing went wrong somehow.
		xmlstream.raiseError("#### NOT AT END ELEMENT, Reading xml stream failed, skipping to next start element");
	}

	return variant;
}


QVariant XmlSerializer::readHomogenousListFromStream(QXmlStreamReader& xmlstream)
{
	QVariantHomogenousList list;
	bool is_first_item = true;
	QString list_item_tag;

	Q_ASSERT(xmlstream.isStartElement());

	QString list_tag = xmlstream.name().toString();

//	qDb() << "List tag:" << list_tag;

	while(xmlstream.readNextStartElement())
	{
		if(is_first_item)
		{
			// We should have just read in the tag for items in this list.
			list_item_tag = xmlstream.name().toString();
			list.set_tag_names(list_tag, list_item_tag);
			is_first_item = false;
		}
		else
		{
			// Check that we're still reading the correct item type.
			auto this_item_tag = xmlstream.name();
			if(list_item_tag != this_item_tag)
			{
				Q_ASSERT_X(0, __func__, "TAG MISMATCH IN LIST");
			}
		}

		// Now read the contents of the <$list_item_tag>.
		QVariant next_list_element = readVariantFromStream(xmlstream);

		if(!next_list_element.isValid())
		{
			check_for_stream_error_and_skip(xmlstream);
		}
		else
		{
			list.push_back(next_list_element);
		}
	}

	return QVariant::fromValue(list);
}

QVariant XmlSerializer::readVariantListFromStream(QXmlStreamReader& xmlstream)
{
	QVariantList list;

	Q_ASSERT(xmlstream.isStartElement());

	while(xmlstream.readNextStartElement())
	{
		// We should have just read in an "<item>".
		Q_ASSERT(xmlstream.name() == "item");

		// Now read the contents of the <item>.
		QVariant next_list_element = readVariantFromStream(xmlstream);

		if(!next_list_element.isValid())
		{
			xmlstream.raiseError("#### Invalid QVariant conversion");
			check_for_stream_error_and_skip(xmlstream);
		}
		else
		{
			list.append(next_list_element);
		}
	}

	return list;
}

QString XmlSerializer::normalize_node_name(const QString& node_name) const
{
//	QRegularExpression re("s/(\\s+)/%20/g");

//	auto matchit = re.globalMatch(node_name);
	QString retval = node_name;
	return retval.replace(" ", "%20");
}

QVariant XmlSerializer::readVariantMapFromStream(QXmlStreamReader& xmlstream)
{
	QVariantMap map;

	Q_ASSERT(xmlstream.isStartElement());

	while(xmlstream.readNextStartElement())
	{
		map.insert(xmlstream.name().toString(), readVariantFromStream(xmlstream));
	}
	return map;
}

QVariant XmlSerializer::readVariantOrderedMapFromStream(std::vector<QXmlStreamAttribute> attributes, QXmlStreamReader& xmlstream)
{
	InsertionOrderedMap<QString, QVariant> map;

	Q_ASSERT(xmlstream.isStartElement());

	// Add the attributes to the QVariantInsertionOrderedMap.
	map.insert_attributes(attributes);

	while(xmlstream.readNextStartElement())
	{
		map.insert(xmlstream.name().toString(), readVariantFromStream(xmlstream));
	}

	return map;
}

QVariant XmlSerializer::readVariantValueFromStream(QXmlStreamReader& xmlstream)
{
	// The lowest-level read function.

	QXmlStreamAttributes attributes = xmlstream.attributes();
	QString attr_type_str = attributes.value("type").toString();

	Q_ASSERT(xmlstream.isStartElement());

	// Slurps up all contents of this element until the EndElement, including all child element text.
	QString element_text = xmlstream.readElementText();

	QVariant variant(element_text);

	if (!variant.isValid())
	{
		Q_ASSERT(0);
	}

	// Cast to type named in attr_type_str.
	// If this fails, status will be false, but variant will be changed to the requested type
	// will be null/cleared but valid.
	QMetaType metatype = QMetaType::fromName(attr_type_str.toStdString().c_str());
	const char* metatype_name = metatype.name();

	// qWr() << "READ FROM XML: Type:" << attr_type_str << ", Element text:" << element_text;
	bool element_text_is_empty = element_text.isEmpty();

	if (metatype.id() == QMetaType::UnknownType)
	{
		// This is bad, we don't know the type.
		xmlstream.raiseError(QString("ERROR: Unknown type: %1").arg(attr_type_str));
	}
	else
	{
		bool is_compatible = variant.canConvert(metatype);

		if (!is_compatible)
		{
			xmlstream.raiseError(QString("ERROR: CAN'T CONVERT FROM QSTRING TO INCOMPATIBLE TYPE: %1/%2/%3")
								.arg(attr_type_str)
								.arg(metatype.id()).arg(metatype_name));
		}
		else
		{
			// Check if it's an empty string.  If so, we're return a default-constructed object of type metatype.
            if (!variant.isValid() || variant.isNull())
			{
                qWr() << "TODO: NULL or INVALID QVARIANT, SKIPPING. Type:" << attr_type_str;
            	xmlstream.raiseError(QString("TODO: NULL or INVALID QVARIANT, SKIPPING. Type:") + attr_type_str);
			}
			else if (element_text_is_empty)
			{
				// Empty string, return default constructed object.
				QMetaType mtype = QMetaType::fromName(attr_type_str.toStdString().c_str());

				variant = QVariant::fromMetaType(mtype);
			}
			else
			{
				// Everything looks OK, we should be able to convert.
				bool status = variant.convert(metatype);

				if (!status)
				{
					qWr() << "ERROR: METATYPES:" << M_ID_VAL(attr_type_str) << M_ID_VAL(metatype) << "( name: " <<
						metatype_name << ")";
					xmlstream.raiseError(
						QString("XML FAIL: Could not convert string '%1' to object of type '%2'").arg(
							element_text, attr_type_str));
				}
			}
		}
	}

	// No check for errors here, let the top-level readVariantFromStream() handle that.
	//	check_for_stream_error_and_skip(xmlstream);

	return variant;
}

void XmlSerializer::check_for_stream_error_and_skip(QXmlStreamReader& xmlstream)
{
	if(xmlstream.hasError())
	{
		auto estr = error_string(xmlstream);

		qWr() << "### XML STREAM READ ERROR:" << estr << ", skipping current element";
		xmlstream.skipCurrentElement();
	}
}

void XmlSerializer::log_current_node(QXmlStreamReader& xmlstream)
{
	qIn() << "#### Current node:" << xmlstream.lineNumber() << ":" << xmlstream.columnNumber() << ":" << xmlstream.qualifiedName();
}

void XmlSerializer::set_default_namespace(const std::string& default_ns, const std::string& default_ns_version)
{
	m_default_ns = default_ns;
	m_default_ns_version = default_ns_version;
}

QString XmlSerializer::error_string(QXmlStreamReader& xmlstream) const
{
	return QObject::tr("%1: Line %2, column %3")
			.arg(xmlstream.errorString())
			.arg(xmlstream.lineNumber())
			.arg(xmlstream.columnNumber());
}

QString XmlSerializer::error_string(QXmlStreamWriter& xmlstream) const
{
	return QObject::tr("%1: Line %2, column %3").arg("Unknown error on write").arg("0", "0");
}


void XmlSerializer::save_extra_start_info(QXmlStreamWriter& xmlstream)
{
	/// @todo This still seems like it should be one layer above this class.

	// Write DTD
	// N/A

	// Write Start Element, default namespace and version.
	xmlstream.writeStartElement("amlm_database");
	xmlstream.writeDefaultNamespace(toqstr(m_default_ns));
	xmlstream.writeAttribute("version", toqstr(m_default_ns_version));
	xmlstream.writeNamespace("http://amlm/ns/0/", "amlm");

#if 0 /// @note This was moved out of the old writer.  It's XSPF stuff, but we need a generally applicable solution.
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

		/// @todo Probably get from derived model class?
//	xml.writeStartElement(m_tree_model->getXmlStreamName());
//	xml.writeAttribute(AbstractTreeModelReader::versionAttribute(), m_tree_model->getXmlStreamVersion());
#endif
}

void XmlSerializer::load_extra_start_info(QXmlStreamReader* xmlstream)
{
	// Write Start Element, default namespace and version.
//	xmlstream.writeStartElement("amlm_database");
//	xmlstream.writeDefaultNamespace(m_default_ns);
	/// @temp Read "playlist"
	xmlstream->readNextStartElement();
	AMLM_ASSERT_EQ(xmlstream->name(), "playlist");
//	xmlstream.writeAttribute("version", m_default_ns_version);
//	xmlstream.writeNamespace("http://amlm/ns/0/", "amlm");
}



