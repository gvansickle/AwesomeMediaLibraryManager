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


#ifndef SRC_LOGIC_TAGLIBHELPERS_H_
#define SRC_LOGIC_TAGLIBHELPERS_H_


/// @name The TagLib::FileRef constructor takes a TagLib::FileName, which:
/// - on Linux is typedef for const char *
/// - on Windows is an actual class with both const char * and const wchar_t * members.
/// So here's a couple templates to smooth this over.
/// @{
template<typename StringType, typename FNType = TagLib::FileName>
std::enable_if_t<std::is_same<FNType, const char*>::value, TagLib::FileRef>
openFileRef(const StringType& local_path)
{
	// TagLib::FileName is a const char *, so this is likely Linux.  Translate the QString accordingly and open the FileRef.
	return TagLib::FileRef(local_path.toStdString().c_str());
}
template<typename StringType, typename FNType = TagLib::FileName>
std::enable_if_t<!std::is_same<FNType, const char *>::value, TagLib::FileRef>
openFileRef(const StringType& local_path)
{
	// TagLib::FileName is not const char *, so this is likely Windows.  Translate the QString accordingly and open the FileRef.
	return TagLib::FileRef(local_path.toStdWString().c_str());
}
/// @}


#endif /* SRC_LOGIC_TAGLIBHELPERS_H_ */
