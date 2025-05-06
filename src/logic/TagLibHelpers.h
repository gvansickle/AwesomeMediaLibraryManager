/*
 * Copyright 2018, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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


#ifndef SRC_LOGIC_TAGLIBHELPERS_H_
#define SRC_LOGIC_TAGLIBHELPERS_H_

/** @file TagLibHelpers.h */

#include <config.h>

// Std C++.
#include <type_traits>

// TagLib.
#include <taglib/fileref.h>
#include <taglib/audioproperties.h>

/// @name The TagLib::FileRef constructor takes a TagLib::FileName, which:
/// - on Linux is typedef for const char *
/// - on Windows is an actual class with both const char * and const wchar_t * members.
/// So here's a couple templates to smooth this over.
/// @{
template<typename StringType, typename FNType = TagLib::FileName>
	std::enable_if_t<std::is_same<FNType, const char*>::value, TagLib::FileRef>
openFileRef(const StringType& local_path, bool readAudioProperties=true,
            TagLib::AudioProperties::ReadStyle audioPropertiesStyle=TagLib::AudioProperties::Average)
{
	// TagLib::FileName is a const char *, so this is likely Linux.  Translate the QString accordingly and open the FileRef.
	return TagLib::FileRef(local_path.toStdString().c_str(), readAudioProperties, audioPropertiesStyle);
}
template<typename StringType, typename FNType = TagLib::FileName>
	std::enable_if_t<!std::is_same<FNType, const char *>::value, TagLib::FileRef>
openFileRef(const StringType& local_path, bool readAudioProperties=true,
            TagLib::AudioProperties::ReadStyle audioPropertiesStyle=TagLib::AudioProperties::Average)
{
	// TagLib::FileName is not const char *, so this is likely Windows.  Translate the QString accordingly and open the FileRef.
	return TagLib::FileRef(local_path.toStdWString().c_str(), readAudioProperties, audioPropertiesStyle);
}
/// @}


#endif /* SRC_LOGIC_TAGLIBHELPERS_H_ */
