/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/**
 * @file StringHelpers.cpp
 */

#include "StringHelpers.h"





std::string tostdstr(const char* cstr, const std::string& if_null)
{
	// Handles the case where cstr == nullptr.
	if(cstr == nullptr)
	{
		return if_null;
	}
	else
	{
		return std::string(cstr);
	}
}

std::string tostdstr(const QString& qstr)
{
	// From the Qt5 docs:
	// "Returns a std::string object with the data contained in this QString. The Unicode data is converted into 8-bit characters using the toUtf8() function."
	return qstr.toStdString();
}

std::string tostdstr(const QUrl& qurl)
{
	return tostdstr(qurl.toString());
}

std::string tostdstr(const TagLib::String& tstr)
{
	/* From TagLib: "Returns a deep copy of this String as an std::string.  The returned string
	 * is encoded in UTF8 if \a unicode is true [...]."
	 */
	return tstr.to8Bit(true);
}

QString toqstr(const char* str)
{
	// From the QT5 docs:
	// "QString QString::fromStdString(const std::string &str)
	//	Returns a copy of the str string. The given string is converted to Unicode using the fromUtf8() function."
	return QString::fromStdString(std::string(str));
}

QString toqstr(const std::string& str)
{
	// From the QT5 docs:
	// "QString QString::fromStdString(const std::string &str)
	//	Returns a copy of the str string. The given string is converted to Unicode using the fromUtf8() function."
	return QString::fromStdString(str);
}

QString toqstr(const TagLib::String& tstr)
{
	// Convert from TagLib::String (UTF-16) to QString (UTF-16).
	return QString::fromStdString(tstr.to8Bit(true));
}

bool isValidUTF8(const char* bytes)
{
	auto toUtf16 = QStringDecoder(QStringDecoder::Utf8);
	QString text = toUtf16(bytes);
	Q_UNUSED(text);
	if(toUtf16.hasError())
	{
		return false;
	}
	return true;
}

QString formattedDataSize(qint64 bytes, int precision, DataSizeFormats format)
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

QString formattedDuration(qint64 msecs, int precision)
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
	else
	{
		Q_ASSERT_X(0, __PRETTY_FUNCTION__, "duration_fmt used uninitialized");
	}
	QString secs_str = t.addMSecs(msecs).toString(duration_fmt);
	Q_ASSERT(secs_str.size() > 0);
	secs_str.resize(secs_str.size()-chars_to_clip_from_end);

	return secs_str;
}

