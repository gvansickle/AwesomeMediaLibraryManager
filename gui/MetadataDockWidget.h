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

class SelectionFilterProxyModel;
class ModelChangeWatcher;


class MetadataDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit MetadataDockWidget(const QString &title, QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());

    void connectToView(MDITreeViewBase* view);
    
public slots:
    
    /**
     * Slot which we connect up to m_proxy_model->&EntryToMetadataTreeProxyModel::dataChanged signal.
     * Invoked when a setData() happens on the EntryToMetadataTreeProxyModel.
	 * @note The model indexes will be relative to m_proxy_model.
     */
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());

protected:

	void PopulateTreeWidget(const QModelIndex& first_model_index);

private:
    Q_DISABLE_COPY(MetadataDockWidget)

	/**
	 * The proxy model we'll use to select out just the currently selected or playing track.
	 */
    SelectionFilterProxyModel* m_proxy_model { nullptr };

	ModelChangeWatcher* m_proxy_model_watcher { nullptr };

    QTreeWidget* m_metadata_widget { nullptr };

    QTreeView* m_metadata_tree_view { nullptr };

    PixmapLabel* m_cover_image_label { nullptr };

    void addChildrenFromTagMap(QTreeWidgetItem* parent, const TagMap& tagmap);

private slots:
	/**
	 * Signaled by the ModelChangeWatcher on a change in m_proxy_model.
	 */
	void onProxyModelChange(bool);
};

#endif // METADATADOCKWIDGET_H
