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

#include "MDILibraryView.h"

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QHeaderView>
#include <QMenu>
#include <QToolTip>

#include "ItemDelegateLength.h"
#include <logic/LibrarySortFilterProxyModel.h>
#include <gui/MDIPlaylistView.h>
#include <logic/LibraryModel.h>
#include <logic/PlaylistModel.h>
#include <utils/DebugHelpers.h>
#include "menus/LibraryContextMenu.h"
#include "gui/NetworkAwareFileDialog.h"

MDILibraryView::MDILibraryView(QWidget* parent) : MDITreeViewBase(parent)
{
	// Not sure what's going on here, but if I don't set this to something here, the tabs stay "(Untitled)".
	setWindowTitle("DUMMY");

	m_underlying_model = nullptr;

	// The sort and Filter proxy model.
	m_sortfilter_model = new LibrarySortFilterProxyModel(this);
	m_sortfilter_model->setDynamicSortFilter(false);
	m_sortfilter_model->setSortCaseSensitivity(Qt::CaseInsensitive);

	// Delegates.
	m_length_delegate = new ItemDelegateLength(this);

	// Configure selection.
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	// Hook up double-click handler.
	connect(this, &MDILibraryView::doubleClicked, this, &MDILibraryView::onDoubleClicked);

	// Configure drag and drop.
	//// Libraries can't have items dragged around inside them.
	setDragEnabled(false);
	//// Libraries can only have copies dragged out of them.
	setAcceptDrops(false);
	setDragDropMode(QAbstractItemView::DragOnly);
    setDropIndicatorShown(true);
}

/**
 * Pop up an 'Open file" dialog and open a new View on the file specified by the user.
 */
MDILibraryView* MDILibraryView::open(QWidget *parent, std::function<MDILibraryView*(QUrl)> find_existing_view_func)
{
    auto liburl = NetworkAwareFileDialog::getExistingDirectoryUrl(parent, "Select a directory to import", QUrl(), "import_dir");
    QUrl lib_url = liburl.first;

    if(lib_url.isEmpty())
    {
        qDebug() << "User cancelled.";
        return nullptr;
    }
    // Open the directory the user chose as an MDILibraryView and associated model.
M_WARNING("TODO: Need to somehow check if a model already exists and needs a view, or if we just need to activate an existing view.");
    return openFile(lib_url, parent, find_existing_view_func);
}

/**
 * Static member function which opens a view on the given @a open_url.
 */
MDILibraryView* MDILibraryView::openFile(QUrl open_url, QWidget *parent, std::function<MDILibraryView*(QUrl)> find_existing_view_func)
{
    // Check if a view of this URL already exists and we just need to activate it.
    qDebug() << "Looking for existing view of" << open_url;
    auto view = find_existing_view_func(open_url);
    qDebug() << "WTH:" << view;
    if(view)
    {
        qDebug() << "View already exists, returning" << view;
        return view;
    }

    qDebug() << "// Try to open the given URL as a model.";
    auto libmodel = LibraryModel::openFile(open_url, parent);

    if(libmodel)
    {
        auto libview = MDILibraryView::openModel(libmodel, parent);
        libview->setCurrentFile(open_url);
        return libview;
    }
    else
    {
        // User must have cancelled.
        return nullptr;
    }
}

/**
 * static member function which opens an MDILibraryView on the given model.
 */
MDILibraryView* MDILibraryView::openModel(QSharedPointer<LibraryModel> model, QWidget* parent, std::function<MDILibraryView*(QUrl)> find_existing_model_func)
{
	auto view = new MDILibraryView(parent);
	view->setModel(model);
	return view;
}

void MDILibraryView::setModel(QAbstractItemModel* model)
{
    Q_ASSERT(0);
#if 0
	// Keep a ref to the real model.
	m_underlying_model = qobject_cast<LibraryModel*>(model);

	// Set our "current file" to the root dir of the model.
	setCurrentFile(m_underlying_model->getLibRootDir());

	m_sortfilter_model->setSourceModel(model);
	auto old_sel_model = selectionModel();
	// This will create a new selection model.
	MDITreeViewBase::setModel(m_sortfilter_model);
	Q_ASSERT((void*)m_sortfilter_model != (void*)old_sel_model);
	old_sel_model->deleteLater();

	
	// Set up the TreeView's header.
	header()->setStretchLastSection(false);
	header()->setSectionResizeMode(QHeaderView::Stretch);
	header()->setContextMenuPolicy(Qt::CustomContextMenu);

	// Set the resize behavior of the header's columns based on the columnspecs.
	int num_cols = m_underlying_model->columnCount();
	for(int c = 0; c < num_cols; ++c)
	{
		if(m_underlying_model->headerData(c, Qt::Horizontal, Qt::UserRole) == true)
		{
			header()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
		}
	}
	// Find the "Length" column.
	auto len_col = m_underlying_model->getColFromSection(SectionID::Length);
	// Set the delegate on it.
	setItemDelegateForColumn(len_col, m_length_delegate);

	/// @note By default, QHeaderView::ResizeToContents causes the View to query every property of every item in the model.
	/// By setting setResizeContentsPrecision() to 0, it only looks at the visible area when calculating row widths.
    header()->setResizeContentsPrecision(0);
#endif
}

