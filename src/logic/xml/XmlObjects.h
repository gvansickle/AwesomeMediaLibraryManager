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


class XmlValue : public QString
{
public:
	XmlValue() = default;
	XmlValue(const QString& qstr) : QString(qstr) {};
	XmlValue(const QDateTime& dt) : QString(dt.toString(Qt::ISODate)) {};
	XmlValue(const QUrl& qurl) : QString(qurl.toString()) {};
	XmlValue(long long value) : QString(QString("%1").arg(value)) {};

//	template <class UnhandledType>
//	XmlValue(UnhandledType t)
//	{
//		static_assert(0, "XmlValue doesn't support this type.");
//	}

	~XmlValue() = default;


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

	XmlAttribute(const QString &qualifiedName, XmlValue value) : QXmlStreamAttribute(qualifiedName, value)
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

class XmlElement;

class XmlElementList : public QVector<XmlElement>
{
public:
	XmlElementList() = default;
	~XmlElementList() = default;

	/**
	 * Initializer-list constructor.
	 * Force this to be used with the "{}" initializer notation.
	 */
	XmlElementList(std::initializer_list<XmlElement> initlist) : QVector<XmlElement>(initlist)
	{
	}

	void write(QXmlStreamWriter* out) const;


//	mutable QXmlStreamWriter* m_out_ptr = nullptr;
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

	/// No attributes, no value.
	explicit XmlElement(const QString& tagname, InnerScopeType inner_scope)
		: m_tagname(tagname), m_inner_scope(inner_scope)
	{ };

	/// With attributes, no value.
	explicit XmlElement(const QString& tagname, XmlAttributeList attrs,
						InnerScopeType inner_scope)
		: m_tagname(tagname), m_attributes(attrs), m_inner_scope(inner_scope)
	{ };

	/// With attributes, value, and children.
	explicit XmlElement(const QString& tagname, XmlAttributeList attrs,
						XmlValue value, XmlElementList children,
						InnerScopeType inner_scope = InnerScopeType(nullptr))
		: m_tagname(tagname), m_attributes(attrs), m_value(value), m_child_elements(children), m_inner_scope(inner_scope)
	{ };

	/// With attributes and value.
	explicit XmlElement(const QString& tagname, XmlAttributeList attrs,
						XmlValue value,
						InnerScopeType inner_scope)
		: m_tagname(tagname), m_attributes(attrs), m_value(value), m_inner_scope(inner_scope)
	{ };

	/// No attributes, but value.
	explicit XmlElement(const QString& tagname, XmlValue value, InnerScopeType inner_scope)
		: XmlElement(tagname, XmlAttributeList(), value, inner_scope)
	{ };

	/// No attributes, no scope, but value.
	explicit XmlElement(const QString& tagname, XmlValue value)
		: XmlElement(tagname, XmlAttributeList(), value, InnerScopeType(nullptr))
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
	XmlElement setId(const QString& idstr)
	{
		m_id = XmlAttribute("id", idstr);
		return *this;
	}

	/// Overload for taking a uint64_t vs. a string.
	XmlElement setId(std::uint64_t idnum)
	{
		QString idstr = QString("%1").arg(idnum);
		return setId(idstr);
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
			/// @todo Experimental, not writing on destruction.
//			Q_ASSERT(m_out_ptr != nullptr);

//			write(m_out_ptr);
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
	XmlValue m_value;

	/// List of child elements.  Mutable because it's likely to be
	/// added to in the InnerScopeType callback as part of write().
	mutable XmlElementList m_child_elements;

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
