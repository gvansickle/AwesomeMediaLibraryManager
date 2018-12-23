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
 * @file Stopwatch.cpp
 */

#include "Stopwatch.h"

// Std C++
#include <iostream>


Stopwatch::Stopwatch(const std::string& being_timed_msg) : m_being_timed_msg(being_timed_msg)
{
	m_start = std::chrono::steady_clock::now();
	std::cout << "START: " << m_being_timed_msg << std::endl;
}

Stopwatch::~Stopwatch()
{
	auto end = std::chrono::steady_clock::now();

	std::chrono::duration<double> elapsed = end - m_start;

	std::cout << "END: " << m_being_timed_msg << std::endl;
	std::cout << "ELAPSED TIME: " << m_being_timed_msg << ": " << elapsed.count() << " sec" << std::endl;
}

