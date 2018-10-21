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
 * @file ScanResultsTreeModelItem.cpp
 */

#include "ScanResultsTreeModelItem.h"

#include <QXmlStreamReader>


ScanResultsTreeModelItem::ScanResultsTreeModelItem(QVector<QVariant> x, AbstractTreeModelItem *parent)
	: AbstractTreeModelItem(x, parent)
{

}

ScanResultsTreeModelItem::~ScanResultsTreeModelItem()
{
	// TODO Auto-generated destructor stub
}

ScanResultsTreeModelItem* ScanResultsTreeModelItem::parse(QXmlStreamReader* xmlp, AbstractTreeModelItem* parent)
{
	auto& xml = *xmlp;

	if(xml.name() != "srtmitem")
	{
		return nullptr;
	}
	else
	{
		// Read in this and all children.
		auto* this_node = createChildItem(parent);

		while(xml.readNextStartElement())
		{
			if(xml.name() == "")
		}
	}
}

bool ScanResultsTreeModelItem::writeItemAndChildren(QXmlStreamWriter* writer) const
{
	// Convenience ref.
	auto& xml = *writer;

	/// @todo Check if we're the root item?

	// Write out this item.

	QString item_tag_name = "srtmitem";
	xml.writeStartElement(item_tag_name);
	xml.writeAttribute("childNumber", QString("%1").arg(childNumber()));

	// Write the columns of data.
	for(int col = 0; col < columnCount(); ++col)
	{
		/// @todo Get header element info.
//		xml.writeAttribute("childNumber", QString("%1").arg(item->childNumber()));
		xml.writeTextElement("column_data", data(col).toString());
	}

	// Write out all children.
	for(int i = 0; i < childCount(); ++i)
	{
		// Hold on tight, we're going recursive!
		child(i)->writeItemAndChildren(writer);
	}
	xml.writeEndElement();

	/// @todo Default to something else if not overridden?
	return true;
}

