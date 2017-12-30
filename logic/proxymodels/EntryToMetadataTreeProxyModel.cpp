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

#include "EntryToMetadataTreeProxyModel.h"

#include <QDebug>

EntryToMetadataTreeProxyModel::EntryToMetadataTreeProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

EntryToMetadataTreeProxyModel::~EntryToMetadataTreeProxyModel()
{
}

void EntryToMetadataTreeProxyModel::setSelectedIndex(const QPersistentModelIndex& selected_index)
{
	qDebug() << "Setting selected index to:" << selected_index;

	m_current_selected_index = selected_index;
}

bool EntryToMetadataTreeProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	// Only accept the row if it's selected.
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

	if(m_current_selected_index == index)
	{
		qDebug() << "Accepting selected index:" << index;
		return true;
	}

	return false;
}
