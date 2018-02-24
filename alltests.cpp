/*
 * alltests.cpp
 *
 *  Created on: Feb 23, 2018
 *      Author: gary
 */

//#include <QObject>
//#include <QtTest>

//#include <utils/concurrency/tests/AsyncTests.h>

#include <gtest/gtest.h>

////
//#include <gmock/gmock-matchers.h>
//
//using namespace testing;
//
//TEST(TestCase1, TestSet1)
//{
//	EXPECT_EQ(1, 1);
//	ASSERT_THAT(0, Eq(0));
//}
////

int main(int argc, char *argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

