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

// Ours
#include <logic/xml/ExtEnum.h>



template <class ScopeTypeEnumType>
using ExtEnumToStringMap = ExtEnumMapBase<ScopeTypeEnumType, QString>;

/**
 *
 */
class ExtUrlTag
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

	/// @todo Should find a way to add to-map members here.

};
Q_DECLARE_METATYPE(ExtUrlTag);

static const auto ExtUrlTagToXMLTagMap = make_map<ExtUrlTag::TagName, QString>(
{
	{ExtUrlTag::HREF, "exturl_dir"},
	{ExtUrlTag::TS_LAST_REFRESH, "ts_last_refresh"},
	{ExtUrlTag::SIZE_FILE, "size_file"},
	{ExtUrlTag::TS_CREATION, "ts_creation"},
	{ExtUrlTag::TS_LAST_MODIFIED, "ts_last_modified"},
	{ExtUrlTag::TS_LAST_MODIFIED_METADATA, "ts_last_modified_metadata"}
});

/**
 *
 */
class DSRTag
{
	Q_GADGET

public:
	enum TagName
	{
		FLAGS_DIRPROPS, /*X(flags_dirprops, m_dir_props)*/
		EXTURL_DIR,
		EXTURL_MEDIA,
		EXTURL_CUESHEET
	};
	Q_ENUM(TagName)
};
Q_DECLARE_METATYPE(DSRTag);

static const auto DSRTagToXMLTagMap = ExtEnum::make_map<DSRTag::TagName, QString>(
{
	{DSRTag::FLAGS_DIRPROPS, "flags_dirprops"},
	{DSRTag::EXTURL_DIR, "exturl_dir"},
	{DSRTag::EXTURL_MEDIA, "exturl_media"},
	{DSRTag::EXTURL_CUESHEET, "exturl_cuesheet"}
});

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

};
Q_DECLARE_METATYPE(SRTMTag);

static const auto SRTMTagToXMLTagMap = ExtEnum::make_map<SRTMTag::TagName, QString>(
{
	{SRTMTag::BASE_DIRECTORY, "base_directory"},
	{SRTMTag::TITLE, "title"},
	{SRTMTag::CREATOR, "creator"},
	{SRTMTag::DATE, "date"},
	{SRTMTag::ROOT_ITEM, "tree_model_root_item"}
});

/**
 *
 */
class SRTMItemTag
{
Q_GADGET
public:
	enum TagName
	{
		DIRSCANRESULT
	};
	Q_ENUM(TagName)

};
Q_DECLARE_METATYPE(SRTMItemTag);

static const auto SRTMItemTagToXMLTagMap = ExtEnum::make_map<SRTMItemTag::TagName, QString>(
{
	{SRTMItemTag::DIRSCANRESULT, "dirscanresult"}
});

#endif //AWESOMEMEDIALIBRARYMANAGER_SCANRESULTSTREEMODELXMLTAGS_H
