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

#include "CollectionDockWidget.h"

#include <QContextMenuEvent>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>


CollectionDockWidget::CollectionDockWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(title, parent, flags)
{
    // FBO QSettings.
    setObjectName("CollectionDockWidget");

    // Don't allow this dock widget to be floated or closed, but allow it to be moved to a different dock area.
    setFeatures(QDockWidget::DockWidgetMovable);
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    sourcesModel = new QStandardItemModel(this);

    collectionTreeView = new QTreeView(this);
    collectionTreeView->setModel(sourcesModel);
    collectionTreeView->setRootIsDecorated(false);
    // Want to have the tree always expanded.
    collectionTreeView->setExpandsOnDoubleClick(false);
    collectionTreeView->setHeaderHidden(true);
    // Prevent double-click from starting a file rename (i.e. edit) operation.
    collectionTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // Hook up the context menu.
    collectionTreeView->setContextMenuPolicy(Qt::DefaultContextMenu);
    setWidget(collectionTreeView);

	localLibsItem = new QStandardItem("Libraries");
    localLibsItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    auto font = QFont(localLibsItem->font());
    font.setBold(true);
    localLibsItem->setFont(font);
	playlistsItem = new QStandardItem("Playlists");
    playlistsItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    playlistsItem->setFont(font);

    // Add top-level items.
    sourcesModel->invisibleRootItem()->appendRows({localLibsItem, playlistsItem});

    // Connect the double-click signal to a custom handler.
	connect(collectionTreeView, &QTreeView::doubleClicked, this, &CollectionDockWidget::tree_doubleclick);

    collectionTreeView->expandAll();
}

void CollectionDockWidget::addLibrary(LocalLibraryItem* lib)
{
	qDebug() << "Adding local library: " << lib;
	localLibsItem->appendRow(lib);
	collectionTreeView->expandAll();
}

void CollectionDockWidget::addPlaylist(PlaylistItem* playlist)
{
	qDebug() << "Adding playlist";
	playlistsItem->appendRow(playlist);
	collectionTreeView->expandAll();
}

void CollectionDockWidget::contextMenuEvent(QContextMenuEvent* event)
{
    qDebug() << "Context menu";

    // CollectionDockWidget actually got the right-click event after the QTreeView ignored it,
    // so we have to convert the position back.
    auto treepos = collectionTreeView->mapFromParent(event->pos());

    auto modelindex = collectionTreeView->indexAt(treepos);
    auto parentindex = modelindex.parent();
	///qDebug() << QString("Parent: {}/{}/{}".format(modelindex.parent(), modelindex.parent().row(), modelindex.parent().column()));

    if (parentindex == sourcesModel->indexFromItem(localLibsItem))
    {
        doLibraryContextMenu(event, treepos);
    }
    else if (parentindex == sourcesModel->indexFromItem(playlistsItem))
    {
		qDebug("Playlist menu, not implemented.");
    }
    else
    {
		event->ignore();
        return;
    }

    event->accept();
}


void CollectionDockWidget::doLibraryContextMenu(QContextMenuEvent* event, QPoint treepos)
{
	QPoint pos = event->pos();
	// Position to put the menu.
	QPoint globalPos = mapToGlobal(pos);
	// The QModelIndex() that was right-clicked.
	QModelIndex modelindex = collectionTreeView->indexAt(treepos);
	qDebug() << QString("INDEX:") << modelindex.row() << modelindex.column();
	if(!modelindex.isValid())
	{
		qDebug() << QString("Invalid model index, not showing context menu.");
		return;
	}
	auto menu = new QMenu(this);
	auto showAct = menu->addAction("Show");
	auto removeAct = menu->addAction("Remove...");
	auto selectedItem = menu->exec(globalPos);
	if(selectedItem == removeAct)
	{
		onRemoveLib(modelindex);
	}
	else if(selectedItem == showAct)
	{
		onShowLib(modelindex);
	}
}


void CollectionDockWidget::onShowLib(QModelIndex modelindex)
{
	emit showLibViewSignal(modelindex.data(Qt::UserRole+1).value<LibraryModel*>());
}

void CollectionDockWidget::onRemoveLib(QModelIndex modelindex)
{
	QString name = modelindex.data(Qt::DisplayRole).toString();
	auto url = modelindex.data(Qt::ToolTipRole);
	auto mb = new QMessageBox(this);
	mb->setIcon(QMessageBox::Question);
	mb->setText("Remove Library Directory");
	mb->setInformativeText(QString("Do you really want to remove '%1' from the library?").arg(name));
	mb->setDetailedText(QString("Name: '%1'\nURL: '%2'").arg(name).arg(url.toString()));
	mb->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	mb->setDefaultButton(QMessageBox::No);
	auto retval = mb->exec();

	if(retval == QMessageBox::Yes)
	{
		// Remove the directory.
		emit removeLibModelFromLibSignal(modelindex.data(Qt::UserRole+1).value<LibraryModel*>());
		// Remove the entry in out Tree model.
		sourcesModel->removeRow(modelindex.row(), modelindex.parent());
	}
}


void CollectionDockWidget::tree_doubleclick(QModelIndex modelindex)
{
	if(!modelindex.isValid())
	{
		return;
	}

	auto parentindex = modelindex.parent();
	qDebug() << QString("Parent:") << modelindex.parent() << modelindex.parent().row() << modelindex.parent().column();
	if(parentindex == sourcesModel->indexFromItem(localLibsItem))
	{
		emit showLibViewSignal(modelindex.data(Qt::UserRole + 1).value<LibraryModel*>());
	}
	else if(parentindex == sourcesModel->indexFromItem(playlistsItem))
	{
		qDebug() << QString("Playlist menu, not implemented.");
	}
}

