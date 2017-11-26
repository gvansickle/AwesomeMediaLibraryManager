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

#ifndef STRINGHELPERS_H
#define STRINGHELPERS_H

#include <type_traits>
#include <string>
#include <QString>

static inline std::string tostdstr(const char *cstr)
{
	// Handles the case where cstr == nullptr.
	if(cstr == nullptr)
	{
		return std::string();
	}
	else
	{
		return std::string(cstr);
	}
}

static inline QString toqstr(const std::string &str)
{
	// From the QT5 docs:
	// "QString QString::fromStdString(const std::string &str)
	//	Returns a copy of the str string. The given string is converted to Unicode using the fromUtf8() function."
	return QString::fromStdString(str);
}

/// From a QStringList, return the first one.  If there isn't a first one, return an empty string.
template <typename StringListType, typename StringType = typename StringListType::value_type>
static inline StringType get_first(const StringListType& sl)
{
	if(sl.size() == 0)
	{
		StringType retval = StringType("");
		return retval;
	}
	else
	{
		return sl[0];
	}
}

/// For TagLib::String.
template<typename LHSType, typename T>
std::enable_if_t<std::is_same<T,TagLib::String>::value, LHSType&>
operator<<(LHSType& out, const T& str)
{
	return out << str.toCString(true);
}

/// For TagLib::StringList to QStringList.
//template<typename LHSType, typename T>
//LHSType& operator<<(LHSType& out, const T& strlist)
//{
//	return out << LHSType::fromUtf8(strlist.toString().toCString(true));
//}



#endif // STRINGHELPERS_H
