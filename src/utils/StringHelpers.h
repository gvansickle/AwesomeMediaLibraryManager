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

#include <config.h>

// Std C++
#include <type_traits>
#include <string>

// Qt5
#include <QtGlobal>
#include <QString>
#include <QTime>
#include <QTextCodec>
#include <QUrl>

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

// KF5
#if HAVE_KF501
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

/// @name Functions for converting between the several thousand different and
/// non-interoperable UTF-8 string classes, one or more brought into the project per library used.
/// There are only two assumptions made here:
/// - Any const char* passed in is a pointer to a UTF-8 string.
/// - Any std::strings are likewise really holding UTF-8 strings.
///
/// The simplest things....
/// @{

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

static inline std::string tostdstr(const QUrl& qurl)
{
	return tostdstr(qurl.toString());
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

/// @}

/**
 * Until we can rely on Qt 5.10's QLocale::formattedDataSize().
 */
static inline QString formattedDataSize(qint64 bytes, int precision = 2, DataSizeFormats format = DataSizeFormats::DataSizeIecFormat)
{
#ifdef HAVE_QLOCALE_FORMATTEDDATASIZE
    // Use the Qt5 version.
    return QLocale().formattedDataSize(bytes, precision, (QLocale::DataSizeFormats)format);
#else
    // Use the KF5 equivalent.
    KFormat::BinaryUnitDialect dialect;
    switch(format)
    {
    case DataSizeFormats::DataSizeIecFormat:
        dialect = KFormat::IECBinaryDialect;
        break;
    case DataSizeFormats::DataSizeTraditionalFormat:
        dialect = KFormat::JEDECBinaryDialect;
        break;
    case DataSizeFormats::DataSizeSIFormat:
        dialect = KFormat::MetricBinaryDialect;
    }
    return KFormat().formatByteSize(bytes, precision, dialect);
#endif
}

static inline QString formattedDuration(qint64 msecs, int precision = 3)
{
    Q_ASSERT(precision >= 0);
    Q_ASSERT(precision <= 3);
    // T=0.
    QTime t(0,0);
    Qt::DateFormat duration_fmt;
    int chars_to_clip_from_end = 0;
    if(precision > 0)
    {
        // Default precision 3 is ISO 8601 "HH:mm:ss.zzz"
        duration_fmt = Qt::ISODateWithMs;
        chars_to_clip_from_end = 3-precision;
    }
    else if(precision == 0)
    {
        // ISO 8601, "HH:mm:ss".
        duration_fmt = Qt::ISODate;
        chars_to_clip_from_end = 0;
    }
    QString secs_str = t.addMSecs(msecs).toString(duration_fmt);
    Q_ASSERT(secs_str.size() > 0);
    secs_str.resize(secs_str.size()-chars_to_clip_from_end);

    return secs_str;
}


#endif // STRINGHELPERS_H
