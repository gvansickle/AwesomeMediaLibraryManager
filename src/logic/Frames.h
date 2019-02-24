/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
 * @file Frames.h
 */
#ifndef SRC_LOGIC_FRAMES_H_
#define SRC_LOGIC_FRAMES_H_

// Std C++.
#include <cstdint>

// Boost.
#include <boost/operators.hpp>
#if 0
/**
 * Arithmetic type for units of CD Cue Sheet Frames == 1/75th of a second.
 */
class Frames
		: boost::totally_ordered< Frames
		, boost::addable< Frames
		> > // That was base-class chaining.

{
public:
	/// Rule of Zero.

	explicit Frames(int64_t value) { m_frames = value; }

	/**
	 * Three-way comparison operator.
	 * @return
	 */
	//friend int compare(Frames& f1, Frames& f2) noexcept;
	bool operator<(const Frames& other) const { return m_frames < other.m_frames; };
	bool operator==(const Frames& other) const { return m_frames == other.m_frames; };

	Frames operator+=(const Frames& other)
	{
		return Frames(m_frames + other.m_frames);
	}

private:

	int64_t m_frames;
};
#endif
#endif /* SRC_LOGIC_FRAMES_H_ */
