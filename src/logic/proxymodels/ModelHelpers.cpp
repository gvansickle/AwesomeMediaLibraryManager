/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "ModelHelpers.h"


QItemSelection mapQItemSelectionToSource(const QItemSelection& proxy_selection)
{
	if(proxy_selection.size() > 0)
	{
		// There's a selection to convert.  Get the first QModelIndex so we can get its model.
		auto model = proxy_selection[0].model();
		if(model)
		{
			// Is it a proxy model?
			auto proxy_model = qobject_cast<const QAbstractProxyModel*>(model);
			if(proxy_model)
			{
				return proxy_model->mapSelectionToSource(proxy_selection);
			}
		}
	}

	return proxy_selection;
}

QPersistentModelIndexVec pindexes(const QItemSelection& selection, int col)
{
	QModelIndexList index_vec = selection.indexes();
	QModelIndexList retval;
	if(col != -1)
	{
		for(auto i : std::as_const(index_vec))
		{
			if(i.isValid())
			{
				if(i.column() == col)
				{
					retval.push_back(i);
				}
			}
		}
		return toQPersistentModelIndexVec(retval);
	}
	else
	{
		return toQPersistentModelIndexVec(index_vec);
	}
}

QModelIndex mapToSource(const QModelIndex& proxy_index)
{
	if(proxy_index.model())
	{
		// There's a model.  See if it's a proxy model.
		auto proxy_model = qobject_cast<const QAbstractProxyModel*>(proxy_index.model());
		if(proxy_model)
		{
			return proxy_model->mapToSource(proxy_index);
		}
	}

	return proxy_index;
}

QModelIndexList mapToSource(const QModelIndexList& source_indices)
{
	QModelIndexList retval;

	for(auto i : source_indices)
	{
		retval.push_back(mapToSource(i));
	}

	return retval;
}



QAbstractItemModel* getRootModel(QAbstractItemModel* maybe_proxy_model)
{
	auto proxy_model = qobject_cast<QAbstractProxyModel*>(maybe_proxy_model);

	if(proxy_model)
	{
		qDebug() << "Is a proxy model:" << proxy_model;
		auto source_model = proxy_model->sourceModel();
		if(source_model)
		{
			qDebug() << "With source model:" << source_model;
			return (QAbstractItemModel*)source_model;
		}
		else
		{
			return maybe_proxy_model;
		}
	}
	else
	{
		// Wasn't a proxy model.
		qDebug() << "Not a proxy model:" << maybe_proxy_model;
		return maybe_proxy_model;
	}
}




