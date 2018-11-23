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
 * @file XmlSerializer.cpp
 */

#include "XmlSerializer.h"

// Qt5
#include <QFile>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QDataStream>

// Ours
#include <utils/DebugHelpers.h>


XmlSerializer::XmlSerializer() : ISerializer()
{
}

XmlSerializer::~XmlSerializer()
{

}

void XmlSerializer::save(const ISerializable &serializable, const QUrl &file_url, const QString &root_name)
{
	/// @todo file_url Currently only file://'s are supported.

	QString save_file_path = file_url.toLocalFile();
	if(save_file_path.isEmpty())
	{
		Q_ASSERT_X(0, __PRETTY_FUNCTION__, "LOCAL FILE PATH IS EMPTY");
	}

	QFile file(save_file_path);

	file.open(QFile::WriteOnly);

	QXmlStreamWriter xmlstream(&file);

	xmlstream.setAutoFormatting(true);

	xmlstream.writeStartDocument();
	writeVariantToStream(root_name, serializable.toVariant(), xmlstream);
	xmlstream.writeEndDocument();

	file.close();
}

void XmlSerializer::load(ISerializable& serializable, const QUrl &file_url)
{
	QString load_file_path = file_url.toLocalFile();
	if(load_file_path.isEmpty())
	{
		Q_ASSERT_X(0, __PRETTY_FUNCTION__, "LOCAL FILE PATH IS EMPTY");
	}

	QFile file(load_file_path);
	file.open(QFile::ReadOnly);
	QXmlStreamReader stream(&file);

	// Read the first element in the file.
	if(!stream.readNextStartElement())
	{
		// Something went wrong.
		stream.raiseError("Reading first start element failed.");

		/// @todo Move
		qWr() << errorString(stream);
	}
	else
	{
		// Stream it all in.
		serializable.fromVariant(readVariantFromStream(stream));
	}
}

void XmlSerializer::writeVariantToStream(const QString &nodeName, const QVariant& variant, QXmlStreamWriter& xmlstream)
{
	xmlstream.writeStartElement(nodeName);
	xmlstream.writeAttribute("type", variant.typeName());

	/**
	 * @note Uhhhhhh..... QMetaType sometimes != QVariant.type().
	 *
	 * This looks like a complete fiasco.  Seriously, from the Qt 5.11.1 docs:
	 *
	 * @link http://doc.qt.io/qt-5/qvariant.html#type
	 * "QVariant::Type QVariant::type() const
	 *    Although this function is declared as returning QVariant::Type, the return value should be
	 * interpreted as QMetaType::Type. [...]
	 * Note that return values in the ranges QVariant::Char through QVariant::RegExp and QVariant::Font
	 * through QVariant::Transform correspond to the values in the ranges QMetaType::QChar through QMetaType::QRegExp
	 * and QMetaType::QFont through QMetaType::QQuaternion. [...huh?] Pay particular attention when working
	 * with char and QChar variants. [...whu...?] Also note that the types void*, long, short, unsigned long,
	 * unsigned short, unsigned char, float, QObject*, and QWidget* are represented in QMetaType::Type but not
	 * in QVariant::Type, and they can be returned by this function. [...???] However, they are considered to
	 * be user defined types when tested against QVariant::Type. [$*&^$%*#@@#!???]".
	 *
	 * ...oh, ok, a partial explanation:
	 * From qvariant.h:495:
	 * "// QVariant::Type is marked as \obsolete, but we don't want to
    // provide a constructor from its intended replacement,
    // QMetaType::Type, instead, because the idea behind these
    // constructors is flawed in the first place. But we also don't
    // want QVariant(QMetaType::String) to compile and falsely be an
    // int variant, so delete this constructor:
    QVariant(QMetaType::Type) Q_DECL_EQ_DELETE;"
	 */

	int type = variant.type(); // AFAICT this is just wrong.
	int usertype = variant.userType(); // This matches variant.typeName()

	if(type != usertype)
	{
//		qWr() << "#### TYPE != USERTYPE: variant.typeName():" << variant.typeName() << "As ints:" << type << "!=" << usertype << ":"
//				<< QVariant::typeToName(type) << QVariant::typeToName(usertype);
	}
	switch (usertype)//variant.type())
	{
		case QMetaType::QVariantList:
			writeVariantListToStream(variant, xmlstream);
			break;
		case QMetaType::QVariantMap:
			writeVariantMapToStream(variant, xmlstream);
			break;
		default:
			writeVariantValueToStream(variant, xmlstream);
			break;
	}
	xmlstream.writeEndElement();
}


