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



//void XmlElement::append(std::unique_ptr<XmlElement> child)
//{
//	m_child_elements.push_back(std::move(child));
//}

void XmlElement::write(QXmlStreamWriter* out) const
{
	// Don't write twice.
	Q_ASSERT(m_i_have_been_written == false);

	m_out_ptr = out;

	auto& m_out = *m_out_ptr;

	// Tag name
	m_out.writeStartElement(m_tagname);

	// Attributes
	if(!m_id.name().isEmpty())
	{
		m_out.writeAttribute(m_id);
	}
	if(!m_attributes.empty())
	{
		m_out.writeAttributes(m_attributes);
	}
	// The single value.
	if(!m_value.isEmpty() && !m_value.isNull())
	{
		m_out.writeCharacters(m_value);
	}

	// Do whatever's in the inner scope lambda.
	if(m_inner_scope)
	{
		m_inner_scope(const_cast<XmlElement*>(this), &m_out);
	}

	// Write out any child elements.
//	for(const auto& e : m_child_elements)
//	{
//		e->write(out);
//	}

	// End element.
	m_out.writeEndElement();

	m_i_have_been_written = true;
}
