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
 * @file FlagsAndEnumsTests.cpp
 */
#include "FlagsAndEnumsTests.h"

// Std C++
#include <type_traits>
#include <atomic>
#include <functional>
#include <chrono>
#include <string>

// Future Std C++
#include <future/function_traits.hpp>
#include <future/future_type_traits.hpp>

// Qt5
#include <QTest>
#include <QString>
#include <QFlags>

// Google Test
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-spi.h> //< For EXPECT_NONFATAL_FAILURE() etc.

// Ours
#include <tests/TestHelpers.h>
#include <utils/DebugHelpers.h>

// Mocks
#include <tests/TestLifecycleManager.h>
#include <tests/IResultsSequenceMock.h>

// Classes Under Test.
#include <utils/EnumFlagHelpers.h>
///// @todo Split off
#include "../logic/ExtUrl.h"

/**
 * Test flag class definition.
 */
class TestFlagHolder
{
	Q_GADGET
public:

	enum TestFlag
	{
		Flag0 = 0x00,
		Flag1 = 0x01,
		Flag2 = 0x02,
		Flag4 = 0x04
	};
	Q_DECLARE_FLAGS(TestFlags, TestFlag)
	Q_FLAG(TestFlags)
};
Q_DECLARE_METATYPE(TestFlagHolder);
//Q_DECLARE_METATYPE(TestFlagHolder::TestFlags);
Q_DECLARE_OPERATORS_FOR_FLAGS(TestFlagHolder::TestFlags);

//static auto dummy_var_123 = [=](){ AMLMRegisterQFlagQStringConverters<TestFlagHolder::TestFlags>(); return 1; }();
static int dummy_456 = (AMLMRegisterQFlagQStringConverters<TestFlagHolder::TestFlags>(), 1);

/**
 * Test QEnum class definition.
 */
class TestEnumHolder
{
	Q_GADGET
public:

	enum TestEnum
	{
		Enum0 = 0x00,
		Enum1 = 0x01,
		Enum2 = 0x02,
		Enum3 = 0x03
	};
	Q_ENUM(TestEnum)
};
Q_DECLARE_METATYPE(TestEnumHolder);


//
// Tests
//

TEST_F(FlagsAndEnumsTests, FlagsToStringViaEnumFlagtoqstr)
{
	TestFlagHolder::TestFlags testflags { TestFlagHolder::Flag1 | TestFlagHolder::Flag4 };

	QString flags_as_str = EnumFlagtoqstr(testflags);

	EXPECT_EQ(flags_as_str, QString("Flag1|Flag4"));
}

