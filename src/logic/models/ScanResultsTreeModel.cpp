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
#include "ScanResultsTreeModel.h"

// Ours
#include "ScanResultsTreeModelItem.h"
#include "AbstractTreeModelHeaderItem.h"

ScanResultsTreeModel::ScanResultsTreeModel(const QStringList &headers, const QString &data, QObject *parent)
    : BASE_CLASS(headers, data, parent)
{
	/// @todo Move all this out of the constructor?
	QVector<QVariant> rootData;
	for(const QString& header : headers)
	{
		rootData << header;
	}

//	m_root_item = new AbstractTreeModelItem(rootData);
	/// @todo virtual function in constructor.
	m_root_item = make_root_node(rootData);

	// Populate the parse factory functions with the first-layer node type names we know about.
	m_parse_factory_functions.emplace_back(&ScanResultsTreeModelItem::parse);
}

bool ScanResultsTreeModel::appendItems(QVector<AbstractTreeModelItem*> new_items, const QModelIndex& parent)
{
	return BASE_CLASS::appendItems(new_items, parent);
}

AbstractTreeModelHeaderItem* ScanResultsTreeModel::make_root_node(QVector<QVariant> rootData)
{
	return new AbstractTreeModelHeaderItem(rootData);
}
