/*
 * Copyright 2017, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef MODELHELPERS_H
#define MODELHELPERS_H

/**
 * @file
 */

// Qt
#include <QPersistentModelIndex>
#include <QModelIndexList>
#include <QItemSelection>
#include <QAbstractProxyModel>
#include <QDebug>

// Ours
#include <utils/DebugHelpers.h>
#include <logic/proxymodels/QPersistentModelIndexVec.h>

/**
 * Map a QItemSelection to a top-level source selection via QAbstractProxyModel::mapSelectionToSource().
 * A no-op if selection is empty or the model isn't a proxy model.
 */
inline static QItemSelection mapQItemSelectionToSource(const QItemSelection& proxy_selection)
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

/**
 * Converts a QItemSelection to a QVector of the equivalent QPersistentModelIndex'es.
 * @param selection
 * @param col
 * @return
 */
inline static QPersistentModelIndexVec pindexes(const QItemSelection& selection, int col = -1)
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

/**
 * Maps a proxy QModelIndex through all intermediate proxies and returns the model's corresponding QModelIndex.
 * @param proxy_index The QModelIndex to an item in the proxy.
 * @return The QModelIndex in the bottom-most model corresponding to the @a proxy_index.
 */
QModelIndex mapToSourceRecursive(const QModelIndex& proxy_index);

/**
 * Applies mapToSourceRecursive(const QModelIndex&) to each index in @a source_indices.
 * @see mapToSourceRecursive(const QModelIndex&)
 * @param source_indices The list of QModelIndex's to map to source.
 * @return The mapped list.
 */
QModelIndexList mapToSourceRecursive(const QModelIndexList& source_indices);

/**
 * 
 * @param proxy_index
 * @param top_proxy
 * @return
 */
QModelIndex mapFromSourceRecursive(const QAbstractItemModel* top_proxy, const QModelIndex& source_model_index);


template <template<class> class T>
T<QPersistentModelIndex> mapQPersistentModelIndexesToSource(const T<QPersistentModelIndex>& iterable_of_pindexes)
{
	T<QPersistentModelIndex> retval;
	for(auto i : iterable_of_pindexes)
	{
        retval.push_back(mapToSourceRecursive(i));
	}
	return retval;
}

/**
 * Returns the bottom-most model in a model<-proxy<-proxy...<-proxy chain.
 * @param maybe_proxy_model
 * @return
 */
QAbstractItemModel* getRootModel(QAbstractItemModel* maybe_proxy_model);

#endif /* MODELHELPERS_H */

