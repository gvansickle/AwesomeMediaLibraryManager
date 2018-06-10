/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "MetadataAbstractBase.h"

#include "MetadataTaglib.h"
#include "utils/Fraction.h"
#include "utils/StringHelpers.h"
#include "utils/DebugHelpers.h"

#include <QDebug>

#include <map>
#include <string>

/**
 * Misc Metadata info:
 *
 * - DISCID (freedb):
 * Shows up in cuesheet as e.g. "REM DISCID D00DA810" (http://wiki.hydrogenaud.io/index.php?title=Cue_sheet, http://wiki.hydrogenaud.io/index.php?title=EAC_CUE_Sheets)
 * https://sound.stackexchange.com/questions/39229/how-is-discid-made-in-cue-sheet-files
 * " As mentioned in the DiscID howto that you can find here : http://ftp.freedb.org/pub/freedb/misc/freedb_howto1.07.zip,
 *   The disc ID is an 8-digit hexadecimal (base-16) number, computed using data from a CD's Table-of-Contents (TOC) in MSF (Minute Second Frame) form.
 *   This document includes a description of the algorithm used to compute the DiscID of a given audio CD."
 */

/// Interface name to Taglib name map.
/// @see http://wiki.hydrogenaud.io/index.php?title=Tag_Mapping
static const std::map<std::string, std::string> f_name_normalization_map =
{
	{"track_name", "TITLE"},
	{"track_number", "TRACKNUMBER"},
	{"track_total", ""},
	{"album_name", "ALBUM"},
	{"album_artist", "ALBUMARTIST"},
	{"track_artist", "ARTIST"},
	{"track_performer", "PERFORMER"},
	{"composer_name", "COMPOSER"},
	{"conductor_name", "CONDUCTOR"},
	{"genre", "GENRE"},
	{"media", "MEDIA"},
	{"ISRC", "ISRC"},
	{"catalog", "CATALOGNUMBER"},
	{"comment", "COMMENT"},
};

MetadataAbstractBase::MetadataAbstractBase()
{

}

static std::string reverse_lookup(const std::string& native_key)
{
	for(auto entry : f_name_normalization_map)
	{
		if(entry.second == native_key)
		{
			return entry.first;
		}
	}
	return native_key;
}

std::string MetadataAbstractBase::operator[](const std::string& key) const
{
	std::string native_key_string;

	auto it = f_name_normalization_map.find(key);
	if(it != f_name_normalization_map.end())
	{
		// Found it.
		native_key_string = it->second;
	}
	else
	{
		// Didn't find it.
		native_key_string = "";
		return native_key_string;
	}

//	TagLib::StringList stringlist = m_pm[native_key_string];
	decltype(m_tag_map)::mapped_type stringlist;

	auto strlist_it = m_tag_map.find(native_key_string);
	if(strlist_it != m_tag_map.end())
	{
		stringlist = strlist_it->second;
	}
	else
	{
//		qDebug() << "No such key:" << native_key_string;
	}

	if(stringlist.empty())
	{
		return "";
	}
	else
	{
		/// @todo EXP return stringlist[0].toCString(true);
		return stringlist[0];
	}
}

TagMap MetadataAbstractBase::filled_fields() const
{
	if(hasBeenRead() && !isError())
	{
        qDebug() << "Converting filled_fields to TagMap";
		TagMap retval;
		for(auto key_val_pairs : m_tag_map) ///@todo EXP m_pm)
		{
            qDebug() << "Native Key:" << key_val_pairs.first;
			std::string key = reverse_lookup(key_val_pairs.first); ///@todo EXP.toCString());
            qDebug() << "Normalized key:" << key;

			if(key.empty() || key.length() == 0)
			{
				// We found an unknown key.
M_WARNING("TODO: Find a better way to track new keys.")
				///f_newly_discovered_keys.insert(key_val_pairs.first); ///@todo EXP .toCString(true));
				continue;
			}

			std::vector<std::string> out_val;
			// Iterate over the StringList for this key.
			for(auto value : key_val_pairs.second)
			{
				/// @todo Not sure what I was doing here.
				///@todo EXP std::string val_as_utf8 = value.to8Bit(true);
				//qDebug() << "Value:" << val_as_utf8 << QString::fromUtf8(val_as_utf8.c_str());
				out_val.push_back(value); ///@todo EXP .toCString(true));
			}
			retval[key] = out_val;
		}
        qDebug() << "Returning:" << retval;
		return retval;
	}
	else
	{
		return TagMap();
	}
}

Fraction MetadataAbstractBase::total_length_seconds() const
{
	if(hasBeenRead() && !isError())
	{
		return Fraction(m_length_in_milliseconds, 1000);
	}
	else
	{
		return Fraction(0, 1);
	}
}

TrackMetadata MetadataAbstractBase::track(int i) const
{
	// Incoming index is 1-based.
	Q_ASSERT(i > 0);
//	if(size_t(i) >= m_tracks.size()+1)
//	{
//		qFatal("i: %d, m_tracks.size()+1: %lu", i, m_tracks.size()+1);
//	}

	try
	{
		TrackMetadata retval = m_tracks.at(i);
		return retval;
	}
	catch (...)
	{
		qFatal("No TrackMetadata found for i: %d, m_tracks.size()+1: %lu", i, m_tracks.size()+1);
	}

//	return retval;
}
