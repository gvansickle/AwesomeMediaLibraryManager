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
 * ExtUrl tags.  These are XML tags which exist in the ExtUrl "namespace".
 */
class ExtUrlTag : public XmlTagBase<ExtUrlTag>
{
	Q_GADGET

public:

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

//protected:

};
Q_DECLARE_METATYPE(ExtUrlTag);

static const auto ExtUrlTagToXMLTagMap = ExtUrlTag::make_map<ExtUrlTag::TagEnum, QString>(
{
			// This expands to an init list with entries which look like: {ExtUrlTag::HREF, "href"},
#define X(s, tag_str) { ExtUrlTag::s, tag_str },
				M_ExtUrlTags(X)
#undef X
});

/**
 * DirScanResult tags.
 */
class DSRTag : public ExtEnum<DSRTag>
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

static const auto DSRTagToXMLTagMap = DSRTag::make_map<DSRTag::TagName, QString>(
{
	{DSRTag::FLAGS_DIRPROPS, "flags_dirprops"},
	{DSRTag::EXTURL_DIR, "exturl_dir"},
	{DSRTag::EXTURL_MEDIA, "exturl_media"},
	{DSRTag::EXTURL_CUESHEET, "exturl_cuesheet"}
});

/**
 * ScanResultsTreeModelItem tags.
 */
class SRTMTag : public ExtEnum<SRTMTag>
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

static const auto SRTMTagToXMLTagMap = SRTMTag::make_map<SRTMTag::TagName, QString>(
{
	{SRTMTag::BASE_DIRECTORY, "base_directory"},
	{SRTMTag::TITLE, "title"},
	{SRTMTag::CREATOR, "creator"},
	{SRTMTag::DATE, "date"},
	{SRTMTag::ROOT_ITEM, "tree_model_root_item"}
});

/**
 * ScanResultsTreeModelItem tags.
 */
class SRTMItemTag : public ExtEnum<SRTMItemTag>
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

static const auto SRTMItemTagToXMLTagMap = SRTMItemTag::make_map<SRTMItemTag::TagName, QString>(
{
	{SRTMItemTag::DIRSCANRESULT, "dirscanresult"}
});

#endif //AWESOMEMEDIALIBRARYMANAGER_SCANRESULTSTREEMODELXMLTAGS_H
