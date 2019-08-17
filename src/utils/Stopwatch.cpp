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

// Ours
#include <utils/DebugHelpers.h>

#define AMLMCOUT qDb()

//// ScopedLap

ScopedLap::ScopedLap(const std::string& lap_desc, const std::shared_ptr<Stopwatch>& psw)
	: m_lap_description(lap_desc), m_parent_stopwatch(psw)
{
	m_lap_start = std::chrono::steady_clock::now();
}

ScopedLap::~ScopedLap()
{
	// Add this lap to the Stopwatch object.
//	if(auto psw = m_parent_stopwatch.lock())
//	{
//		m_lap_end = std::chrono::steady_clock::now();
//		psw->addCompletedScopedLap(*this);
//	}
}

//// Stopwatch

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
	AMLMCOUT << "START: " << m_being_timed_msg; // << std::endl;
}

void Stopwatch::lap(const std::string& lap_marker_str)
{
	std::scoped_lock sl(m_mutex);

	lap_marker lm;
	lm.m_lap_time = std::chrono::steady_clock::now();
	lm.m_lap_description = lap_marker_str;
	m_lap_markers.push_back(lm);

	std::chrono::duration<double> elapsed = lm.m_lap_time - m_start;

	AMLMCOUT << "ELAPSED TIME, LAP:" << lm.m_lap_description << ": " << elapsed.count() << " sec"; // << std::endl;
}

//ScopedLap Stopwatch::scoped_lap(const std::string& lap_marker_str)
//{
//	std::scoped_lock sl(m_mutex);
//
//	ScopedLap retval(lap_marker_str, this->shared_from_this());
//
//	return retval;
//}

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

	AMLMCOUT << "END: " << m_being_timed_msg; // << std::endl;
	AMLMCOUT << "TOTAL ELAPSED TIME: " << m_being_timed_msg << ": " << elapsed.count() << " sec"; // << std::endl;
	if(!m_lap_markers.empty())
	{
		AMLMCOUT << "LAP MARKERS:\n";
		int lap = 0;
		for(const auto& lm : m_lap_markers)
		{
			std::chrono::duration<double> elapsed = lm.m_lap_time - m_start;
			AMLMCOUT << "LAP " << lap << " TIME: " << elapsed.count() << " DESC: " << lm.m_lap_description;// << "\n";
			lap++;
		}
	}
//	if(!m_scoped_lap_markers.empty())
//	{
//		AMLMCOUT << "SCOPED LAP MARKERS:\n";
//		int lap = 0;
//		for(const auto& slm : m_scoped_lap_markers)
//		{
//			std::chrono::duration<double> elapsed = slm.m_lap_end - slm.m_lap_start;
//			std::chrono::duration<double> start = slm.m_lap_start.time_since_epoch();
//			std::chrono::duration<double> end = slm.m_lap_end.time_since_epoch();
//			AMLMCOUT << "SCOPED LAP " << lap << "START:" << start.count() << "END:" << end.count() << "ELAPSED:" << elapsed.count() << " DESC: " << slm.m_lap_description;// << "\n";
//			lap++;
//		}
//	}
}

void Stopwatch::TSI_reset()
{
	m_start = decltype(m_start)::min();
	m_end = decltype(m_end)::max();
	m_lap_markers.clear();
//	m_scoped_lap_markers.clear();
	m_being_timed_msg.clear();
}

//void Stopwatch::addCompletedScopedLap(const ScopedLap& sl)
//{
//	m_scoped_lap_markers.emplace_back(sl);
//}


