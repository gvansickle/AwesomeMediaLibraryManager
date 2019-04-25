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

#ifndef AWESOMEMEDIALIBRARYMANAGER_SCANRESULTSTREEMODELXMLTAGS_H
#define AWESOMEMEDIALIBRARYMANAGER_SCANRESULTSTREEMODELXMLTAGS_H

// Std C++
#include <initializer_list>
#include <map>
#include <string>
//#include <experimental/array>

// Qt5
#include <QObject>
#include <QtXml>

// Ours
#include <src/logic/serialization/ExtEnum.h>
#include <src/logic/serialization/XmlTagBase.h>

/**
 * ExtUrl tags.
 * These are the XML tags which you'll find in an ExtUrl element.
 */
class ExtUrlTag : public XmlTagBase<ExtUrlTag>
{
	Q_GADGET

public:
	/// The tags in X-macro form.  First is text usable as a C++ identifier,
	/// and second here is the text to use for the tag in an XML document.
	#define M_ExtUrlTags(X) \
		X(HREF, "href") \
		X(TS_LAST_REFRESH, "ts_last_refresh") \
		X(SIZE_FILE, "size_file") \
		X(TS_CREATION,"ts_creation") \
		X(TS_LAST_MODIFIED, "ts_last_modified") \
		X(TS_LAST_MODIFIED_METADATA, "ts_last_modified_metadata")

	enum TagEnum
	{
#define X(id, tag_str) id,
		M_ExtUrlTags(X)
#undef X
	};
	Q_ENUM(TagEnum)

#define X(id, tag_str) { id, tag_str },
/// @todo Not currently used.
	static constexpr std::tuple<TagEnum, std::string_view> m_testmap[] = { M_ExtUrlTags(X) };
#undef X

};
Q_DECLARE_METATYPE(ExtUrlTag);

static const auto ExtUrlTagToXMLTagMap = ExtUrlTag::make_map<ExtUrlTag::TagEnum, QString>(
{
			// This expands to an init list with entries which look like: {ExtUrlTag::HREF, "href"},
#define X(s, tag_str) { ExtUrlTag::s, tag_str },
			M_ExtUrlTags(X)
#undef X
});



#endif //AWESOMEMEDIALIBRARYMANAGER_SCANRESULTSTREEMODELXMLTAGS_H
