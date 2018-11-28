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
 * @file XmlSerializer.h
 */
#ifndef SRC_LOGIC_XMLSERIALIZER_H_
#define SRC_LOGIC_XMLSERIALIZER_H_

// Qt5
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QVariant>
class QString;
class QStringList;

// Ours
#include "ISerializer.h"

/*
 *
 */
class XmlSerializer: public ISerializer
{
public:
	XmlSerializer() = default;
	~XmlSerializer() override = default;

	/**
	 *
	 * @param serializable
	 * @param file_url
	 * @param root_element_name  String to use for the single root element of the document.
	 * @param
	 */
	void save(const ISerializable& serializable,
			const QUrl& file_url,
			const QString& root_element_name,
			std::function<void(void)> extra_save_actions = nullptr
			) override;

	void load(ISerializable& serializable, const QUrl& file_url) override;

	void set_default_namespace(const QString& default_ns, const QString& default_ns_version);

protected:

	void save_extra_start_info(QXmlStreamWriter& xmlstream);

private:

	/// @name Write members
	/// @{

	void writeVariantToStream(const QString& nodeName,
	                          const QVariant& variant, QXmlStreamWriter& xmlstream);

	void writeVariantListToStream(const QVariant &variant, QXmlStreamWriter& xmlstream);

	void writeVariantMapToStream(const QVariant& variant, QXmlStreamWriter& xmlstream);

	void writeVariantValueToStream(const QVariant& variant, QXmlStreamWriter& xmlstream);

	/// @}

	/// @name Read members
	/// @{

	QVariant readVariantFromStream(QXmlStreamReader& xmlstream);
	QVariant readVariantValueFromStream(QXmlStreamReader& xmlstream);
	QVariant readVariantListFromStream(QXmlStreamReader& xmlstream);
	QVariant readVariantMapFromStream(QXmlStreamReader& xmlstream);

	/// @}

	void check_for_stream_error_and_skip(QXmlStreamReader& xmlstream);


	QString errorString(QXmlStreamReader& xmlstream) const
	{
		return QObject::tr("%1\nLine %2, column %3")
				.arg(xmlstream.errorString())
				.arg(xmlstream.lineNumber())
				.arg(xmlstream.columnNumber());
	}

	QString m_root_name;
	QString m_default_ns;
	QString m_default_ns_version;

};

#endif /* SRC_LOGIC_XMLSERIALIZER_H_ */
