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

#include <config.h>

#include "ExtFutureProgressInfo.h"

// Qt5
#include <QStringList>

#include <utils/StringHelpers.h>
#include <utils/DebugHelpers.h>

//static QStringList replaceInStrings(const QStringList &strlist, const QString& char_to_escape, const QString& escaped_str)
//{
//	auto retval = strlist;

//	retval.replaceInStrings(char_to_escape, escaped_str);

//	return retval;
//}

const QString f_split_str = QStringLiteral(R"(^)");

static QStringList escape(const QStringList &strlist, const QString& char_to_escape)
{
	auto retlist = strlist;

	retlist.replaceInStrings(char_to_escape, R"(\)" + char_to_escape);

	return retlist;
}

static QStringList unescape(const QStringList &strlist, const QString& char_to_escape)
{
	auto retlist = strlist;

	retlist.replaceInStrings(R"(\)" + char_to_escape, char_to_escape);

	retlist.replaceInStrings(R"(\\)", R"(\)");

	return retlist;
}

void ExtFutureProgressInfo::fromKJobDescription(const QString& title, const QPair<QString, QString>& field1, const QPair<QString, QString>& field2)
{
	QStringList strlist;

	strlist << title << field1.first << field1.second << field2.first << field2.second;
	QStringList original_strlist = strlist;
	strlist = escape(strlist, f_split_str);

	if(false) // Debug
	{
		qDb() << "ORIGINAL:" << original_strlist;
		QStringList check_unescaped = unescape(strlist, f_split_str);
		qDb() << "UNESCAPED:" << check_unescaped;
		Q_ASSERT(original_strlist == check_unescaped);
	}

	QString str = strlist.join(f_split_str);
//	// join looks like it's sort of broken, it doesn't add the separator to the beginning or end of the resulting string.
//	str = f_split_str + str + f_split_str;
	// Prepend the unescaped type to the string.
	str.prepend(toqstr(EncodedType::DESC) + ":");

//	qDb() << "DESCRIPTION:" << str;

	*this = str;
}

void ExtFutureProgressInfo::fromKJobInfoMessage(const QString& plain, const QString& rich)
{
	QStringList strlist;

	strlist << plain << rich;
	strlist = escape(strlist, f_split_str);
	auto str = strlist.join(f_split_str);
	// Prepend the unescaped type to the string.
	str.prepend(toqstr(EncodedType::INFO) + ":");

//	qDb() << "INFOMESSAGE:" << str;

	*this = str;
}

void ExtFutureProgressInfo::fromKJobWarning(const QString& plain, const QString& rich)
{
	QStringList strlist;

	strlist << plain << rich;
	strlist = escape(strlist, f_split_str);
	auto str = strlist.join(f_split_str);
	// Prepend the unescaped type to the string.
	str.prepend(toqstr(EncodedType::WARN) + ":");

//	qDb() << "WARNING:" << str;

	*this = str;
}

void ExtFutureProgressInfo::fromSetProgressUnit(int kob_progress_unit)
{
	QStringList strlist;

	strlist << QString("%1").arg(kob_progress_unit);
	strlist = escape(strlist, f_split_str);
	auto str = strlist.join(f_split_str);
	// Prepend the unescaped type to the string.
	str.prepend(toqstr(EncodedType::SET_PROGRESS_UNIT) + ":");

	*this = str;
}

QPair<ExtFutureProgressInfo::EncodedType, QStringList> ExtFutureProgressInfo::fromQString(const QString& str)
{
	QPair<EncodedType, QStringList> retval;
	EncodedType retenum = EncodedType::UNKNOWN;
	QStringList retlist;

	auto metaenum = QMetaEnum::fromType<EncodedType>();
	for(int index = 0; index < metaenum.keyCount(); ++index)
	{
		const char* key = metaenum.key(index);
		int value = metaenum.value(index);

//		qDb() << "Checking key:" << key;
		if(str.startsWith(key))
		{
			// Found it.
//			qDb() << "Found match:" << key;
			retenum = static_cast<EncodedType>(value);

			QString qkey = toqstr(retenum);
//			qDb() << "qkey:" << qkey;
			// Strip off the prefix plus the ":".
			QString strcopy = str.mid(qkey.length()+1);
			// Strip off leading/trailing spaces.
			strcopy = strcopy.trimmed();

//			qDb() << "ABOUT TO SPLIT THIS:" << strcopy;

			// Parse the string appropriately.
			retlist = strcopy.split(f_split_str);

			break;
		}
	}

	retval = qMakePair(retenum, retlist);

	// Will be EncodedType::UNKNOWN/empty if it wasn't recognized.
	return retval;
}