void MDILibraryView::setModel(QSharedPointer<LibraryModel> model)
{
    // Keep a ref to the real model.
    m_underlying_model = model;

    // Set our "current file" to the root dir of the model.
    setCurrentFile(m_underlying_model->getLibRootDir());

    m_sortfilter_model->setSourceModel(model.data());
    auto old_sel_model = selectionModel();
    // This will create a new selection model.
    MDITreeViewBase::setModel(m_sortfilter_model);
    Q_ASSERT((void*)m_sortfilter_model != (void*)old_sel_model);
    old_sel_model->deleteLater();


    // Set up the TreeView's header.
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(QHeaderView::Stretch);
    header()->setContextMenuPolicy(Qt::CustomContextMenu);

    // Set the resize behavior of the header's columns based on the columnspecs.
    int num_cols = m_underlying_model->columnCount();
    for(int c = 0; c < num_cols; ++c)
    {
        if(m_underlying_model->headerData(c, Qt::Horizontal, Qt::UserRole) == true)
        {
            header()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
        }
    }
    // Find the "Length" column.
    auto len_col = m_underlying_model->getColFromSection(SectionID::Length);
    // Set the delegate on it.
    setItemDelegateForColumn(len_col, m_length_delegate);

    /// @note By default, QHeaderView::ResizeToContents causes the View to query every property of every item in the model.
    /// By setting setResizeContentsPrecision() to 0, it only looks at the visible area when calculating row widths.
    header()->setResizeContentsPrecision(0);
}

LibraryModel* MDILibraryView::underlyingModel() const
{
    return m_underlying_model.data();
}

QSharedPointer<LibraryModel> MDILibraryView::underlyingModelSharedPtr() const
{
    return m_underlying_model;
}

void MDILibraryView::setEmptyModel()
{
    M_WARNING("TODO");
    Q_ASSERT(0);
}

QString MDILibraryView::getNewFilenameTemplate() const
{
	QString name = model()->data(QModelIndex(), Qt::UserRole).toString();
	return name;
}

QString MDILibraryView::defaultNameFilter()
{
	return "";
}

bool MDILibraryView::readFile(QUrl load_url)
{
	m_underlying_model->setLibraryRootUrl(load_url);
	setCurrentFile(load_url);
	return true;
}

void MDILibraryView::serializeDocument(QFileDevice& file) const
{
	m_underlying_model->serializeToFile(file);
}

void MDILibraryView::deserializeDocument(QFileDevice& file)
{
	m_underlying_model->deserializeFromFile(file);
}

bool MDILibraryView::isModified() const
{
	return false;
}

bool MDILibraryView::onBlankAreaToolTip(QHelpEvent* event)
{
	// Return True if you handle it, False if you don't.
	// Blank-area tooltip, for debugging.
M_WARNING("TODO: Get/print library stats")
	QToolTip::showText(event->globalPos(),
        QString("<b>Library Info</b><hr>"
        "Total number of entries: %1\n"
		"rowCount: %2").arg(m_underlying_model->getLibraryNumEntries()).arg(model()->rowCount()));
	return true;
}

QModelIndex MDILibraryView::to_underlying_qmodelindex(const QModelIndex &proxy_index)
{
	auto underlying_model_index = qobject_cast<LibrarySortFilterProxyModel*>(model())->mapToSource(proxy_index);
	Q_ASSERT(underlying_model_index.isValid());

	return underlying_model_index;
}

QModelIndex MDILibraryView::from_underlying_qmodelindex(const QModelIndex &underlying_index)
{
	auto proxy_model_index = qobject_cast<LibrarySortFilterProxyModel*>(model())->mapFromSource(underlying_index);
	return proxy_model_index;
}


