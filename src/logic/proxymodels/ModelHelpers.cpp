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
/// @file

#include "ModelHelpers.h"


QModelIndex mapToSourceRecursive(const QModelIndex& proxy_index)
{
	if (!proxy_index.isValid())
	{
		return QModelIndex();
	}

	if (proxy_index.model())
	{
		// There's an underlying model.  See if it's a proxy model.
		auto proxy_model = qobject_cast<const QAbstractProxyModel*>(proxy_index.model());
		if (proxy_model)
		{
			// recurse into the next proxy model.
			return mapToSourceRecursive(proxy_model->mapToSource(proxy_index));
		}
		// We've hit the true source model.
		return proxy_index;
	}

	// No model, return invalid index.
	return QModelIndex();
}

QModelIndex mapFromSourceRecursive(const QAbstractItemModel* top_proxy, const QModelIndex& source_model_index)
{
	if (!top_proxy || !source_model_index.isValid())
	{
		return QModelIndex();
	}

	auto proxy_model = qobject_cast<const QAbstractProxyModel*>(top_proxy);
	if (proxy_model)
	{
		// Recurse through all proxy model layers.
		QModelIndex next_source_model_index = mapFromSourceRecursive(proxy_model->sourceModel(), source_model_index);
		return proxy_model->mapFromSource(next_source_model_index);
	}

	// We've hit the actual source model.
	return source_model_index;
}

QModelIndexList mapToSourceRecursive(const QModelIndexList& source_indices)
{
	QModelIndexList retval;

	for (const auto& i : source_indices)
	{
		retval.push_back(mapToSourceRecursive(i));
	}

	return retval;
}

QAbstractItemModel* getRootModel(QAbstractItemModel* maybe_proxy_model)
{
	auto proxy_model = qobject_cast<QAbstractProxyModel*>(maybe_proxy_model);

	if (proxy_model && proxy_model->sourceModel())
	{
		// It's a proxymodel with a source model, so recurse on the source model.
		auto source_model = proxy_model->sourceModel();
		return getRootModel(source_model);
	}
	else
	{
		// Wasn't a proxy model.
		return maybe_proxy_model;
	}
}