TEST_F(FlagsAndEnumsTests, FlagsToStringViaQVariant)
{
	// Up to at least Qt5.11.1, Q_ENUM()s can be converted to QStrings when held in QVariants,
	// but Q_FLAG()s have no such built-in support, hence the EXPECT_NE() below.

	TestFlagHolder::TestFlags testflags { TestFlagHolder::Flag1 | TestFlagHolder::Flag4 };

	QString flags_as_str = QVariant::fromValue(testflags).toString();

	// For multiple runs, can't seem to unregister converters.
	if(QMetaType::hasRegisteredConverterFunction<TestFlagHolder::TestFlags, QString>())
	{
		EXPECT_EQ(flags_as_str, QString("Flag1|Flag4"));
	}
	else
	{
		EXPECT_NE(flags_as_str, QString("Flag1|Flag4"));
	}
}
TEST_F(FlagsAndEnumsTests, FlagsRoundTripThroughQVariantStringRepWithRegisteredConverters)
{
	// @link https://stackoverflow.com/questions/36532527/qflags-and-qvariant

//	bool success = QMetaType::hasRegisteredConverterFunction<TestFlagHolder::TestFlags, QString>();
//	EXPECT_FALSE(success);

	EXPECT_FALSE(QMetaType::hasRegisteredDebugStreamOperator<TestFlagHolder::TestFlags>());
	int id = qMetaTypeId<TestFlagHolder::TestFlags>();
//	EXPECT_TRUE(QMetaType::isRegistered<TestFlagHolder::TestFlags>());

	// Can't register an implicit conversion from QFlags<> type to QString. Fails to compile.
//	success = QMetaType::registerConverter<TestFlagHolder::TestFlags, QString>();
//	EXPECT_FALSE(success);

	// For multiple test runs; can't seem to unregister converters.
	if(!QMetaType::hasRegisteredConverterFunction<TestFlagHolder::TestFlags, QString>())
	{
		// Register converters between TestFlagHolder::TestFlags-to-QString for at least QVariant's benefit.
		bool success = QMetaType::registerConverter<TestFlagHolder::TestFlags, QString>([](const TestFlagHolder::TestFlags& flags) -> QString {
			return EnumFlagtoqstr(flags);
		});
		EXPECT_TRUE(success);

		success = QMetaType::registerConverter<QString, TestFlagHolder::TestFlags>([](const QString& str) -> TestFlagHolder::TestFlags {
			return QFlagsFromQStr<TestFlagHolder::TestFlags>(str);
		});
		EXPECT_TRUE(success);
	}

	TestFlagHolder::TestFlags testflags_original { TestFlagHolder::Flag1 | TestFlagHolder::Flag2 };
	TestFlagHolder::TestFlags testflags = testflags_original;

	// To QVariant with converters but no special action at callsite.
	QVariant flags_as_qvar_with_converters = QVariant::fromValue(testflags);

	/// @note Comes out like I think we want it to: QVariant(TestFlagHolder::TestFlags, "Flag1|Flag2")
	TCOUT << "CONVERTERS REGISTERED, via QDebug operator<<:" << flags_as_qvar_with_converters;

	/// @note Comes out as "Flag1|Flag2", which is what we want for serialization.
	TCOUT << "CONVERTERS REGISTERED, via <qvariant>.toString():" << flags_as_qvar_with_converters.toString();

	// Ok, let's try to round-trip from

	// Convert QVariant<QFlags<>> to a QString.
	// Comes out as "Flag1|Flag2".
	QString flags_as_qstr = QVariant::fromValue(testflags).toString();
	TCOUT << M_NAME_VAL(flags_as_qstr);
	EXPECT_EQ(flags_as_qstr, QString("Flag1|Flag2"));


	// Convert the string into a variant.
	QVariant flags_as_qvar_from_qstr = QVariant::fromValue(flags_as_qstr);
//	QVariant flags_as_qvar = QVariant::fromValue<TestFlagHolder::TestFlags>(flags_as_qstr);
	EXPECT_TRUE(flags_as_qvar_from_qstr.canConvert<TestFlagHolder::TestFlags>());

	// This looks like "QFlags<TestFlagHolder::TestFlags>(Flag1|Flag2)"
	TCOUT << "FLAGS:" << testflags;
	// This looks like "QVariant(QString, "Flag1|Flag2")".
	TCOUT << "FLAGS AS QVAR:" << flags_as_qvar_from_qstr;
	TCOUT << "FLAGS EXTRACTED FROM QVAR WITH .value<TestFlagHolder::TestFlags>():" << flags_as_qvar_from_qstr.value<TestFlagHolder::TestFlags>();
//	TCOUT << "FLAGS EXTRACTED FROM QVAR WITH .value<>():" << flags_as_qvar_from_qstr.value();
//	TCOUT << "FLAGS EXTRACTED FROM QVAR WITH .value<>().toString():" << flags_as_qvar.value<TestFlagHolder::TestFlags>().toString();

	TestFlagHolder::TestFlags testflags_through_qvar;

	testflags_through_qvar = flags_as_qvar_from_qstr.value<TestFlagHolder::TestFlags>();

	EXPECT_EQ(testflags_through_qvar, testflags_original);
}
TEST_F(FlagsAndEnumsTests, FlagsRoundTripThroughQVariantDefault)
{
	TestFlagHolder::TestFlags testflags_original { TestFlagHolder::Flag1 | TestFlagHolder::Flag2 };
	TestFlagHolder::TestFlags testflags = testflags_original;

	auto flags_as_qvar = QVariant::fromValue(testflags);

	TestFlagHolder::TestFlags testflags_through_qvar;

	testflags_through_qvar = flags_as_qvar.value<TestFlagHolder::TestFlags>();

	EXPECT_EQ(testflags_through_qvar, testflags_original);
}

