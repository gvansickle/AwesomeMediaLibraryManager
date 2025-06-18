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
/// @file

#include "AlgorithmTests.h"

#include "DebugHelpers.h"
#include "MapConverter.h"

TEST_F(AlgorithmTests, mapdiff1)
{
	std::multimap<int, int> m1 {{1,1},{1,2},{1,3}};
	std::multimap<int, int> m2 {{1,1},{1,2},{1,3},{1,4}, {1, 5}};

	auto diff = mapdiff(m1, m2);

	EXPECT_EQ(diff.value().m_in1not2.size(), 0);
	EXPECT_EQ(diff.value().m_in2not1.size(), 2);
	EXPECT_EQ(diff.value().m_in2not1[0].second, 4);
	EXPECT_EQ(diff.value().m_in2not1[1].second, 5);

	qDb() << diff.value();
}
