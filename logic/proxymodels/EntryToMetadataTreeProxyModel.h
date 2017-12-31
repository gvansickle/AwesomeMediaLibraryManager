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

#ifndef ENTRYTOMETADATATREEPROXYMODEL_H
#define ENTRYTOMETADATATREEPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QPersistentModelIndex>

class EntryToMetadataTreeProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

	using BASE_CLASS = QSortFilterProxyModel;
    
public:
    explicit EntryToMetadataTreeProxyModel(QObject *parent = Q_NULLPTR);
    virtual ~EntryToMetadataTreeProxyModel();
    
	void setSourceModel(QAbstractItemModel* sourceModel) override;

	/**
	 * Call this to set the one sourceIndex to pass through this proxy.
	 */
	void setSourceIndexToShow(const QPersistentModelIndex& source_index_to_filter_on);
    
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    
private:
	Q_DISABLE_COPY(EntryToMetadataTreeProxyModel)

	QPersistentModelIndex m_current_selected_index;
};

#endif /* ENTRYTOMETADATATREEPROXYMODEL_H */

