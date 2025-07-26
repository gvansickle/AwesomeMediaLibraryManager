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

#include "BaseSortFilterProxyModel.h"

// Ours
#include <utils/ConnectHelpers.h>

BaseSortFilterProxyModel::BaseSortFilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent)
{
	// Connect to signals emitted before/after model is reset.
	// We can't really override QSortFilterProxyModel::setSourceModel() because:
	// 1. It does a lot of work.
	// 2. It calls beginResetModel()/endResetModel(), which don't nest, so we can't call them in derived proxymodels.
	connect_or_die(this, &QSortFilterProxyModel::modelAboutToBeReset, this, &BaseSortFilterProxyModel::onModelAboutToBeReset);
	connect_or_die(this, &QSortFilterProxyModel::modelReset, this, &BaseSortFilterProxyModel::onModelReset);
}

void BaseSortFilterProxyModel::onModelAboutToBeReset()
{
	// NO-OP
}

void BaseSortFilterProxyModel::onModelReset()
{
	// NO-OP
}

