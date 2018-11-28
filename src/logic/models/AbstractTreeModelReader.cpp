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
 * @file AbstractTreeModelReader.cpp
 */

#include "AbstractTreeModelReader.h"

// Qt5

// Ours
#include "AbstractTreeModel.h"
#include "AbstractTreeModelItem.h"
#include <utils/DebugHelpers.h>

AbstractTreeModelReader::AbstractTreeModelReader(AbstractTreeModel* model) : m_model(model)
{
}

AbstractTreeModelReader::~AbstractTreeModelReader()
{
}

bool AbstractTreeModelReader::read(QIODevice* device)
{
	auto& xml = m_xml_stream_reader;
	auto& model = m_model;

	xml.setDevice(device);

	// Get the first start element.
	if(xml.readNextStartElement())
	{
		// Check that we're reading an XML file with the right format.
		if(model->readModel(&xml))
		{
			// model read successfully.
			qIn() << "Read model successfully";
		}
		else
		{
			xml.raiseError(QObject::tr("Bad XML name or version elements."));
		}
	}
	return !xml.error();
}




