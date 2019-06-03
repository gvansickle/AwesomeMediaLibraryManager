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

#ifndef AWESOMEMEDIALIBRARYMANAGER_UUINCD_H
#define AWESOMEMEDIALIBRARYMANAGER_UUINCD_H

// Std C++
#include <atomic>
#include <cstdint>
//#include <utility> // For std::rel_ops;

// Ours
#include <future/guideline_helpers.h>

//using namespace std::rel_ops;

class UUIncD
{
public:
	M_GH_RULE_OF_FIVE_DEFAULT_C21(UUIncD);
	~UUIncD() = default;

	static UUIncD create();

	// User-defined conversion to uin64_t.
	operator uint64_t() const;

protected:
	explicit UUIncD(std::uint64_t id);

private:

	std::uint64_t m_my_id;

	bool operator==(const UUIncD& rhs) const { return m_my_id == rhs.m_my_id; };
	bool operator<(const UUIncD& rhs) const { return m_my_id < rhs.m_my_id; };

	/**
	 * The program-global threadsafe 64-bit ID which will next be doled out by create().
	 */
	static std::atomic_uint64_t m_next_id;
};

//inline static bool operator==(const UUIncD& lhs, const UUIncD& rhs)
//{
//	return lhs.m_my_id == rhs.m_my_id;
//}

//inline static bool operator<(const UUIncD& lhs, const UUIncD& rhs)
//{
//	return lhs.m_my_id < rhs.m_my_id;
//}

#endif //AWESOMEMEDIALIBRARYMANAGER_UUINCD_H