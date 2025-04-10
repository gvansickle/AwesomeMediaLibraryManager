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

#include "ShuffleProxyModel.h"

// Std C++
#include <numeric>
#include <algorithm>
#include <random>

// Ours
#include <ConnectHelpers.h>


ShuffleProxyModel::ShuffleProxyModel(QObject* parent): QSortFilterProxyModel(parent)
{

}

void ShuffleProxyModel::shuffle(bool shuffle)
{
	beginResetModel();

	m_shuffle = shuffle;
	m_indices.resize(sourceModel()->rowCount());
	std::ranges::iota(m_indices, 0);
	if (shuffle)
	{
		std::ranges::shuffle(m_indices, std::mt19937(std::random_device{}()));
	}

	endResetModel();
}

QModelIndex ShuffleProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
	if (!sourceIndex.isValid())
    {
        return {};
    }
	if (m_shuffle_model_rows)
	{
		return createIndex(m_indices[sourceIndex.row()], sourceIndex.column());
	}
	else
	{
		return sourceIndex;
	}
}

QModelIndex ShuffleProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
	if (!proxyIndex.isValid())
	{
		return {};
	}

	if (m_shuffle_model_rows)
	{
		return sourceModel()->index(m_indices[proxyIndex.row()], proxyIndex.column());
	}
	else
	{
		return proxyIndex;
	}
}

void ShuffleProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
	beginResetModel();

	m_disconnector.disconnect();
	QSortFilterProxyModel::setSourceModel(sourceModel);
	connectToModel(sourceModel);

	onNumRowsChanged();

	endResetModel();
}

void ShuffleProxyModel::onNumRowsChanged()
{
	// Resize the shuffle map.
	auto num_rows = sourceModel()->rowCount();
	m_indices.resize(num_rows);
	std::iota(m_indices.begin(), m_indices.end(), 0);
	if (m_shuffle)
	{
		std::ranges::shuffle(m_indices, std::mt19937(std::random_device{}()));
	}
}

void ShuffleProxyModel::connectToModel(QAbstractItemModel* model)
{
	if (model == nullptr)
	{
		// Disconnect from the model.
		m_disconnector.disconnect();
	}
	else
	{
		m_disconnector
		<< connect_or_die(model, &QAbstractItemModel::modelReset, this, &ShuffleProxyModel::onNumRowsChanged)
		<< connect_or_die(model, &QAbstractItemModel::rowsInserted, this, &ShuffleProxyModel::onNumRowsChanged)
		<< connect_or_die(model, &QAbstractItemModel::rowsRemoved, this, &ShuffleProxyModel::onNumRowsChanged);
	}
}
