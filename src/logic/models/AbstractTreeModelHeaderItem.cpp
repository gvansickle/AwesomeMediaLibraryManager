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
 * @file AbstractTreeModelHeaderItem.cpp
 */
#include "AbstractTreeModelHeaderItem.h"

// Ours
#include <logic/xml/XmlObjects.h>

AbstractTreeModelHeaderItem::AbstractTreeModelHeaderItem(QVector<QVariant> x, AbstractTreeModelItem *parent)
	: AbstractTreeModelItem (x, parent)
{
#warning "TODO This should take a list of AbsHeaderSections"
//	m_item_data = x;
}

AbstractTreeModelHeaderItem::~AbstractTreeModelHeaderItem()
{
}

bool AbstractTreeModelHeaderItem::writeItemAndChildren(QXmlStreamWriter* writer) const
{
	// Convenience ref.
	auto& xml = *writer;

	XmlElement e("abstract_tree_model_header", {}, {},
	{XmlElement("test", 1)},
				 [=](XmlElement* e, QXmlStreamWriter* xml){
		for(int i = 0; i < childCount(); ++i)
		{
			const auto* child = this->child(i);
			child->writeItemAndChildren(xml);
		}
	});

	e.write(writer);
#warning "TODO"
	return false;
}
