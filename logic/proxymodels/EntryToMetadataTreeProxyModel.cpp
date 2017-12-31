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

#include <utils/ModelHelpers.h>

EntryToMetadataTreeProxyModel::EntryToMetadataTreeProxyModel(QObject *parent) : BASE_CLASS(parent)
{
	setDynamicSortFilter(true);
}

EntryToMetadataTreeProxyModel::~EntryToMetadataTreeProxyModel()
{
}

void EntryToMetadataTreeProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
	qDebug() << "Setting source model to:" << sourceModel;

	// Clear out the old index to show, it doesn't apply to the new model.
	m_current_selected_index = QPersistentModelIndex();

	BASE_CLASS::setSourceModel(sourceModel);
}

void EntryToMetadataTreeProxyModel::setSourceIndexToShow(const QPersistentModelIndex& source_index_to_filter_on)
{
	qDebug() << "Setting selected index to:" << source_index_to_filter_on;

	qDebug() << "Source root model:" << sourceModel() << "index model:" << source_index_to_filter_on.model();

//	Q_ASSERT(sourceModel() == source_index_to_filter_on.model());

	/// @todo Maybe another way to notify listeners of this change?
	beginResetModel();

	m_current_selected_index = source_index_to_filter_on;

	endResetModel();
}

bool EntryToMetadataTreeProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	// Only accept the row if it's currently selected.
	QModelIndex index = ::mapToSource(sourceModel()->index(sourceRow, 0, sourceParent));

//	qDebug() << "index model:" << index.model();
//	qDebug() << "root model:" << ::mapToSource(index);

	if(m_current_selected_index == index)
	{
		qDebug() << "Accepting selected index:" << index;
		return true;
	}

	return false;
}
