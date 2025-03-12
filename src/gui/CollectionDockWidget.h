/*
 * Copyright 2017, 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef COLLECTIONDOCKWIDGET_H
#define COLLECTIONDOCKWIDGET_H

#include <QDockWidget>
#include <QWidget>
#include <QModelIndex>
#include <QPointer>
#include <QTreeWidget>

#include "utils/DebugHelpers.h"

class QStandardItemModel;
class QTreeView;
class QMdiSubWindow;

class LibraryModel;

class CollectionDockWidget : public QDockWidget
{
    Q_OBJECT

Q_SIGNALS:
	// Signal indicating the user wants to remove the given LibraryModel.
	void removeLibModelFromLibSignal(QPointer<LibraryModel>);

	// Signal indicating the user wants to show the window for the given LibraryModel.
	void showLibraryModelSignal(QPointer<LibraryModel>);

	void activateSubwindow(QMdiSubWindow* subwindow);

public:
    explicit CollectionDockWidget(const QString &title, QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());

    ~CollectionDockWidget() override = default;

	void setModel(QPointer<QStandardItemModel> model);

	void addActionExperimental(QAction* act);

public Q_SLOTS:
	void tree_doubleclick(QModelIndex modelindex);

protected:

protected Q_SLOTS:
	void onTreeContextMenu(const QPoint &point);

private:
	Q_DISABLE_COPY(CollectionDockWidget)

	QPointer<QStandardItemModel> m_sources_model;
    QPointer<QTreeView> m_collection_tree_view;

	QPointer<LibraryModel> modelIndexToLibraryModelPtr(const QModelIndex& modelindex) const;

	void doLibraryContextMenu(QPoint treepos);
	void onShowLib(QModelIndex modelindex);
	void onRemoveLib(QModelIndex modelindex);
};

#endif // COLLECTIONDOCKWIDGET_H
