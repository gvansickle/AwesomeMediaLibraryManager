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


Stopwatch::Stopwatch()
{
	reset();
}

Stopwatch::Stopwatch(const std::string& being_timed_msg)
{
	start(being_timed_msg);
}

Stopwatch::~Stopwatch()
{
	stop();
	print_results();
}

void Stopwatch::start(const std::string& being_timed_msg)
{
	std::scoped_lock sl(m_mutex);

	TSI_reset();
	m_end = decltype(m_end)::min();
	m_being_timed_msg = being_timed_msg;
	m_start = std::chrono::steady_clock::now();
	std::cout << "START: " << m_being_timed_msg << std::endl;
}

void Stopwatch::lap(const std::string& lap_marker_str)
{
	std::scoped_lock sl(m_mutex);

	lap_marker lm;
	lm.m_lap_time = std::chrono::steady_clock::now();
	lm.m_lap_discription = lap_marker_str;
	m_lap_markers.push_back(lm);

	std::chrono::duration<double> elapsed = lm.m_lap_time - m_start;

	std::cout << "ELAPSED TIME, LAP:" << lm.m_lap_discription << ": " << elapsed.count() << " sec" << std::endl;
}

void Stopwatch::stop()
{
	std::scoped_lock sl(m_mutex);

	m_end = std::chrono::steady_clock::now();
}

void Stopwatch::reset()
{
	std::scoped_lock sl(m_mutex);

	TSI_reset();
}

void Stopwatch::print_results()
{
	std::scoped_lock sl(m_mutex);

	std::chrono::duration<double> elapsed{};
	if(m_end == decltype(m_end)::min())
	{
		// Don't set m_end here, this print could be during the event still being timed.
		elapsed = std::chrono::steady_clock::now() - m_start;
	}
	else
	{
		// m_end is set, so whatever was being timed is complete.  Use m_end.
		elapsed = m_end - m_start;
	}

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
			lap++;
		}
	}
}

void Stopwatch::TSI_reset()
{
	m_start = decltype(m_start)::min();
	m_end = decltype(m_end)::max();
	m_lap_markers.clear();
	m_being_timed_msg.clear();
}

