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
#include <QSplitter>
#include <QPushButton>
#include <QToolButton>


CollectionDockWidget::CollectionDockWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(title, parent, flags)
{
    // FBO QSettings.
    setObjectName("CollectionDockWidget");

    // Don't allow this dock widget to be floated or closed, but allow it to be moved to a different dock area.
    setFeatures(QDockWidget::DockWidgetMovable);
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

	auto center_widget = new QSplitter(Qt::Vertical, this);

	m_collection_tree_view = new QTreeView(this);
	m_collection_tree_view->setRootIsDecorated(false);
    // Want to have the tree always expanded.
	m_collection_tree_view->setExpandsOnDoubleClick(false);
	m_collection_tree_view->setHeaderHidden(true);
    // Prevent double-click from starting a file rename (i.e. edit) operation.
	m_collection_tree_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // Hook up the context menu.
	m_collection_tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_collection_tree_view, &QTreeView::customContextMenuRequested, this, &CollectionDockWidget::onTreeContextMenu);

	/// @todo EXPERIMENTAL
	if(false)
	{
	m_tree_widget = new QTreeWidget(this);
	QPushButton *topLevelButton = new QPushButton("Top Level Button");
	QTreeWidgetItem *topLevelItem = new QTreeWidgetItem();
	m_tree_widget->addTopLevelItem(topLevelItem);
	m_tree_widget->setItemWidget(topLevelItem, 0, topLevelButton);

	center_widget->addWidget(m_tree_widget);
	}

	center_widget->addWidget(m_collection_tree_view);

	// Set the widget for this dock widget.
	setWidget(center_widget);

    // Connect the double-click signal to a custom handler.
	connect(m_collection_tree_view, &QTreeView::doubleClicked, this, &CollectionDockWidget::tree_doubleclick);

	m_collection_tree_view->expandAll();
}

void CollectionDockWidget::setModel(QPointer<QStandardItemModel> model)
{
	m_sources_model = model;
	m_collection_tree_view->setModel(m_sources_model);
	m_collection_tree_view->expandAll();
}

void CollectionDockWidget::addActionExperimental(QAction* act)
{
	QToolButton *button = new QToolButton();
	button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	button->setDefaultAction(act);

	QTreeWidgetItem *treewidgetitem = new QTreeWidgetItem();
	auto parent = m_tree_widget->invisibleRootItem()->child(0);
	parent->addChild(treewidgetitem);
	m_tree_widget->setItemWidget(treewidgetitem, 0, button);
}

void CollectionDockWidget::onTreeContextMenu(const QPoint& point)
{
	qDebug() << "Tree Context menu";

	auto modelindex = m_collection_tree_view->indexAt(point);

	qDebug() << "INDEX:" << modelindex;

	auto item = m_sources_model->itemFromIndex(modelindex);

	if(!item)
	{
		qDebug() << "NO ITEM AT INDEX:" << modelindex;
		return;
	}

	qDebug() << "ITEM:" << item->data(Qt::DisplayRole);

	auto libmodel = item->data(Qt::UserRole + 1).value<QSharedPointer<LibraryModel>>();
	if(libmodel)
	{
		doLibraryContextMenu(point);
		return;
	}

}

QSharedPointer<LibraryModel> CollectionDockWidget::modelIndexToLibraryModelPtr(const QModelIndex& modelindex) const
{
	if(!modelindex.isValid())
	{
		qWarning() << "INVALID INDEX:" << modelindex;
		return nullptr;
	}

	auto libmodel = modelindex.data(Qt::UserRole+1).value<QSharedPointer<LibraryModel>>();
	if(!libmodel)
	{
		qWarning() << "NO VALID LIBMODEL AT INDEX:" << modelindex;
	}

	return libmodel;
}


void CollectionDockWidget::doLibraryContextMenu(QPoint treepos)
{
	// Get a global coordinate to put the menu at.
	auto menu_pos = m_collection_tree_view->mapToGlobal(treepos);

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
	auto selectedItem = menu->exec(menu_pos);
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
	auto libmodel = modelIndexToLibraryModelPtr(modelindex);
	if(!libmodel)
	{
		qWarning() << "NO VALID LIBMODEL AT INDEX:" << modelindex;
		return;
	}

	emit showLibraryModelSignal(libmodel);
}

void CollectionDockWidget::onRemoveLib(QModelIndex modelindex)
{
	auto libmodel = modelIndexToLibraryModelPtr(modelindex);
	if(!libmodel)
	{
		qWarning() << "NO VALID LIBMODEL AT INDEX:" << modelindex;
		return;
	}

	QString name = modelindex.data(Qt::DisplayRole).toString();
	auto url = libmodel->getLibRootDir();
	auto mb = new QMessageBox(this);
	mb->setIcon(QMessageBox::Question);
	mb->setText(tr("Remove Library Directory"));
	mb->setInformativeText(QString("Do you really want to remove '%1' from the library?").arg(name));
	mb->setDetailedText(QString("Name: '%1'\nURL: '%2'").arg(name).arg(url.toDisplayString()));
	mb->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	mb->setDefaultButton(QMessageBox::No);
	auto retval = mb->exec();

	if(retval == QMessageBox::Yes)
	{
		// Remove the directory.
		emit removeLibModelFromLibSignal(libmodel);
		// Remove the entry in our Tree model.
		m_sources_model->removeRow(modelindex.row(), modelindex.parent());
	}
}


void CollectionDockWidget::tree_doubleclick(QModelIndex modelindex)
{
//	m_collection_tree_view->expandAll();
	qDebug() << "DOUBLECLICK on INDEX:" << modelindex;
	if(!modelindex.isValid())
	{
		qDebug() << "Invalid index:" << modelindex;
		return;
	}

	if(!modelindex.parent().isValid())
	{
		// No parent, it's a top-level item.
		qDebug() << "Section index, ignoring:" << modelindex;
		return;
	}

	auto libmodel = modelindex.data(Qt::UserRole + 1).value<QSharedPointer<LibraryModel>>();
	if(libmodel)
	{
		emit showLibraryModelSignal(libmodel);
		return;
	}

	auto playlist_view = modelindex.data(Qt::UserRole + 1).value<MDIPlaylistView*>();
	qDebug() << "Playlistview:" << playlist_view << "MDISubwin:" << playlist_view->getQMdiSubWindow();
	if(playlist_view)
	{
		emit activateSubwindow(playlist_view->getQMdiSubWindow());
	}
}