void XmlSerializer::writeVariantValueToStream(const QVariant &variant, QXmlStreamWriter& xmlstream)
{
	Q_ASSERT(variant.isValid());

	QString str = variant.toString();

	xmlstream.writeCharacters(str);
}


void XmlSerializer::writeVariantListToStream(const QVariant &variant, QXmlStreamWriter& xmlstream)
{
	QVariantList list = variant.toList();

	// Stream each QVariant in the list out.
	/// @note tag name will be "item" for each element, not sure we want that.
	for(const QVariant& element : list)
	{
		writeVariantToStream("item", element, xmlstream);
	}
}

void XmlSerializer::writeVariantMapToStream(const QVariant &variant, QXmlStreamWriter& xmlstream)
{
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


QVariant XmlSerializer::readVariantFromStream(QXmlStreamReader& xmlstream)
{
	QXmlStreamAttributes attributes = xmlstream.attributes();
	QString typeString = attributes.value("type").toString();

	QVariant variant;
	auto metatype = QVariant::nameToType(typeString.toStdString().c_str());
	switch (metatype)
	{
		case QMetaType::QVariantList:
			variant = readVariantListFromStream(xmlstream);
			break;
		case QMetaType::QVariantMap:
			variant = readVariantMapFromStream(xmlstream);
			break;
//		case QMetaType::QUrl:
//		{
//			qInfo() << "#### Trying to read type:" << metatype;
//			QByteArray ba;
//			QDataStream ds(&ba, QIODevice::ReadWrite);
//			ds << xmlstream.readElementText();
//
//			variant.fromValue(ds) = ds;
//			qIn() << "### ds:" << ds;
//			break;
//		}
		default:
//			qInfo() << "#### Trying to read type:" << metatype;
			variant = readVariantValueFromStream(xmlstream);
			break;
	}

	if(!variant.isValid())
	{
		// Whatever we read, it didn't make it to a QVariant successfully.
		xmlstream.raiseError("Invalid QVariant conversion.");
		qWr() << errorString(xmlstream);

		// Try to keep going.
		/// @todo Not sure what we need to do here.
	}

	if(!xmlstream.isEndElement())
	{
		// Not at an end element, parsing went wrong somehow.
		xmlstream.raiseError("Reading xml stream failed, skipping to next start element.");
		qWr() << errorString(xmlstream);

		// Try to keep going, skip to the next sibling element.
		xmlstream.skipCurrentElement();
	}

	return variant;
}

QVariant XmlSerializer::readVariantValueFromStream(QXmlStreamReader& xmlstream)
{
	QXmlStreamAttributes attributes = xmlstream.attributes();
	QString typeString = attributes.value("type").toString();

	// Slurps up all contents of this element until the EndElement, including all child element text.
	/// @note I know, not cool with all the RAM wasteage.
	QString dataString = xmlstream.readElementText();

//	qIn() << "Type:" << typeString << ", Data:" << dataString;

	QVariant variant(dataString);

	if(!variant.isValid())
	{
		Q_ASSERT(0);
	}

	// Cast to type named in typeString.
	// If this fails, status will be false, but variant will be changed to the requested type
	// will be null/cleared byt valid.
	bool status = variant.convert(QVariant::nameToType(typeString.toStdString().c_str()));

	if(!status)
	{
		qWr() << QString("XML FAIL: Could not convert string '%1' to object of type '%2'").arg(dataString, typeString);
		qWr() << "isValid():" << variant.isValid();
	}

	return variant;
}

QVariant XmlSerializer::readVariantListFromStream(QXmlStreamReader& xmlstream)
{
	QVariantList list;
	while(xmlstream.readNextStartElement())
	{
		// We should have just read in an "<item>".
		Q_ASSERT(xmlstream.text() == );
		QVariant next_list_element = readVariantFromStream(xmlstream);

		check_for_stream_error_and_skip(xmlstream);

		list.append(next_list_element);
	}

	check_for_stream_error_and_skip(xmlstream);

	return list;
}

QVariant XmlSerializer::readVariantMapFromStream(QXmlStreamReader& xmlstream)
{
	QVariantMap map;
	while(xmlstream.readNextStartElement())
	{
		map.insert(xmlstream.name().toString(), readVariantFromStream(xmlstream));
	}
	return map;
}

void XmlSerializer::check_for_stream_error_and_skip(QXmlStreamReader& xmlstream)
{

}
