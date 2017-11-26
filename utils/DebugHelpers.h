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

#ifndef DEBUGHELPERS_H
#define DEBUGHELPERS_H

#include <type_traits>

#include <QString>
#include <QDebug>

static inline QDebug& operator<<(QDebug& d, const std::string& s)
{
	return d << QString::fromStdString(s);
}


template <typename T>
static inline auto idstr(const char *id_as_c_str, T id) -> std::enable_if_t<std::is_convertible<T, std::string>::value == true, std::string> /// SFINAE version for T already convertible to std::string.
{
	return std::string(id_as_c_str) + "(" + id + ")";
}

template <typename T>
static inline
std::enable_if_t<std::is_convertible<T, std::string>::value == false, std::string> /// SFINAE version for T not convertible to std::string.
idstr(const char *id_as_c_str, T id)
{
	return std::string(id_as_c_str) + "(" + std::to_string(id) + ")";
}

#define M_IDSTR(id) idstr(#id ": ", id) + ", " +

/// Portable compile-time warning.
#define STRINGISE_IMPL(x) #x
#define STRINGISE(x) STRINGISE_IMPL(x)
#define FILE_LINE_LINK __FILE__ "(" STRINGISE(__LINE__) "): "

// Use: M_WARNING("My message")
#if _MSC_VER
#   define M_WARNING(exp) __pragma(message(FILE_LINE_LINK "warning C2660: " exp))
#else//__GNUC__ - may need other defines for different compilers
#	define DO_PRAGMA(x) _Pragma(#x)
#   define M_WARNING(exp) DO_PRAGMA(message FILE_LINE_LINK "warning: " exp)
#endif

#endif // DEBUGHELPERS_H
