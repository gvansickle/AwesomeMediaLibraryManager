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
 * @file XmlSerializerTest.h
 */

// Google Test
#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>

// Qt
#include <QVariantMap>

// Ours
//#include <tests/TestHelpers.h>
#include <concurrency/tests/ExtAsyncTestCommon.h>
#include "../ISerializable.h"
#include "../ISerializer.h"
#include "../XmlSerializer.h"
#include <AMLMTagMap.h>

#include "ExtUrl.h"


/**
 * Test Suite (ISTQB) or "Test Case" (Google) for ExtAsyncTests.
 */
class XmlSerializerTests : public ExtAsyncTestsSuiteFixtureBase
{
//public:
//    // For type-parameterized tests.
//    using List = std::list<T>;
//    static T shared_;
//    T value_;

protected:

	// Objects declared here can be used by all tests in this Fixture.


};

class ToVariant : public ISerializable
{
public:
	explicit ToVariant(std::function<QVariant(void)> f, std::function<void(const QVariant&)> from_func)
		: m_to_func(f), m_from_func(from_func) { };
	QVariant toVariant() const override { return m_to_func();  };
	void fromVariant(const QVariant& variant) override { m_from_func(variant); };

	std::function<QVariant(void)> m_to_func;
	std::function<void(const QVariant&)> m_from_func;
};

class TestXmlSerializer : public XmlSerializer
{
public:
	void save(const ISerializable& serializable,
			const QUrl& file_url,
			const QString& root_element_name,
			std::function<void(void)> extra_save_actions = nullptr
			) override;

	bool load(ISerializable& serializable, const QUrl& file_url) override;

};

TEST_F(XmlSerializerTests, ShouldPass)
{
	EXPECT_TRUE(true);
}



TEST_F(XmlSerializerTests, TypeRoundTripping)
{
	// Round-tripped variant.
	QVariantMap round_tripped_variant;

	// To variant...
	ToVariant to_var([](){
		QVariantMap retval;

		QStringList sl; sl << "a" << "b" << "c";
		retval.insert("stringlist", sl);
		retval.insert("nextone", 126);

		return retval;
	},
	[&](const QVariant& variant){
		round_tripped_variant = variant.toMap();
	});

	QVariant to_variant = to_var.toVariant();
	to_var.fromVariant(to_variant);

	TCOUT << round_tripped_variant;

	EXPECT_EQ(to_variant, round_tripped_variant);
	EXPECT_EQ(round_tripped_variant["nextone"], 126);

	TCOUT << "nextone" << round_tripped_variant["nextone"];
}

TEST_F(XmlSerializerTests, AMLMTagMapRoundTripping)
{
	AMLMTagMap tm, tm2;

	tm.insert(AMLMTagMap::value_type("Hello", "Goodbye"));

	QVariantMap map;
	map.insert("testfield", QVariant::fromValue(tm));
	QVariant retval = map;

	EXPECT_TRUE(retval.isValid());

	QVariantMap frommap = retval.toMap();

	tm2 = frommap.value("testfield").value<AMLMTagMap>();

	auto vec = tm2.equal_range_vector("Hello");
	EXPECT_FALSE(vec.empty());
	EXPECT_EQ(vec[0], std::string("Goodbye"));
}

TEST_F(XmlSerializerTests, ExtUrlRoundTripping)
{
	ExtUrl u1(QUrl("file:///test.com/somefile.flac"));
	ExtUrl u2;

	EXPECT_NE(u1.m_url, u2.m_url);

	QVariantMap map;
	map.insert("test_field_name", QVariant::fromValue(u1));
	QVariant retval = map;

	EXPECT_TRUE(retval.isValid());

	QVariantMap frommap = retval.toMap();

	u2 = frommap.value("test_field_name").value<ExtUrl>();

	EXPECT_EQ(u1.m_url, u2.m_url);
}