TEST_F(FlagsAndEnumsTests, FlagsRoundTripThroughQVariantStringRep)
{
	// @link https://stackoverflow.com/questions/36532527/qflags-and-qvariant

	/// @note Appears to do nothing.
//	qRegisterMetaTypeStreamOperators<TestFlagHolder::TestFlags>("TestFlagHolder::TestFlags");

	TestFlagHolder::TestFlags testflags_original { TestFlagHolder::Flag1 | TestFlagHolder::Flag2 };
	TestFlagHolder::TestFlags testflags = testflags_original;

	QVariant flags_as_qvar = QVariant::fromValue(testflags);

	// QFlags<TestFlagHolder::TestFlags>(Flag1|Flag2)
	TCOUT << "FLAGS:" << testflags;

	// Note there is nothing where the value should be.
	// QVariant(TestFlagHolder::TestFlags, )
	TCOUT << "FLAGS AS QVAR:" << flags_as_qvar;

	// QFlags<TestFlagHolder::TestFlags>(Flag1|Flag2)
	TCOUT << "FLAGS EXTRACTED FROM QVAR, <<:" << flags_as_qvar.value<TestFlagHolder::TestFlags>();

	// integer 3
	TCOUT << QString("FLAGS EXTRACTED FROM QVAR, arg(): %1").arg(flags_as_qvar.value<TestFlagHolder::TestFlags>());

	TestFlagHolder::TestFlags testflags_through_qvar;

	testflags_through_qvar = flags_as_qvar.value<TestFlagHolder::TestFlags>();

	EXPECT_EQ(testflags_through_qvar, testflags_original);
}

TEST_F(FlagsAndEnumsTests, EnumToStringViaQVariant)
{
	// Up to at least Qt5.11.1, Q_ENUM()s can be converted to QStrings when held in QVariants,
	// but Q_FLAG()s have no such built-in support.
	TestEnumHolder::TestEnum test_enum { TestEnumHolder::Enum1 };

	QString enum_as_str = QVariant::fromValue<TestEnumHolder::TestEnum>(test_enum).toString();

	EXPECT_EQ(enum_as_str, QString("Enum1"));
}

TEST_F(FlagsAndEnumsTests, EnumRoundTripThroughQVariantStringRep)
{
	TestEnumHolder::TestEnum testflags_original { TestEnumHolder::Enum1 };
	TestEnumHolder::TestEnum testflags = testflags_original;

	auto flags_as_qvar = QVariant::fromValue(testflags);

	TCOUT << "ENUM:" << testflags;
	TCOUT << "ENUM AS QVAR:" << flags_as_qvar;

	TestEnumHolder::TestEnum testflags_through_qvar;

	testflags_through_qvar = flags_as_qvar.value<TestEnumHolder::TestEnum>();

	EXPECT_EQ(testflags_through_qvar, testflags_original);
}

////////////////////////////////////////////////////////////
M_WARNING("TODO: Split into a serialization test.");

TEST_F(FlagsAndEnumsTests, ExtUrlRoundTripThroughQVariant)
{
	ExtUrl before;

	before.m_url = "file://a.b.com/";

	QVariant during = before.toVariant();

	ExtUrl after;
	after.fromVariant(during);

	TCOUT << M_NAME_VAL(before);
	TCOUT << M_NAME_VAL(during);
	TCOUT << M_NAME_VAL(after);

//	AMLMTEST_EXPECT_EQ(before, after);
}

TEST_F(FlagsAndEnumsTests, QUrlRoundTripThroughQVariant)
{
	QUrl before;

	before = "file://a.b.com/";

	QVariant during = QVariant::fromValue(before);

	TCOUT << "TYPENAME:" << during.typeName();

	QUrl after;
	after = during.value<QUrl>();

	TCOUT << M_NAME_VAL(before);
	TCOUT << M_NAME_VAL(during);
	TCOUT << M_NAME_VAL(after);

	AMLMTEST_EXPECT_EQ(before, after);
}

#include "FlagsAndEnumsTests.moc"
