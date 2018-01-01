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

#ifndef METADATADOCKWIDGET_H
#define METADATADOCKWIDGET_H

#include <QDockWidget>

#include "logic/MetadataAbstractBase.h"

class MDITreeViewBase;
class QTreeWidget;
class PixmapLabel;
class QItemSelection;
class QItemSelectionModel;
class QTreeWidgetItem;
class QTreeView;
class EntryToMetadataTreeProxyModel;

class MetadataDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit MetadataDockWidget(const QString &title, QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());

    void connectToView(MDITreeViewBase* view);
    
public slots:
    
    /**
     * Slot we connect up to the view's selectionModel()->QItemSelectionModel::selectionChanged signal.
     * Note then that it's receiving selections relative to view->model(), which may need conversion.
     */
    void viewSelectionChanged(const QItemSelection& newSelection, const QItemSelection&);

    /**
     * Slot which we connect up to m_proxy_model->&EntryToMetadataTreeProxyModel::dataChanged signal.
     * Invoked when a setData() happens on the EntryToMetadataTreeProxyModel.
     * @note The model indexes will be relative to m_proxy_model, so again may need conversion.
     */
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());

    ///@todo
    void PopulateTreeWidget(const QModelIndex& first_model_index);

private:
    Q_DISABLE_COPY(MetadataDockWidget)

    EntryToMetadataTreeProxyModel* m_proxy_model { nullptr };

//    QItemSelectionModel* m_connected_selection_model { nullptr };

    QTreeWidget* m_metadata_widget { nullptr };

    QTreeView* m_metadata_tree_view { nullptr };

    PixmapLabel* m_cover_image_label { nullptr };

    void addChildrenFromTagMap(QTreeWidgetItem* parent, const TagMap& tagmap);

};

#endif // METADATADOCKWIDGET_H
