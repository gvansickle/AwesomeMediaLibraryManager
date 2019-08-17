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
 * @file Stopwatch.h
 */
#ifndef SRC_UTILS_STOPWATCH_H_
#define SRC_UTILS_STOPWATCH_H_

// Std C++
#include <chrono>
#include <string>
#include <vector>
#include <mutex>

class ScopedLap
{
public:
};

/**
 * Threadsafe, scoped Elapsed time timer, mostly for debug purposes.
 */
class Stopwatch
{
public:
	explicit Stopwatch();
	explicit Stopwatch(const std::string& being_timed_msg);
	virtual ~Stopwatch();

	void start(const std::string& being_timed_msg);

	void lap(const std::string& lap_marker_str);

	std::shared_ptr<ScopedLap> scoped_lap(const std::string& lap_marker_str);

	void stop();
	/// Threadsafe Interface pattern, public interface to the internal reset functionality in TSI_reset().
	void reset();

	void print_results();

private:
	/// Threadsafe Interface pattern, internal reset.
	void TSI_reset();


	struct lap_marker
	{
		std::chrono::steady_clock::time_point m_lap_time;
		std::string m_lap_discription;
	};

	std::mutex m_mutex;

	std::chrono::steady_clock::time_point m_start;
	std::chrono::steady_clock::time_point m_end;
	std::string m_being_timed_msg;
	std::vector<lap_marker> m_lap_markers;
};

#endif /* SRC_UTILS_STOPWATCH_H_ */
