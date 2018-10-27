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

// Qt5
#include <QXmlStreamWriter>


class XmlAttribute : public QXmlStreamAttribute
{
//public:
//	XmlAttribute(std::initializer_list<QString> initlist) : QXmlStreamAttribute()
//	{

//	}
};

class XmlAttributeList : public QXmlStreamAttributes
{
public:
	XmlAttributeList() : QXmlStreamAttributes()
	{ };

	XmlAttributeList(std::initializer_list<QXmlStreamAttribute> initlist) : QXmlStreamAttributes()
	{
		this->append(initlist);
	}

//	XmlAttributeList(std::initializer_list<XmlAttribute> initlist) : QXmlStreamAttributes()
//	{
//		for(auto e : initlist)
//		{
//			append(e);
//		}
////		initlist.begin(), initlist.end());
////		std::copy(initlist.begin(), initlist.end(), this-> ->append(initlist));
//	}
};

/**
 * RAII/convenience class for XML elements.
 */
class XmlElement
{
public:

	/// Type of the callback which will be called after the State element and any attributes are written
	/// but before the EndElement.
	using InnerScopeType = std::function<void(QXmlStreamWriter*)>;

//	/// No attributes.
//	explicit XmlElement(QXmlStreamWriter& out, QString tagname, InnerScopeType inner_scope = InnerScopeType(nullptr))
//		: m_out(out), m_tagname(tagname), m_inner_scope(inner_scope)
//	{ };

//	/// With attributes.
//	explicit XmlElement(QXmlStreamWriter& out, QString tagname, XmlAttributeList attrs,
//						InnerScopeType inner_scope = InnerScopeType(nullptr))
//		: m_out(out), m_tagname(tagname), m_attributes(attrs), m_inner_scope(inner_scope)
//	{ };

	/// No attributes.
	explicit XmlElement(QString tagname, InnerScopeType inner_scope = InnerScopeType(nullptr))
		: m_tagname(tagname), m_inner_scope(inner_scope)
	{ };

	/// With attributes.
	explicit XmlElement(QString tagname, XmlAttributeList attrs,
						InnerScopeType inner_scope = InnerScopeType(nullptr))
		: m_tagname(tagname), m_attributes(attrs), m_inner_scope(inner_scope)
	{ };

	/**
	 * Specific function for setting an "id" attribute from outside this element's creator.
	 */
	void setId(const QString& idstr)
	{
		m_attributes.append("id", idstr);
	}

	void set_out(QXmlStreamWriter* out)
	{
		m_out_ptr = out;
	}

	void write(QXmlStreamWriter* out)
	{
		m_out_ptr = out;

		auto& m_out = *m_out_ptr;

		// Tag name
		m_out.writeStartElement(m_tagname);

		// Attributes
		if(!m_attributes.empty())
		{
			m_out.writeAttributes(m_attributes);
		}

		// Do whatever's in the inner scope lambda.
		if(m_inner_scope)
		{
			m_inner_scope(&m_out);
		}

		// End element.
		m_out.writeEndElement();
	}

	XmlElement(const XmlElement& other)
	{
		*this = other;
		other.m_i_have_been_copied_from = true;
	}

	virtual ~XmlElement()
	{
		if(m_i_have_been_copied_from)
		{
			// Skip the write.
		}
		else
		{
			Q_ASSERT(m_out_ptr != nullptr);

			write(m_out_ptr);
		}
	};

protected:
	mutable bool m_i_have_been_copied_from = false;
	QXmlStreamWriter* m_out_ptr = nullptr;
	QString m_tagname;
	XmlAttributeList m_attributes;
	InnerScopeType m_inner_scope;
};

//QXmlStreamWriter& operator<<(QXmlStreamWriter& out, const XmlElement& e)
//{

//	return out;
//}

#endif /* SRC_LOGIC_XML_XMLOBJECTS_H_ */
