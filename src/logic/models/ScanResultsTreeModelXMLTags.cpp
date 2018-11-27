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

#include "ScanResultsTreeModelXMLTags.h"

const ExtEnumToStringMap<ExtUrlTag::TagName> ExtUrlTag::m_exturltag_to_string
{
	{ ExtUrlTag::HREF, "exturl_dir"},
	{ ExtUrlTag::TS_LAST_REFRESH, "ts_last_refresh"},
	{ ExtUrlTag::SIZE_FILE, "size_file"},
	{ ExtUrlTag::TS_CREATION, "ts_creation"},
	{ ExtUrlTag::TS_LAST_MODIFIED, "ts_last_modified"},
	{ ExtUrlTag::TS_LAST_MODIFIED_METADATA, "ts_last_modified_metadata"}
};

//const ExtEnumToStringMap<DSRTag::TagName> DSRTag::m_dsrtag_to_string
//{
//	{DSRTag::EXTURL_DIR, "exturl_dir"},
//	{DSRTag::EXTURL_MEDIA, "exturl_media"},
//	{DSRTag::EXTURL_CUESHEET, "exturl_cuesheet"}
//};

//static const auto DSRTagToXMLTagMap = make_map<DSRTag::TagName, QString>({
//		                                          {DSRTag::EXTURL_DIR, "exturl_dir"},
//		                                          {DSRTag::EXTURL_MEDIA, "exturl_media"},
//		                                          {DSRTag::EXTURL_CUESHEET, "exturl_cuesheet"}
//                                          });

const ExtEnumToStringMap<SRTMTag::TagName> SRTMTag::m_srtmtag_to_string
{
	{SRTMTag::BASE_DIRECTORY, "base_directory"},
	{SRTMTag::TITLE, "title"},
	{SRTMTag::CREATOR, "creator"},
	{SRTMTag::DATE, "date"},
	{SRTMTag::ROOT_ITEM, "tree_model_root_item"}
};
