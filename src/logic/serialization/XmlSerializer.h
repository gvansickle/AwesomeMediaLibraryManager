/*
 * Copyright 2018, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef SRC_LOGIC_SERIALIZATION_XMLSERIALIZER_H_
#define SRC_LOGIC_SERIALIZATION_XMLSERIALIZER_H_

// Std C++
#include <functional>
#include <variant>
#include <vector>

// Qt5
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QVariant>
class QString;
//class QStringList;
class QVariantHomogenousList;

// Ours
#include "ISerializer.h"


/**
 * Concrete ISerializer class for serializing ISerializables as XML.
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

	bool load(ISerializable& serializable, const QUrl& file_url) override;

	void HACK_skip_extra(bool hack_skip) { m_HACK_SKIP = hack_skip; };

	/**
	 * Call this before save() to set the default XML namespace.
	 * @param default_ns
	 * @param default_ns_version
	 */
	void set_default_namespace(const std::string& default_ns, const std::string& default_ns_version);

protected:

	void save_extra_start_info(QXmlStreamWriter& xmlstream);
	void load_extra_start_info(QXmlStreamReader* xmlstream);

private:

	/// @name Write members
	/// @{

	void writeVariantToStream(const QString& nodeName,
	                          const QVariant& variant, QXmlStreamWriter& xmlstream);

	void InnerWriteVariantToStream(const QVariant& variant, QXmlStreamWriter* xmlstream);
	void writeQVariantHomogenousListToStream(const QVariant& variant, QXmlStreamWriter& xmlstream);
	void writeVariantListToStream(const QVariant &variant, QXmlStreamWriter& xmlstream);
	void writeVariantMapToStream(const QVariant& variant, QXmlStreamWriter& xmlstream);
	void writeVariantOrderedMapToStream(const QVariant& variant, QXmlStreamWriter& xmlstream);
	void writeVariantValueToStream(const QVariant& variant, QXmlStreamWriter& xmlstream);

	/// @}

	/// @name Read members
	/// @{

	QVariant readVariantFromStream(QXmlStreamReader& xmlstream);

	QVariant InnerReadVariantFromStream(QString typeString, const QXmlStreamAttributes& attributes, QXmlStreamReader& xmlstream);
	QVariant readHomogenousListFromStream(QXmlStreamReader& xmlstream);
	QVariant readVariantListFromStream(QXmlStreamReader& xmlstream);
	QVariant readVariantMapFromStream(QXmlStreamReader& xmlstream);
	QVariant readVariantOrderedMapFromStream(std::vector<QXmlStreamAttribute> attributes, QXmlStreamReader& xmlstream);
	QVariant readVariantValueFromStream(QXmlStreamReader& xmlstream);

	/// @}

	/// @name Read/Write helpers
	/// @{

	QString normalize_node_name(const QString& node_name) const;

	/// @}

	void check_for_stream_error_and_skip(QXmlStreamReader& xmlstream);

	void log_current_node(QXmlStreamReader& xmlstream);

//	using QXmlStreamRWRef = std::variant<std::reference_wrapper<QXmlStreamReader>, std::reference_wrapper<QXmlStreamWriter>>;
	using QXmlStreamRWRef = std::variant<QXmlStreamReader*, QXmlStreamWriter*>;
	/**
	 * If the given QXmlStreamReader/Writer has an error, returns the error string.
	 * @return
	 */
	QString error_string(QXmlStreamReader& xmlstream) const;
	QString error_string(QXmlStreamWriter& xmlstream) const;

	QString m_root_name;

	std::string m_default_ns;
	std::string m_default_ns_version;

	bool m_HACK_SKIP {true};
};

#endif /* SRC_LOGIC_SERIALIZATION_XMLSERIALIZER_H_ */
