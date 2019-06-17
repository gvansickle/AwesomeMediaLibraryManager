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
 * @file Core.cpp
 */
#include <Core.h>

// Std C++
#include <memory>
#include "AMLMApp.h"

namespace AMLM
{

std::unique_ptr<Core> Core::m_self;

/// Private constructor.
Core::Core()
{
}

//void Core::prepareShutdown()
//{
//	m_guiConstructed = false;
//}

Core::~Core()
{
}

void Core::build()
{
	if(m_self)
	{
		return;
	}
	m_self.reset(new Core());

	// Create the single (at this point) ScanResultsTreeModel.
	/// @note In KDenLive, this is the same, no parent QObject given to ProjectItemModel::construct();
	m_self->m_srtm_instance = ScanResultsTreeModel::construct();
	// Create and set the root item / headers
	M_TODO("Needs to be ColumnSpecs");
	m_self->m_srtm_instance->setColumnSpecs({"DirProps", "MediaURL", "SidecarCueURL"});
	// Let's add two more columns
	m_self->m_srtm_instance->insertColumns(3, 2);
}

void Core::initGUI()
{
	/// @todo This is a KdenLive member.  Sets up and shows the GUI.  Not sure we need it, we have that going on in AMLMApp.
}

std::unique_ptr<Core>& Core::self()
{
	if (!m_self)
	{
		qDb() << "Error : Core has not been created";
	}
	return m_self;
}

std::shared_ptr<ScanResultsTreeModel> Core::getScanResultsTreeModel()
{
	return m_srtm_instance;
};

void Core::clean()
{
	m_self.reset();
}

} /* namespace AMLM */
