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
MDIModelViewPair MDILibraryView::open(QWidget *parent, std::function<MDIModelViewPair(QUrl)> find_existing_view_func)
{
    auto liburl = NetworkAwareFileDialog::getExistingDirectoryUrl(parent, "Select a directory to import", QUrl(), "import_dir");
    QUrl lib_url = liburl.first;

    if(lib_url.isEmpty())
    {
        qDebug() << "User cancelled.";
		return MDIModelViewPair();
    }

    // Open the directory the user chose as an MDILibraryView and associated model.
    // Note that openFile() may return an already-existing view if one is found by find_existing_view_func().
    return openFile(lib_url, parent, find_existing_view_func);
}

/**
 * Static member function which opens a view on the given @a open_url.
 * Among other things, this function is responsible for calling setCurrentFile().
 */
MDIModelViewPair MDILibraryView::openFile(QUrl open_url, QWidget *parent, std::function<MDIModelViewPair(QUrl)> find_existing_view_func)
{
    // Check if a view of this URL already exists and we just need to activate it.
    qDebug() << "Looking for existing view of" << open_url;
    auto mv_pair = find_existing_view_func(open_url);
    if(mv_pair.m_view)
    {
        Q_ASSERT_X(mv_pair.m_view_was_existing == true, "openFile", "find_existing function returned a view but said it was not pre-existing.");
        qDebug() << "View of" << open_url << "already exists, returning" << mv_pair.m_view;
        return mv_pair;
    }

	// No existing view.  Open a new one.

	/// @note This should probably be creating an empty View here and then
	/// calling an overridden readFile().

	qDebug() << "// Try to open a model on the given URL.";
	QSharedPointer<LibraryModel> libmodel;
	if(mv_pair.m_model)
	{
		Q_ASSERT_X(mv_pair.m_model_was_existing, "openFile", "find_exisiting returned a model but said it was not pre-existing.");

		qDebug() << "Model exists:" << mv_pair.m_model;
		libmodel = qSharedPointerObjectCast<LibraryModel>(mv_pair.m_model);
	}
	else
	{
		qDebug() << "Opening new model on URL" << open_url;
		libmodel = LibraryModel::openFile(open_url, parent);
	}

    if(libmodel)
    {
		// The model has either been found already existing and with no associated View, or it has been newly opened.
		// Either way it's valid and we now create and associate a View with it.

		auto mvpair = MDILibraryView::openModel(libmodel, parent);
		/// @note Need this cast due to some screwyness I mean subtleties of C++'s member access control system.
		/// In very shortened form: Derived member functions can only access "protected" members through
		/// an object of the Derived type, not of the Base type.
		static_cast<MDILibraryView*>(mvpair.m_view)->setCurrentFile(open_url);
		return mvpair;
    }
    else
    {
		// Library import failed.
M_WARNING("TODO: Add a QMessageBox or something here.");
		return MDIModelViewPair();
    }
}

/**
 * static member function which opens an MDILibraryView on the given model.
 * @param model  The model to open.  Must exist and must be valid.
 */
MDIModelViewPair MDILibraryView::openModel(QSharedPointer<LibraryModel> model, QWidget* parent)
{
	MDIModelViewPair retval;
	retval.setModel(model);

	retval.m_view = new MDILibraryView(parent);
	static_cast<MDILibraryView*>(retval.m_view)->setModel(model);

	return retval;
}

void MDILibraryView::setModel(QAbstractItemModel* model)
{
	Q_ASSERT(0); /// Obsolete, use QSharedPointer version.
}

void MDILibraryView::setModel(QSharedPointer<QAbstractItemModel> model)
{
    // Keep a ref to the real model.
	m_underlying_model = qSharedPointerObjectCast<LibraryModel>(model);

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

QSharedPointer<QAbstractItemModel> MDILibraryView::underlyingModelSharedPtr() const
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

void MDILibraryView::onActivated(const QModelIndex& index)
{
	// Should always be valid.
	Q_ASSERT(index.isValid());

	// Tell the player to start playing the song at index.
	qDebug() << "Activated index:" << index;
	auto underlying_model_index = to_underlying_qmodelindex(index);

	Q_ASSERT(underlying_model_index.isValid());

	qDebug() << "Underlying index:" << underlying_model_index;

	// Get the item that was activated.
	auto item = m_underlying_model->getItem(underlying_model_index);

	Q_ASSERT(item != nullptr);

	// Send it to the "Now Playing" playlist, by way of MainWindow.
	emit sendToNowPlaying(item);
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
#if 0 /// We're handling this in the onActivated() handler at the moment.
	// Send it to the "Now Playing" playlist, by way of MainWindow.
	emit sendToNowPlaying(item);
#endif
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
