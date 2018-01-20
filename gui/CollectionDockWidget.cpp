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
#include "MDILibraryView.h"
#include "MDINowPlayingView.h"

#include <QContextMenuEvent>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>


CollectionDockWidget::CollectionDockWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(title, parent, flags)
{
    // FBO QSettings.
    setObjectName("CollectionDockWidget");

    // Don't allow this dock widget to be floated or closed, but allow it to be moved to a different dock area.
    setFeatures(QDockWidget::DockWidgetMovable);
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

	m_sources_model = new QStandardItemModel(this);

	m_collection_tree_view = new QTreeView(this);
	m_collection_tree_view->setModel(m_sources_model);
	m_collection_tree_view->setRootIsDecorated(false);
    // Want to have the tree always expanded.
	m_collection_tree_view->setExpandsOnDoubleClick(false);
	m_collection_tree_view->setHeaderHidden(true);
    // Prevent double-click from starting a file rename (i.e. edit) operation.
	m_collection_tree_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // Hook up the context menu.
	m_collection_tree_view->setContextMenuPolicy(Qt::DefaultContextMenu);
	setWidget(m_collection_tree_view);

	m_localLibsItem = new QStandardItem("Libraries");
	m_localLibsItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
	auto font = QFont(m_localLibsItem->font());
    font.setBold(true);
	m_localLibsItem->setFont(font);
	m_playlistsItem = new QStandardItem("Playlists");
	m_playlistsItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
	m_playlistsItem->setFont(font);

    // Add top-level items.
	m_sources_model->invisibleRootItem()->appendRows({m_localLibsItem, m_playlistsItem});

    // Connect the double-click signal to a custom handler.
	connect(m_collection_tree_view, &QTreeView::doubleClicked, this, &CollectionDockWidget::tree_doubleclick);

	m_collection_tree_view->expandAll();
}

void CollectionDockWidget::setModel(QPointer<QStandardItemModel> model)
{
	m_sources_model = model;
	m_collection_tree_view->setModel(m_sources_model);
}

void CollectionDockWidget::addLibrary(LocalLibraryItem* lib)
{
	qDebug() << "Adding local library: " << lib;
	m_localLibsItem->appendRow(lib);
	m_collection_tree_view->expandAll();
}

void CollectionDockWidget::addPlaylist(PlaylistItem* playlist)
{
	qDebug() << "Adding playlist:" << playlist;
	m_playlistsItem->appendRow(playlist);
	m_collection_tree_view->expandAll();
}

void CollectionDockWidget::contextMenuEvent(QContextMenuEvent* event)
{
    qDebug() << "Context menu";

    // CollectionDockWidget actually got the right-click event after the QTreeView ignored it,
    // so we have to convert the position back.
	auto treepos = m_collection_tree_view->mapFromParent(event->pos());

	auto modelindex = m_collection_tree_view->indexAt(treepos);
    auto parentindex = modelindex.parent();
	///qDebug() << QString("Parent: {}/{}/{}".format(modelindex.parent(), modelindex.parent().row(), modelindex.parent().column()));

	if (parentindex == m_sources_model->indexFromItem(m_localLibsItem))
    {
        doLibraryContextMenu(event, treepos);
    }
	else if (parentindex == m_sources_model->indexFromItem(m_playlistsItem))
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
	QModelIndex modelindex = m_collection_tree_view->indexAt(treepos);
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
	emit showLibViewSignal(modelindex.data(Qt::UserRole+1).value<QSharedPointer<LibraryModel>>());
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
		emit removeLibModelFromLibSignal(modelindex.data(Qt::UserRole+1).value<QSharedPointer<LibraryModel>>());
		// Remove the entry in our Tree model.
		m_sources_model->removeRow(modelindex.row(), modelindex.parent());
	}
}


void CollectionDockWidget::tree_doubleclick(QModelIndex modelindex)
{
	if(!modelindex.isValid())
	{
		return;
	}

	auto libmodel = modelindex.data(Qt::UserRole + 1).value<QSharedPointer<LibraryModel>>();
	if(libmodel)
	{
		emit showLibViewSignal(libmodel);
		return;
	}

	auto parentindex = modelindex.parent();
	qDebug() << QString("Parent:") << modelindex.parent() << modelindex.parent().row() << modelindex.parent().column();
	if(parentindex == m_sources_model->indexFromItem(m_localLibsItem))
	{
		emit showLibViewSignal(modelindex.data(Qt::UserRole + 1).value<QSharedPointer<LibraryModel>>());
	}
	else if(parentindex == m_sources_model->indexFromItem(m_playlistsItem))
	{
		qDebug() << QString("Playlist menu, not implemented.");
	}
}

void CollectionDockWidget::view_is_closing(MDITreeViewBase* viewptr, QAbstractItemModel* modelptr)
{
	qDebug() << "Got closing() signal from view" << viewptr;

	// We only care if it's not a Library or Now Playing view.
	if(qobject_cast<MDILibraryView*>(viewptr) || qobject_cast<MDINowPlayingView*>(viewptr))
	{
		qDebug() << "Ignoring, view is Now Playing or Library";
		return;
	}

	auto parentindex = m_sources_model->indexFromItem(m_playlistsItem);

	auto indexes_to_delete = m_sources_model->match(parentindex, si_role_view_ptr,
													QVariant::fromValue<MDITreeViewBase*>(viewptr), -1,
													Qt::MatchExactly | Qt::MatchRecursive);
	qDebug() << "Num indexes found:" << indexes_to_delete.size();

	for(auto i : indexes_to_delete)
	{
		m_sources_model->removeRow(i.row(), parentindex);
	}

}

