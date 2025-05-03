/*
 * Copyright 2017, 2019, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/// @file StringHelpers.h

#include <config.h>


// Std C++
#include <cstring>
#include <type_traits>
#include <string>

// Qt
#include <QtGlobal>
#include <QString>
#include <QStringList>
#include <QTime>
//#include <QTextCodec>
#include <text_encoding> // C++26.
#include <QUrl>
#include <QDebug>
#include <QMetaEnum>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#define HAVE_QLOCALE_FORMATTEDDATASIZE 1
#include <QLocale>
enum /*QLocale::*/DataSizeFormats
{
    /// Base 1024/IEC KiB, MiB, etc.
    DataSizeIecFormat = QLocale::DataSizeIecFormat,
    /// Base 1024/SI kB, MB, etc.
    DataSizeTraditionalFormat = QLocale::DataSizeTraditionalFormat,
    /// Base 1000/SI kB, MB, etc.
    DataSizeSIFormat = QLocale::DataSizeSIFormat
};
#endif

// KF
#if HAVE_KF501 || HAVE_KF6
#   ifndef HAVE_QLOCALE_FORMATTEDDATASIZE
    /// Needed to format numbers into "12.3 GB" etc. until we can rely on Qt 5.10, which supports
    /// it in QLocale.
#   include <KFormat>
#include <QTime>
enum /*QLocale::*/DataSizeFormats
{
    /// Base 1024/IEC KiB, MiB, etc.
    DataSizeIecFormat = 0,
    /// Base 1024/SI kB, MB, etc.
    DataSizeTraditionalFormat = 1,
    /// Base 1000/SI kB, MB, etc.
    DataSizeSIFormat = 2
};
#   endif // HAVE_QLOCALE_FORMATTEDDATASIZE
#endif

#if HAVE_GTKMM01
#include <glibmm/ustring.h>
#endif

// TagLib
#include <taglib/tag.h>

// Ours
//#include <utils/DebugHelpers.h> // For MSVC __PRETTY_FUNCTION__
/**
 * Portable __PRETTY_FUNCTION__.
 * @see @link https://msdn.microsoft.com/library/b0084kay.aspx
 */
#if defined(_MSC_VER)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#else
/* Nothing, gcc and clang's __PRETTY_FUNCTION__ are synonymous with __FUNCSIG__ */
#endif

#include <future/cpp14_concepts.hpp>

/// @name Functions for converting between the several thousand different and
/// non-interoperable UTF-8 string classes, one or more brought into the project per library used.
/// There are only two assumptions made here:
/// - Any const char* passed in is one of:
/// -- A pointer to a UTF-8 string.
/// -- A nullptr, in which case an empty std::string is returned.
/// - Any std::strings are likewise really holding UTF-8 strings.
///
/// The simplest things....
/// @{

/**
 * Convert a const char* to a std::string.  If @a cstr is nullptr, return @a if_null instead, which
 * defaults to a default constructed std::string().
 */
std::string tostdstr(const char* cstr, const std::string& if_null = std::string());

std::string tostdstr(const QString& qstr);

std::string tostdstr(const QUrl& qurl);

std::string tostdstr(const TagLib::String& tstr);

QString toqstr(const char* str);

QString toqstr(const std::string& str);

QString toqstr(const TagLib::String& tstr);

template<class IntegerType>
std::string tostr_hex(const IntegerType x)
{
	constexpr int max_hex_chars_for_type = sizeof(IntegerType)*2;
	constexpr int max_string_bytes_for_type = max_hex_chars_for_type + 2;

	std::string str; //(max_string_bytes_for_type, "0");
	str.resize(max_string_bytes_for_type);
	std::string fmt_str = "0x%0" + std::to_string(max_hex_chars_for_type) + "llx";
    std::sprintf(&(str[0]), fmt_str.c_str(), static_cast<unsigned long long>(x));

	return str;
}

#if HAVE_GTKMM01
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

bool isValidUTF8(const char* bytes);

/// @}

/**
 * And the one thing you might want to use Qt's QMetaWhatever infrastructure for, implicitly or explicitly converting
 * a Q_ENUM() to a string, you can't do directly.  So this.  The.  Simplest.  Things.
 * And Q_FLAG()s?  Yep, need to handle them separately.
 *
 * @note Yet, you can stream to QDebug and that works out of the box.
 *
 * @param value  Any Q_ENUM() or Q_FLAG().
 * @return A QString representing that Q_ENUM/Q_FLAG.
 */
template<typename QEnumType>
requires requires (QEnumType){ QtPrivate::IsQEnumHelper<QEnumType>::Value; }
QString toqstr(const QEnumType value)
{
	QMetaEnum me = QMetaEnum::fromType<QEnumType>();
	if(QMetaEnum::fromType<QEnumType>().isFlag())
	{
		// It's a Q_FLAG().
		return QString(me.valueToKeys(value));
	}
	else
	{
		// It's a Q_ENUM().
		return QString(me.valueToKey(value));
	}
}

/// @name Streaming helpers
/// @{

template <class StreamLikeType>
StreamLikeType log_QStringList(const QStringList& strlist, StreamLikeType out)
{
    QDebugStateSaver saver(out);
    for(const QString& str : strlist)
    {
        out << "  " << str << Qt::endl;
    }
    return out;
}

/// @}

/**
 * Until we can rely on Qt 5.10's QLocale::formattedDataSize().
 */
QString formattedDataSize(qint64 bytes, int precision = 2, DataSizeFormats format = DataSizeFormats::DataSizeIecFormat);

QString formattedDuration(qint64 msecs, int precision = 3);


/**
 *  Specialize std::formatter for QString, so std::format knows what to do with it.
 */
template <>
struct std::formatter<QString> : std::formatter<std::string>
{
    auto format(const QString& qstr, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(qstr.toStdString(), ctx);
    }
};

#endif // STRINGHELPERS_H
