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
	qDebug() << "Setting source model to:" << sourceModel << "root model is:" << getRootModel(sourceModel);

	// Clear out the old index to show, it doesn't apply to the new model.
	m_current_selected_index = QPersistentModelIndex();

	BASE_CLASS::setSourceModel(sourceModel);
}

void EntryToMetadataTreeProxyModel::setSelectionModel(QItemSelectionModel* filter_selection_model)
{
	m_filter_selection_model = filter_selection_model;

	// Connect up the signals and slots.
	connect(m_filter_selection_model, &QItemSelectionModel::selectionChanged, this, &EntryToMetadataTreeProxyModel::onSelectionChanged);
	connect(m_filter_selection_model, &QItemSelectionModel::modelChanged, this, &EntryToMetadataTreeProxyModel::onModelChanged);
}

QItemSelectionModel* EntryToMetadataTreeProxyModel::selectionModel() const
{
	return m_filter_selection_model;
}

void EntryToMetadataTreeProxyModel::setSourceIndexToShow(const QPersistentModelIndex& source_index_to_filter_on)
{
	qDebug() << "Setting selected index to:" << source_index_to_filter_on;

	qDebug() << "Source model:" << sourceModel() << "index model:" << source_index_to_filter_on.model();

//	Q_ASSERT(sourceModel() == source_index_to_filter_on.model());

	/// @todo Maybe another way to notify listeners of this change?
	beginResetModel();

	m_current_selected_index = source_index_to_filter_on;

	auto sm = sourceModel();
	auto topleft = sm->index(0, 0, QModelIndex());
	auto bottomright = sm->index(sm->rowCount(), sm->columnCount(), QModelIndex());

//	emit dataChanged(topleft, bottomright);

	endResetModel();
}

bool EntryToMetadataTreeProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	// Only accept the row if it's currently selected.
	//QModelIndex index = ::mapToSource(sourceModel()->index(sourceRow, 0, sourceParent));
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

	if(!m_current_selected_index.isValid())
	{
		qDebug() << "selected index invalid" << m_current_selected_index;
	}

	if(m_current_selected_index == index)
	{
		qDebug() << "Accepting selected index:" << index;
		qDebug() << "index model:" << index.model();
		return true;
	}

	return false;
}

void EntryToMetadataTreeProxyModel::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	// Only pick the first selected index for now.
	auto source_rows = m_filter_selection_model->selectedRows();

	if(source_rows.isEmpty())
	{
		qWarning() << "Selection is empty";
	}
	else
	{
		setSourceIndexToShow(QPersistentModelIndex(source_rows[0]));
	}
}

void EntryToMetadataTreeProxyModel::onModelChanged(QAbstractItemModel* model)
{
///@todo
}
