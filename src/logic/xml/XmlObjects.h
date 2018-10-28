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
 * @file XmlObjects.h
 */
#ifndef SRC_LOGIC_XML_XMLOBJECTS_H_
#define SRC_LOGIC_XML_XMLOBJECTS_H_

// Std C++
#include <functional>
#include <memory>
#include <initializer_list>
#include <cstdint>

// Qt5
#include <QXmlStreamWriter>
#include <QDateTime>
#include <QUrl>


class XmlAttrValue : public QString
{
public:
	XmlAttrValue() = default;
	XmlAttrValue(const QDateTime& dt) : QString(dt.toString(Qt::ISODate)) {};
	XmlAttrValue(const QUrl& qurl) : QString(qurl.toString()) {};
	XmlAttrValue(long long value) : QString(QString("%1").arg(value)) {};
};

class XmlAttribute : public QXmlStreamAttribute
{
public:
	XmlAttribute() = default;
//	XmlAttribute(const QXmlStreamAttribute& other);
	~XmlAttribute() = default;

	XmlAttribute(const QString &qualifiedName, const QString &value) : QXmlStreamAttribute(qualifiedName, value)
	{
	}

	XmlAttribute(const QString &qualifiedName, long long value) : QXmlStreamAttribute(qualifiedName, QString("%1").arg(value))
	{
	}

	XmlAttribute(const QString &qualifiedName, XmlAttrValue value) : QXmlStreamAttribute(qualifiedName, value)
	{
	}

	/**
	 * Honeypot for any attribute whose value is not convertible to a QString.
	 */
//	template<class T>
//	XmlAttribute(const QString &qualifiedName, const T& value)
//	{
//		static_assert (0, "value must be convertible to a QString");
//	}
};

class XmlAttributeList : public QXmlStreamAttributes
{
public:
	XmlAttributeList() = default;
	~XmlAttributeList() = default;

//	XmlAttributeList(std::initializer_list<QXmlStreamAttribute> initlist)
//	{
//		this->append(initlist);
//	}

	/**
	 * Initializer-list constructor.
	 * Force this to be used with the "{}" initializer notation.
	 */
	XmlAttributeList(std::initializer_list<XmlAttribute> initlist)
	{
		for(const auto& e : initlist)
		{
			append(e);
		}
	}
};

/**
 * RAII/convenience class for XML elements.
 */
class XmlElement
{
public:

	/// Type of the callback which will be called after the State element and any attributes are written
	/// but before the EndElement.
	using InnerScopeType = std::function<void(XmlElement*, QXmlStreamWriter*)>;

	/// No attributes.
	explicit XmlElement(const QString& tagname, InnerScopeType inner_scope = InnerScopeType(nullptr))
		: m_tagname(tagname), m_inner_scope(inner_scope)
	{ };

	/// With attributes.
	explicit XmlElement(const QString& tagname, XmlAttributeList attrs,
						InnerScopeType inner_scope = InnerScopeType(nullptr))
		: m_tagname(tagname), m_attributes(attrs), m_inner_scope(inner_scope)
	{ };

	/// With attributes.
	explicit XmlElement(const QString& tagname, XmlAttributeList attrs,
						XmlAttrValue value,
						InnerScopeType inner_scope = InnerScopeType(nullptr))
		: m_tagname(tagname), m_attributes(attrs), m_value(value), m_inner_scope(inner_scope)
	{ };

	/// No attributes.
	explicit XmlElement(const QString& tagname, XmlAttrValue value, InnerScopeType inner_scope = InnerScopeType(nullptr))
		: XmlElement(tagname, {}, value, inner_scope)
	{ };

	/**
	 * Copy constructor.
	 */
	XmlElement(const XmlElement& other)
	{
		*this = other;
		other.m_i_have_been_copied_from = true;
	}

	/**
	 * Specific function for setting the "id" attribute from outside this element's creator.
	 */
	void setId(const QString& idstr)
	{
		m_id = XmlAttribute("id", idstr);
	}

	/// Overload for taking a uint64_t vs. a string.
	void setId(std::uint64_t idnum)
	{
		QString idstr = QString("%1").arg(idnum);
		setId(idstr);
	}

	// Add a child element to this element.
//	void append(std::unique_ptr<XmlElement> child);


	void set_out(QXmlStreamWriter* out)
	{
		m_out_ptr = out;
	}

	void write(QXmlStreamWriter* out) const;

	virtual ~XmlElement()
	{
		if(m_i_have_been_copied_from || m_i_have_been_written)
		{
			// Skip the write-on-destruction.
		}
		else
		{
			Q_ASSERT(m_out_ptr != nullptr);

			write(m_out_ptr);
		}
	};

protected:
	mutable bool m_i_have_been_copied_from = false;
	mutable bool m_i_have_been_written = false;

	mutable QXmlStreamWriter* m_out_ptr = nullptr;

	/// The tagname of this element.
	QString m_tagname;

	/// The id attribute, if any.
	XmlAttribute m_id;

	/// List of all other attributes, if any.
	XmlAttributeList m_attributes;

	/// The element's singular value, if any.
	XmlAttrValue m_value;

	/// List of child elements.  Mutable because it's likely to be
	/// added to in the InnerScopeType callback as part of write().
//	mutable std::vector<std::unique_ptr<XmlElement>> m_child_elements;

	/// The inner scopr lambda, containing:
	/// - Additional attribute adds for this element.
	/// - Child XmlElements.
	/// Is passed a pointer to m_out_ptr.
	InnerScopeType m_inner_scope;
};

static inline QXmlStreamWriter& operator<<(QXmlStreamWriter& out, const XmlElement& e)
{
	e.write(&out);
	return out;
}

#endif /* SRC_LOGIC_XML_XMLOBJECTS_H_ */
