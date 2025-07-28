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

#ifndef LIBRARYSORTFILTERPROXYMODEL_H
#define LIBRARYSORTFILTERPROXYMODEL_H

/**
 * @file
 */

// Std C++
#include <memory>

// Qt
#include <QSortFilterProxyModel>

// Ours
#include "BaseSortFilterProxyModel.h"

class LibraryEntry;

class LibrarySortFilterProxyModel : public BaseSortFilterProxyModel
{
    Q_OBJECT

    using BASE_CLASS = BaseSortFilterProxyModel;

public:
    explicit LibrarySortFilterProxyModel(QObject* parent = Q_NULLPTR);

    std::shared_ptr<LibraryEntry> getItem(QModelIndex index) const;

    bool hasChildren(const QModelIndex &parent) const override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

protected Q_SLOTS:
    void resetInternalData() override;

private:
    Q_DISABLE_COPY(LibrarySortFilterProxyModel)
};

#endif // LIBRARYSORTFILTERPROXYMODEL_H
