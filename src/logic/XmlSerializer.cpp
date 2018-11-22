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

// Ours
#include <utils/DebugHelpers.h>


XmlSerializer::XmlSerializer()
{
	// TODO Auto-generated constructor stub

}

XmlSerializer::~XmlSerializer()
{
	// TODO Auto-generated destructor stub
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

void XmlSerializer::load(ISerializable &serializable, const QUrl &file_url)
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
	stream.readNextStartElement();

	// Stream it all in.
	serializable.fromVariant(readVariantFromStream(stream));
}

void XmlSerializer::writeVariantToStream(const QString &nodeName, const QVariant& variant, QXmlStreamWriter& xmlstream)
{
	xmlstream.writeStartElement(nodeName);
	xmlstream.writeAttribute("type", variant.typeName());
	int type = variant.type();
	int usertype = variant.userType();
	if(type != usertype)
	{
		qWr() << "#### TYPE != USER TYPE:" << type << usertype;
	}
	switch (variant.userType())//variant.type())
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


void XmlSerializer::writeVariantValueToStream(const QVariant &variant, QXmlStreamWriter& xmlstream)
{
	xmlstream.writeCharacters(variant.toString());
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
		default:
			qInfo() << "#### type:" << metatype;
			variant = readVariantValueFromStream(xmlstream);
			break;
	}
	return variant;
}

QVariant XmlSerializer::readVariantValueFromStream(QXmlStreamReader& xmlstream)
{
	QXmlStreamAttributes attributes = xmlstream.attributes();
	QString typeString = attributes.value("type").toString();
	QString dataString = xmlstream.readElementText();
	QVariant variant(dataString);
	variant.convert(QVariant::nameToType(
			typeString.toStdString().c_str()));
	return variant;
}

QVariant XmlSerializer::readVariantListFromStream(QXmlStreamReader& xmlstream)
{
	QVariantList list;
	while(xmlstream.readNextStartElement())
	{
		list.append(readVariantFromStream(xmlstream));
	}
	return list;
}

QVariant XmlSerializer::readVariantMapFromStream(QXmlStreamReader& xmlstream)
{
	QVariantMap map;
	while(xmlstream.readNextStartElement())
	{
		map.insert(xmlstream.name().toString(),
		           readVariantFromStream(xmlstream));
	}
	return map;
}
