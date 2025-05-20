/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "LibrarySortFilterProxyModel.h"

#include <logic/LibraryEntry.h>
#include <logic/models/LibraryModel.h>

LibrarySortFilterProxyModel::LibrarySortFilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent)
{
	// Filter all columns by default.
	setFilterKeyColumn(-1);
}

std::shared_ptr<LibraryEntry> LibrarySortFilterProxyModel::getItem(QModelIndex index) const
{
	// Need to map to the source model index.
	auto source_index = mapToSource(index);
	LibraryModel* libmodel = qobject_cast<LibraryModel*>(sourceModel());
	return libmodel->getItem(source_index);
}

bool LibrarySortFilterProxyModel::hasChildren(const QModelIndex& parent) const
{
    ///qDebug() << "hasChildren(): " << parent;
	LibraryModel* libmodel = qobject_cast<LibraryModel*>(sourceModel());
	return libmodel->hasChildren(mapToSource(parent));
}

bool LibrarySortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	// Apply any filter to all columns.
	bool retval = false;
	for(auto c = 0; c < sourceModel()->columnCount(); c++)
	{
		QModelIndex index = sourceModel()->index(sourceRow, c, sourceParent);
		retval |= sourceModel()->data(index).toString().contains(filterRegularExpression());
		if (retval)
		{
			break;
		}
	}
	return retval;
}

bool LibrarySortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	LibraryModel* libmodel = qobject_cast<LibraryModel*>(sourceModel());
	auto section = libmodel->getSectionFromCol(left.column());
	if(section != SectionID::Length)
	{
		// Not the column we need to treat differently, call the base class.
		return QSortFilterProxyModel::lessThan(left, right);
	}
	else
	{
		QVariant leftData = sourceModel()->data(left);
		QVariant rightData = sourceModel()->data(right);
		return leftData.value<Fraction>() < rightData.value<Fraction>();
	}
}
