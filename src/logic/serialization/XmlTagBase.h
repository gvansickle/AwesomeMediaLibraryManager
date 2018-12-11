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

#if 0
template <class DerivedType>
struct XmlTag : public ExtEnum<XmlTag>, crtp<DerivedType, XmlTag>
{
	XmlTag(std::string tag_string) : m_tag_string(tag_string) {};

private:

	/// The tag in string form.  Does not contain the "<>"'s or the end slash.
	std::string m_tag_string;
};
#endif

/**
 * CRTP Base class for XML tags.
 */
template <class DerivedType>
class XmlTagBase : public ExtEnum<DerivedType>//, public XmlTag<DerivedType>
{
public:
	XmlTagBase() {};
	XmlTagBase(const std::string& tag_string) : m_tag_string(tag_string) {};
	virtual ~XmlTagBase() {};

	/// The tag in string form.  Does not contain the "<>"'s or the end slash.
	std::string m_tag_string;
};

#endif /* SRC_LOGIC_SERIALIZATION_XMLTAGBASE_H_ */
