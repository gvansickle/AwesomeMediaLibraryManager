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
#include <taglib/tag.h>
#include <QTextCodec>

#warning "TODO: SET THIS IN CMAKE"
#define HAVE_GLIBMM
#ifdef HAVE_GLIBMM
#include <glibmm/ustring.h>
#endif

/// Functions for converting between the several thousand different and
/// non-interoperable UTF-8 string classes, one or more brought into the project per library used.
/// There are only two assumptions made here:
/// - Any const char* passed in is a pointer to a UTF-8 string.
/// - Any std::strings are likewise really holding UTF-8 strings.
///
/// The simplest things....

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

static inline std::string tostdstr(const QString& qstr)
{
	// From the Qt5 docs:
	// "Returns a std::string object with the data contained in this QString. The Unicode data is converted into 8-bit characters using the toUtf8() function."
	return qstr.toStdString();
}

static inline std::string tostdstr(const TagLib::String &tstr)
{
	/* From TagLib: "Returns a deep copy of this String as an std::string.  The returned string
     * is encoded in UTF8 if \a unicode is true [...]."
     */
	return tstr.to8Bit(true);
}

static inline QString toqstr(const char * str)
{
	// From the QT5 docs:
	// "QString QString::fromStdString(const std::string &str)
	//	Returns a copy of the str string. The given string is converted to Unicode using the fromUtf8() function."
	return QString::fromStdString(std::string(str));
}

static inline QString toqstr(const std::string &str)
{
	// From the QT5 docs:
	// "QString QString::fromStdString(const std::string &str)
	//	Returns a copy of the str string. The given string is converted to Unicode using the fromUtf8() function."
	return QString::fromStdString(str);
}

static inline QString toqstr(const TagLib::String& tstr)
{
	// Convert from TagLib::String (UTF-16) to QString (UTF-16).
	return QString::fromStdString(tstr.to8Bit(true));
}

#ifdef HAVE_GLIBMM
static inline Glib::ustring toustring(const QString& qstr)
{
    // "Glib::ustring has implicit type conversions to and from std::string.
    // These conversions do @em not convert to/from the current locale [...]"
    return tostdstr(qstr);
}
#endif

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
	return out << toqstr(str);
}

/// For TagLib::StringList to QStringList.
//template<typename LHSType, typename T>
//LHSType& operator<<(LHSType& out, const T& strlist)
//{
//	return out << LHSType::fromUtf8(strlist.toString().toCString(true));
//}

static inline bool isValidUTF8(const char* bytes)
{
	QTextCodec::ConverterState state;
	QTextCodec *codec = QTextCodec::codecForName("UTF-8");
	const QString text = codec->toUnicode(bytes, strlen(bytes), &state);
	if (state.invalidChars > 0)
	{
		return false;
	}
	return true;
}

#endif // STRINGHELPERS_H
