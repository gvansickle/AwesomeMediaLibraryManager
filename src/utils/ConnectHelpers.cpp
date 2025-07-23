/*
 * Copyright 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/// @file ConnectHelpers.cpp

#include "ConnectHelpers.h"

// Qt
#include <QObject>
#include <QMetaObject>


Disconnector::Disconnector()
{
}

Disconnector::~Disconnector()
{
}

void Disconnector::addConnection(QMetaObject::Connection connection)
{
	Q_ASSERT(true == static_cast<bool>(connection));
	m_connections.push_back(connection);
}

Disconnector& Disconnector::operator<<(QMetaObject::Connection connection)
{
	addConnection(connection);
    return *this;
}

void Disconnector::disconnect()
{
	for(auto& connection : m_connections)
    {
    	bool status = QObject::disconnect(connection);
        Q_ASSERT(status == true);
    }
	m_connections.clear();
}
