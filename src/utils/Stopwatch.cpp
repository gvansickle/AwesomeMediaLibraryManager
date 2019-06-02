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
	if(!m_lap_markers.empty())
	{
		std::cout << "LAP MARKERS:\n";
		int lap = 0;
		for(const auto& lm : m_lap_markers)
		{
			std::cout << "LAP " << lap << " DESC: " << lm.m_lap_discription << "\n";
			std::chrono::duration<double> elapsed = lm.m_lap_time - m_start;
			std::cout << "LAP " << lap << " TIME: " << elapsed.count() << "\n";
		}
	}
}

void Stopwatch::lap(const std::string& lap_marker_str)
{
	lap_marker lm;
	lm.m_lap_time = std::chrono::steady_clock::now();
	lm.m_lap_discription = lap_marker_str;
	m_lap_markers.push_back(lm);

	std::chrono::duration<double> elapsed = lm.m_lap_time - m_start;

	std::cout << "ELAPSED TIME, LAP:" << lm.m_lap_discription << ": " << elapsed.count() << " sec" << std::endl;
}

