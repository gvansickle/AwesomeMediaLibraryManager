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

#ifndef COLLECTIONDOCKWIDGET_H
#define COLLECTIONDOCKWIDGET_H

#include "MDIPlaylistView.h"

#include <QDockWidget>
#include <QWidget>
#include <QModelIndex>
#include <QStandardItemModel>

#include <logic/LibraryModel.h>
#include <logic/PlaylistModelItem.h>

class QStandardItemModel;
class QTreeView;

class LocalLibraryItem;
class PlaylistModelItem;

class LocalLibraryItem : public QStandardItem
{
public:
	LocalLibraryItem(LibraryModel* libmodel)
	{
		m_libmodel = libmodel;
		setData(QVariant::fromValue(libmodel));
		setData(QIcon::fromTheme("folder"), Qt::DecorationRole);
	};

	QVariant data(int role = Qt::UserRole+1) const override
	{
		// Get the data we need from the model we're connected to
		if(role == Qt::EditRole || role == Qt::DisplayRole)
		{
			return QVariant(m_libmodel->getLibraryName());
		}
		else if(role == Qt::ToolTipRole)
		{
			return QVariant(m_libmodel->getLibRootDir());
		}
		else
		{
			return QStandardItem::data(role);
		}
	}

private:
	LibraryModel* m_libmodel;
};

class PlaylistItem: public QStandardItem
{
public:
	PlaylistItem(MDIPlaylistView* view)
	{
		//playlist_model: PlaylistModel = model;
		playlist_view = view;

		setData(QIcon::fromTheme("folder"), Qt::DecorationRole);
	};

	QVariant data(int role = Qt::UserRole+1) const override
	{
		// Get the data we need from the model we're connected to
		if(role == Qt::EditRole || role == Qt::DisplayRole)
		{
			return QVariant(playlist_view->userFriendlyCurrentFile());
		}
		else if(role == Qt::ToolTipRole)
		{
			return QVariant(playlist_view->getCurrentUrl());
		}
		else
		{
			return QStandardItem::data(role);
		}
	}

private:
	  MDIPlaylistView* playlist_view;
};

class CollectionDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit CollectionDockWidget(const QString &title, QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());

	void addLibrary(LocalLibraryItem* library);
	void addPlaylist(PlaylistItem* playlist);


signals:
	// Signal indicating the user wants to remove the given LibraryModel.
	void removeLibModelFromLibSignal(LibraryModel*);

	// Signal indicating the user wants to show the window for the given LibraryModel.
	void showLibViewSignal(LibraryModel*);

public slots:
	void tree_doubleclick(QModelIndex modelindex);

protected:
	void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QStandardItemModel* sourcesModel;
    QTreeView* collectionTreeView;

	QStandardItem* localLibsItem;
	QStandardItem* playlistsItem;

	void doLibraryContextMenu(QContextMenuEvent* event, QPoint treepos);
	void onShowLib(QModelIndex modelindex);
	void onRemoveLib(QModelIndex modelindex);
};

#endif // COLLECTIONDOCKWIDGET_H
