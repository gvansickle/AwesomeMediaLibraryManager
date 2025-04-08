/*
 * Copyright 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
* @file SerializationTests.cpp.
*/

// Google Test
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Qt
#include <QVariant>
#include <QUrl>

// Ours
#include "TestHelpers.h"

// Ours, under test.
#include <logic/Metadata.h>
#include "ExtUrl.h"
#include <logic/TrackMetadata.h>
#include <logic/TrackIndex.h>
#include <logic/AMLMTagMap.h>


class SerializationTests : public ::testing::Test
{
protected:
};


TEST_F(SerializationTests, TrackMetadataThroughQVariant)
{
	TrackMetadata metadata1;

    metadata1.m_track_number = 3;
    metadata1.m_start_frames = 45;
	TrackIndex ti;
    ti.m_index_num="00";
    ti.m_index_frames=67;
    metadata1.m_indexes.push_back(ti);
    ti.m_index_num = "01";
    ti.m_index_frames=89;
    metadata1.m_indexes.push_back(ti);

	QVariant during = metadata1.toVariant();
	TrackMetadata metadata2;
    metadata2.fromVariant(during);

//	EXPECT_EQ(metadata1, metadata2);
    EXPECT_EQ(metadata1.m_track_number, metadata2.m_track_number);
    EXPECT_EQ(metadata1.m_start_frames, metadata2.m_start_frames);
    EXPECT_EQ(metadata1.m_indexes[0].m_index_num, metadata2.m_indexes[0].m_index_num);
	EXPECT_EQ(metadata1.m_indexes[1].m_index_num, metadata2.m_indexes[1].m_index_num);

}

TEST_F(SerializationTests, AMLMTagMapRT)
{
	AMLMTagMap tm1, tm2;

    tm1.insert(std::pair<std::string, std::string>("hello", "hello again"));

	QVariant during = tm1.toVariant();
	tm2.fromVariant(during);

	EXPECT_EQ(tm2.find("hello")->second, std::string("hello again"));
    EXPECT_EQ(*tm1.find("hello"), *tm2.find("hello"));
}

TEST_F(SerializationTests, MetadataThroughQVariant)
{
	Metadata md1, md2;

	md1.m_audio_file_url = "file:///a.bc.com";
	md1.m_audio_file_type = AudioFileType::MP3;
	md1.m_sample_rate = 44100;
	md1.m_num_channels = 2;

	QVariant during = md1.toVariant();
	md2.fromVariant(during);

	EXPECT_EQ(md2.m_audio_file_url, QUrl("file:///a.bc.com"));
	EXPECT_EQ(md1.m_audio_file_url, md2.m_audio_file_url);
	EXPECT_NE(md1.m_audio_file_type, md2.m_audio_file_type);
	EXPECT_NE(md1.m_sample_rate, md2.m_sample_rate);
	EXPECT_NE(md1.m_num_channels, md2.m_num_channels);

}

TEST_F(SerializationTests, ExtUrlRoundTripThroughQVariant)
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


TEST_F(SerializationTests, QUrlRoundTripThroughQVariant)
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

