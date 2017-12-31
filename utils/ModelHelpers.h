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

#ifndef MODELHELPERS_H
#define MODELHELPERS_H

#include <QPersistentModelIndex>
#include <QModelIndexList>
#include <QItemSelection>
#include <QAbstractProxyModel>

/// Convert a QModelIndexList into a QList of QPersistentIndexes.
inline static QList<QPersistentModelIndex> toQPersistentModelIndexList(QModelIndexList mil)
{
    QList<QPersistentModelIndex> retval;

    for(auto i : mil)
    {
        retval.append(i);
    }
    return retval;
}

/**
 *  Map a QItemSelection to a top-level source selection via QAbstractProxyModel::mapSelectionToSource().
 */
inline static QItemSelection mapSelectionToSource(const QItemSelection& proxy_selection)
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

#endif /* MODELHELPERS_H */

