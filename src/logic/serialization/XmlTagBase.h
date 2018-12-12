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
 * @file XMLTagBase.h
 */
#ifndef SRC_LOGIC_SERIALIZATION_XMLTAGBASE_H_
#define SRC_LOGIC_SERIALIZATION_XMLTAGBASE_H_

// Ours
#include "ExtEnum.h"
#include <utils/crtp.h>

template <class ScopeTypeEnumType>
using ExtEnumToStringMap = ExtEnumMapBase<ScopeTypeEnumType, QString>;

/**
 * CRTP Base class for XML tags.
 */
template <class DerivedType>
class XmlTagBase : public ExtEnum<DerivedType>, crtp<DerivedType, XmlTagBase>
{
public:
	XmlTagBase() {};
	XmlTagBase(const std::string& tag_string) : m_tag_string(tag_string) {};
//	XmlTagBase(std::initializer_list<>)
	virtual ~XmlTagBase() {};

//	const std::string toXmlTagString(int enumerator) const { return this->m_tag_string; };

	/// The tag in string form.  Does not contain the "<>"'s or the end slash.
	std::string m_tag_string { "ERROR" };

	struct MapStruct { int the_tag_index; const char * str; };
//#define X(s, tag_str) { s, tag_str },
//	static constexpr MapStruct m_the_xml_tagmap[] = { M_ExtUrlTags(X) };
////	static constexpr decltype(auto)  map2 = std::experimental::make_array({M_ExtUrlTags(X)});
//#undef X
};

/**
 * X-Macro support for mappings.
 * @rant Seriously.  It's the 21st century and we still have to resort to this.
 */

#define M_ID_COMMA(ident) ident,
#define M_ID_COMMA_STR_COMMA(ident) ident, #ident

#endif /* SRC_LOGIC_SERIALIZATION_XMLTAGBASE_H_ */
