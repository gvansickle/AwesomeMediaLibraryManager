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


void ShuffleProxyModel::shuffle(bool shuffle)
{
	beginResetModel();
	m_indices.resize(sourceModel()->rowCount());
	std::iota(m_indices.begin(), m_indices.end(), 0);
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
	return createIndex(m_indices[sourceIndex.row()], sourceIndex.column());
}

QModelIndex ShuffleProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
	if (!proxyIndex.isValid())
	{
		return {};
	}

	return sourceModel()->index(m_indices[proxyIndex.row()], proxyIndex.column());
}
