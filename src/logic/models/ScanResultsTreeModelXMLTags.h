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

// Qt5
#include <QObject>
#include <QtXml>

template <class ScopeTypeEnumType, class ToType>
struct ExtEnumMapBase
{
	using maptype = std::map<ScopeTypeEnumType, ToType>;
public:
	ExtEnumMapBase(std::initializer_list<typename maptype::value_type> init_list) : m_ExtEnum_to_ToType_map(init_list)
	{
	};

//	static ExtEnumMapBase make_map(ScopeTypeEnumType, ToType);

	const ToType operator[](ScopeTypeEnumType i) const { return m_ExtEnum_to_ToType_map.at(i); };
	const ToType at(ScopeTypeEnumType i) const { return m_ExtEnum_to_ToType_map.at(i); };

	const std::map<ScopeTypeEnumType, ToType> m_ExtEnum_to_ToType_map;

};

template <class ScopeTypeEnumType>
using ExtEnumToStringMap = ExtEnumMapBase<ScopeTypeEnumType, QString>;

/**
 *
 */
class ExtUrlTag //: public ExtEnumMapBase<ExtUrlTag, ExtUrlTag::TagName>
{
	Q_GADGET

public:
	enum TagName
	{
		/// ExtUrl
		HREF,
		TS_LAST_REFRESH,
		SIZE_FILE,
		TS_CREATION,
		TS_LAST_MODIFIED,
		TS_LAST_MODIFIED_METADATA
	};
	Q_ENUM(TagName)

	/**
	 * Forwards ExtUrlTag operator[] to the std::map's .at() function, so we don't populate the map but rather throw.
	 */
//	const std::string operator[](ExtUrlTag::TagName i) { return m_exturltag_to_string[i]; };

	static const QString at(ExtUrlTag::TagName i) { return m_exturltag_to_string.at(i); };

protected:
	// Map from ExtUrlTag's to the std::string's which should be used in the XML.
	static const ExtEnumToStringMap<ExtUrlTag::TagName> m_exturltag_to_string;
//	static std::map<ExtUrlTag::TagName, std::string> m_exturltag_to_string;

};
Q_DECLARE_METATYPE(ExtUrlTag);

/**
 *
 */
class DSRTag
{
Q_GADGET
public:
	enum TagName
	{
		/// flags_dirprops	/*X(flags_dirprops, m_dir_props)*/
		EXTURL_DIR,
		EXTURL_MEDIA,
		EXTURL_CUESHEET
//	X(exturl_dir, m_dir_exturl) \
//	X(exturl_media, m_media_exturl) \
//	X(exturl_cuesheet, m_cue_exturl)
	};
	Q_ENUM(TagName)

	/**
	 * Forwards DSRTag operator[] to the std::map's .at() function, so we don't populate the map but rather throw.
	 */
	const QString operator[](DSRTag::TagName i) const { return m_dsrtag_to_string[i]; };

protected:
	static const ExtEnumToStringMap<DSRTag::TagName> m_dsrtag_to_string;
};
Q_DECLARE_METATYPE(DSRTag);


/**
 *
 */
class SRTMTag
{
	Q_GADGET
public:
	enum TagName
	{
		ROOT_ITEM,
		TITLE,
		CREATOR,
		DATE,
		BASE_DIRECTORY
	};
	Q_ENUM(TagName)

	/**
	 * Forwards SRTMTag operator[] to the std::map's .at() function, so we don't populate the map but rather throw.
	 */
	const QString operator[](SRTMTag::TagName i) const { return m_srtmtag_to_string[i]; };

protected:
	static const ExtEnumToStringMap<SRTMTag::TagName> m_srtmtag_to_string;
};
Q_DECLARE_METATYPE(SRTMTag);


#endif //AWESOMEMEDIALIBRARYMANAGER_SCANRESULTSTREEMODELXMLTAGS_H
