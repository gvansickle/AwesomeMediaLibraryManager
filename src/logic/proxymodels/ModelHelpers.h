/*
 * Copyright 2017, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

// Qt5
#include <QPersistentModelIndex>
#include <QModelIndexList>
#include <QItemSelection>
#include <QAbstractProxyModel>
#include <QDebug>

// Ours
#include <utils/DebugHelpers.h>
#include <logic/proxymodels/QPersistentModelIndexVec.h>
#include <utils/ConnectHelpers.h>

/**
 * Map a QItemSelection to a top-level source selection via QAbstractProxyModel::mapSelectionToSource().
 * A no-op if selection is empty or the model isn't a proxy model.
 */
QItemSelection mapQItemSelectionToSource(const QItemSelection& proxy_selection);

/**
 * Converts a QItemSelection to a QVector of the equivalent QPersistentModelIndex'es.
 * @param selection
 * @param col
 * @return
 */
QPersistentModelIndexVec pindexes(const QItemSelection& selection, int col = -1);

QModelIndex mapToSource(const QModelIndex& proxy_index);

template <template<class> class T>
T<QPersistentModelIndex> mapQPersistentModelIndexesToSource(const T<QPersistentModelIndex>& iterable_of_pindexes)
{
	T<QPersistentModelIndex> retval;
	for(auto i : iterable_of_pindexes)
	{
		retval.push_back(mapToSource(i));
	}
	return retval;
}

QModelIndexList mapToSource(const QModelIndexList& source_indices);

QAbstractItemModel* getRootModel(QAbstractItemModel* maybe_proxy_model);

template <typename ModelType, class ViewType, class ContextType>
inline static void connect_jit_item_expansion(ModelType* model, ViewType* view, ContextType* context)
{
	// Hook up Just-In-Time item expansion.
	connect_or_die(model, &ModelType::rowsInserted,
				   context, [context, view](const QModelIndex& parent, int first, int last)
	{
		if(!view->isExpanded(parent))
		{
			view->expand(parent);
		}
	});
}

#endif /* MODELHELPERS_H */

