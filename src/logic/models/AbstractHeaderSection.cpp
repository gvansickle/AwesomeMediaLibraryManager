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
 * @file AbstractHeaderSection.cpp
 */
#include "AbstractHeaderSection.h"

#include <utils/DebugHelpers.h>

AbstractHeaderSection::AbstractHeaderSection()
{

}

AbstractHeaderSection::~AbstractHeaderSection()
{
}


///
/// BasicHeaderSection implementation.
///

BasicHeaderSection::BasicHeaderSection(int section, Qt::Orientation orientation, const QVariant& value, int role)
	: m_section(section), m_orientation(orientation)
{
	m_role_to_value_map.insert_or_assign(role, value);
}

QVariant BasicHeaderSection::headerData(int section, Qt::Orientation orientation, int role)
{
	if((section == m_section) && (orientation == m_orientation))
	{
		// Section/orientation is right, see if we have any role data to return.
		return lookup_role(role);
	}
	else
	{
		qCr() << "Invalid section or orientation";
	    return QVariant();
	}
}

bool BasicHeaderSection::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
{
	m_section = section;
	m_orientation = orientation;
	auto retval = m_role_to_value_map.insert_or_assign(role, value);

	return (retval.first != m_role_to_value_map.end());
}

QVariant BasicHeaderSection::lookup_role(int role) const
{
	auto retval_it = m_role_to_value_map.find(role);
	if(retval_it == m_role_to_value_map.end())
	{
		return QVariant();
	}
	return retval_it->second;
}
