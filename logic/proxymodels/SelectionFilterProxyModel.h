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

class QItemSelectionModel;

/**
 * Proxy model which filters its source model based on the source model's selection.
 * Similar in concept to KDE's KSelectionProxyModel.
 */
class SelectionFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

	Q_PROPERTY(QItemSelectionModel *selectionModel
		   READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged)
			
	using BASE_CLASS = QSortFilterProxyModel;

signals:

	void selectionModelChanged();

public:
	explicit SelectionFilterProxyModel(QObject *parent = Q_NULLPTR);
	virtual ~SelectionFilterProxyModel();
    
	void setSourceModel(QAbstractItemModel* sourceModel) override;

	void setSelectionModel(QItemSelectionModel *selectionModel);
    QItemSelectionModel *selectionModel() const;

	/**
	 * Call this to set the one sourceIndex to pass through this proxy.
	 */
	void setSourceIndexToShow(const QPersistentModelIndex& source_index_to_filter_on);
    
protected:
    
    /**
     * Row filtering function.
     * @param sourceRow     Row to consider relative to sourceModel().
     * @param sourceParent  Parent of row, again relative to sourceModel().
     * @return 
     */
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

protected slots:
	void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	void onModelChanged(QAbstractItemModel *model);
    
private:
	Q_DISABLE_COPY(SelectionFilterProxyModel)

	/// The root index to allow to pass through.
	QPersistentModelIndex m_current_selected_index;

	QItemSelectionModel* m_filter_selection_model = nullptr;
};

#endif /* ENTRYTOMETADATATREEPROXYMODEL_H */

