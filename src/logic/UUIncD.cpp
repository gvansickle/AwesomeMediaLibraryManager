/*
 * Copyright 2019, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "UUIncD.h"


std::atomic_uint64_t UUIncD::m_next_id {1};

UUIncD::UUIncD(std::uint64_t id)
{
	m_my_id = id;
}

// static
UUIncD::UUIncD(quintptr qmodelindex_int_id)
{
	static_assert(sizeof(quintptr) == sizeof(m_my_id));
	m_my_id = qmodelindex_int_id;
}

// Static
UUIncD UUIncD::create()
{
	UUIncD retval;
	retval.m_my_id = UUIncD::m_next_id.fetch_add(1);
	return retval;
}

UUIncD::operator uint64_t() const
{
	return m_my_id;
}