void MDILibraryView::addSendToMenuActions(QMenu* menu)
{
	auto playlistviews = getAllMdiPlaylistViews();
	if(playlistviews.size() > 0)
	{
		if(playlistviews.size() == 1)
		{
			auto sendToPlaylistAct = menu->addAction(QString("Send to playlist '%1'").arg(playlistviews[0]->windowTitle()));
			sendToPlaylistAct->setData(QVariant::fromValue(playlistviews[0]->underlyingModel()));
		}
		else
		{
			auto submenu = menu->addMenu("Send to playlist");
			for(auto v : playlistviews)
			{
				auto act = submenu->addAction(v->windowTitle());
				act->setData(QVariant::fromValue(v->underlyingModel()));
			}
		}
	}
}

LibrarySortFilterProxyModel* MDILibraryView::getTypedModel()
{
	auto retval = qobject_cast<LibrarySortFilterProxyModel*>(model());
	return retval;
}


void MDILibraryView::onContextMenuIndex(QContextMenuEvent* event, const QModelIndex& index)
{
	// Open context menu for the item.
	qDebug() << "INDEX:" << index;
	
	auto context_menu = new LibraryContextMenu(tr("Library Context Menu"), this);
	context_menu->exec(event->globalPos());
}

void MDILibraryView::onContextMenuViewport(QContextMenuEvent* event)
{
	// Open the blank area context menu.
	qDebug() << "Viewport";

	auto context_menu = new LibraryContextMenu(tr("Library Context Menu"), this);
	context_menu->exec(event->globalPos());
}

void MDILibraryView::onContextMenu(QPoint pos)
{
	// Position to put the menu.
	auto globalPos = mapToGlobal(pos);
	// The QModelIndex() that was right-clicked.
	auto modelindex = indexAt(pos);
	qDebug() << "INDEX:" << modelindex.row() << modelindex.column();
	if(!modelindex.isValid())
	{
		qDebug() << "Invalid model index, not showing context menu.";
		return;
	}
	auto menu = new QMenu(this);
	//sendToPlaylistAct = menu->addAction("Send to playlist");
	addSendToMenuActions(menu);
	auto playNowAct = menu->addAction("Play");
	auto extractAct = menu->addAction("Extract to file...");
	auto selectedItem = menu->exec(globalPos);
	if(selectedItem)
	{
		if(selectedItem->data().isValid())
		{
			// Send the selected library item to the selected playlist.
			emit sendEntryToPlaylist(std::shared_ptr<LibraryEntry>(getTypedModel()->getItem(modelindex)), selectedItem->data().value<std::shared_ptr<PlaylistModel>>());
		}
		else if( selectedItem == extractAct)
		{
//			auto item = model()->getItem(modelindex);
//			auto tc = Transcoder();
//			tc.extract_track(item, "deleteme.flac");
		}
		else if(selectedItem == playNowAct)
		{
			// Play the track.
			qDebug() << "PLAY";
//			onPlayTrack(modelindex);
		}
	}
}

/**
 * Handler which gets invoked by a double-click on a Library Model item.
 * Sends the clicked-on item to the "Now Playing" playlist to be played.
 */
void MDILibraryView::onDoubleClicked(const QModelIndex &index)
{
	// Should always be valid.
	Q_ASSERT(index.isValid());

	// Tell the player to start playing the song at index.
	qDebug() << "Double-clicked index:" << index;
	auto underlying_model_index = to_underlying_qmodelindex(index);

	Q_ASSERT(underlying_model_index.isValid());

	qDebug() << "Underlying index:" << underlying_model_index;

	// Get the item that was double clicked.
	auto item = m_underlying_model->getItem(underlying_model_index);

	Q_ASSERT(item != nullptr);

	// Send it to the "Now Playing" playlist, by way of MainWindow.
	emit sendToNowPlaying(item);
}


std::vector<MDIPlaylistView*> MDILibraryView::getAllMdiPlaylistViews()
{
	auto subwindows = getQMdiSubWindow()->mdiArea()->subWindowList(QMdiArea::ActivationHistoryOrder);
	std::vector<MDIPlaylistView*> retval;
	for(auto s : subwindows)
	{
		auto w = s->widget();
		if(w != nullptr)
		{
			auto pv = qobject_cast<MDIPlaylistView*>(w);
			if(pv != nullptr)
			{
				retval.push_back(pv);
			}
		}
	}
	return retval;
}
