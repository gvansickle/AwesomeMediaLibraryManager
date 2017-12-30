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
class EntryToMetadataTreeProxyModel;

class MetadataDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit MetadataDockWidget(const QString &title, QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());

    void connectToView(MDITreeViewBase* view);
    
public slots:
    void playlistSelectionChanged(const QItemSelection& newSelection, const QItemSelection&);

private:
	Q_DISABLE_COPY(MetadataDockWidget)

	EntryToMetadataTreeProxyModel* m_proxy_model;

	QItemSelectionModel* m_connected_selection_model;

    QTreeWidget* m_metadata_widget;

    PixmapLabel* m_cover_image_label;

    void addChildrenFromTagMap(QTreeWidgetItem* parent, const TagMap& tagmap);

};

#endif // METADATADOCKWIDGET_H
