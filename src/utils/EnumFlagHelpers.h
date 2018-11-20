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

/**
 * @file EnumFlagHelpers.h
 */
#ifndef SRC_UTILS_ENUMFLAGHELPERS_H_
#define SRC_UTILS_ENUMFLAGHELPERS_H_

#include <QString>
#include <QFlags>
#include <QMetaEnum>
#include <QMetaType>

// Ours
#include "StringHelpers.h"

/**
 * And the one thing you might want to use Qt's QMetaWhatever infrastructure for, implicitly or explicitly converting
 * a Q_ENUM() to a string?  You can't do directly.  So this.  The.  Simplest.  Things.
 * And Q_FLAG()s?  Yep, need to handle them separately.
 *
 * @note Yet, you can stream to QDebug and that works out of the box.
 *
 * @note It's "intentional".  No lie: @link https://bugreports.qt.io/browse/QTBUG-54210
 *
 * @param value  Any Q_ENUM() or Q_FLAG().
 * @return A QString representing the value of the Q_ENUM/Q_FLAG.
 */
template<typename QEnumType>
QString EnumFlagtoqstr(const QEnumType value)
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

template<typename F> // Output QFlags of registered enumerations
inline typename std::enable_if<QtPrivate::IsQEnumHelper<F>::Value, char*>::type toString_QF(QFlags<F> f)
{
	const QMetaEnum me = QMetaEnum::fromType<F>();
	return qstrdup(me.valueToKeys(int(f)).constData());
}

template <typename F> // Fallback: Output hex value
inline typename std::enable_if<!QtPrivate::IsQEnumHelper<F>::Value, char*>::type toString_QF(QFlags<F> f)
{
	const size_t space = 3 + 2 * sizeof(unsigned); // 2 for 0x, two hex digits per byte, 1 for '\0'
	char *msg = new char[space];
	qsnprintf(msg, space, "0x%x", unsigned(f));
	return msg;
}

template<typename QFlagsType>
QFlagsType QFlagsFromQStr(const QString& rep, bool *ok = nullptr)
{
	QMetaEnum me = QMetaEnum::fromType<QFlagsType>();

	// It's a Q_FLAG().
	Q_ASSERT(me.isFlag());

	return QFlagsType(me.keysToValue(tostdstr(rep).c_str(), ok));
}
//
//template<typename QEnumType>
//QEnumType QEnumFromQStr(const QString& rep)
//{
//	QMetaEnum me = QMetaEnum::fromType<QEnumType>();
//	if(QMetaEnum::fromType<QEnumType>().isFlag())
//	{
//		// It's a Q_FLAG().
//		return QString(me.valueToKeys(value));
//	}
//}

template <class FlagsType>
class QFlagQstringConverterRegistrationHelper
{
public:
	QFlagQstringConverterRegistrationHelper()
	{
		if(!QMetaType::hasRegisteredConverterFunction<FlagsType, QString>())
		{
			// Register converters between TestFlagHolder::TestFlags-to-QString for at least QVariant's benefit.
			bool success = QMetaType::registerConverter<FlagsType, QString>([](const FlagsType& flags) -> QString {
				return EnumFlagtoqstr(flags);
			});
			Q_ASSERT_X(success, __PRETTY_FUNCTION__, "FlagsType-to-QString converter registration failed");

			success = QMetaType::registerConverter<QString, FlagsType>([](const QString& str) -> FlagsType {
				return QFlagsFromQStr<FlagsType>(str);
			});
			Q_ASSERT_X(success, __PRETTY_FUNCTION__, "FlagsType-from-QString converter registration failed");
		}
	}
};

/**
 * Register QFlags-specialization-to/from-QString converters.
 *
 * @params FlagsType  The full "<holder_class_name>::<QFlags_name>" name of the QFlags type.
 * @params VarName    A name usable as a variable identifier, unique to the translation unit.
 */
#define AMLM_REGISTER_QFLAG_QSTRING_CONVERTERS(FlagsType, VarName) \
	Q_GLOBAL_STATIC(QFlagQstringConverterRegistrationHelper< FlagsType >, VarName ## _QString_converters_registration_obj)


#endif /* SRC_UTILS_ENUMFLAGHELPERS_H_ */
