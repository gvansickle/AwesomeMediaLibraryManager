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

#include "DebugSequence.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <atomic>

DebugSequence::DebugSequence()
{
	// TODO Auto-generated constructor stub

}

DebugSequence::~DebugSequence()
{
	// TODO Auto-generated destructor stub
}

void DebugSequence::reset(int reset)
{
	m_expected_state = reset;
}

bool DebugSequence::expect_and_set(int expect, int set)
{
	int was = m_expected_state.exchange(set);
    if (was != expect)
	{
		throw std::runtime_error("DebugSequence::expect_and_set: expected " + std::to_string(expect) + " but was " + std::to_string(was));
	}
	return was == expect;
}

