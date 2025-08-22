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

#include "string_ops_tests.h"

// Ours
#include <future/string_ops.h>
#include "TestHelpers.h"


TEST_F(StringOpsTests, trim_quotes1)
{
	std::string s = "\"hello\"";
	auto result = trim_quotes(s);
	EXPECT_STREQ(std::string(result).c_str(), "hello");

	std::string s2 = "\"hello";
	auto result2 = trim_quotes(s2);
	EXPECT_STREQ(std::string(result2).c_str(), s2.c_str());

	std::string s3 = "hello\"";
	auto result3 = trim_quotes(s3);
	EXPECT_STREQ(std::string(result3).c_str(), s3.c_str());
}

TEST_F(StringOpsTests, trim_quotes2)
{
	std::string s = "hello";
	auto result = trim_quotes(s);
	EXPECT_STREQ(std::string(result).c_str(), s.c_str());
}
