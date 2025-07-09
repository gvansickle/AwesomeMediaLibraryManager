/*
 * Copyright 2017, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include <gui/delegates/ItemDelegateLength.h>
#include "MDILibraryView.h"

// Qt
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QHeaderView>
#include <QMenu>
#include <QToolTip>
#include <QContextMenuEvent>
#include <QMessageBox>

// Ours
#include <gui/MDIPlaylistView.h>
#include <logic/models/LibraryModel.h>
#include <logic/models/PlaylistModel.h>
#include <utils/DebugHelpers.h>
#include <logic/proxymodels/ModelHelpers.h>
#include "menus/LibraryContextMenu.h"
#include "gui/NetworkAwareFileDialog.h"
#include "logic/proxymodels/QPersistentModelIndexVec.h"

#include <gui/delegates/MimeTypeDelegate.h>
#include <logic/models/LibraryEntryMimeData.h>
#include <logic/ModelUserRoles.h>
#include <logic/proxymodels/LibrarySortFilterProxyModel.h>
#include <logic/serialization/XmlSerializer.h>
#include <AMLMApp.h>
#include <MainWindow.h>

MDILibraryView::MDILibraryView(QWidget* parent) : MDITreeViewBase(parent)
{
	// Not sure what's going on here, but if I don't set this to something here, the tabs stay "(Untitled)".
	/// @todo Seems like we no longer need to do this.
//	setWindowTitle("DUMMY");

	m_act_window->setIcon(QIcon::fromTheme("folder"));

	m_underlying_model = nullptr;

	// The sort and Filter proxy model.
	m_sortfilter_model = new LibrarySortFilterProxyModel(this);
	m_sortfilter_model->setDynamicSortFilter(false);
	m_sortfilter_model->setSortCaseSensitivity(Qt::CaseInsensitive);

	// Delegates.
	m_length_delegate = new ItemDelegateLength(this);
    m_mimetype_delegate = new MimeTypeDelegate(this);

	// Configure selection.
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	// Hook up double-click handler.
	connect_or_die(this, &MDILibraryView::doubleClicked, this, &MDILibraryView::onDoubleClicked);

	// Configure drag and drop.
	//// Libraries can't have items dragged around inside them.
	setDragEnabled(false);
	//// Libraries can only have copies dragged out of them.
	setAcceptDrops(false);
	setDragDropMode(QAbstractItemView::DragOnly);
	setDropIndicatorShown(true);
}

QString MDILibraryView::getDisplayName() const
{
	/// @todo Let the user rename the library/ies.
	return tr("Library");
}

/**
 * Pop up an "Open file" dialog and open a new View on the file specified by the user.
 */
MDIModelViewPair MDILibraryView::open(QWidget *parent, std::function<MDIModelViewPair(QUrl)> find_existing_view_func)
{
	auto liburl = NetworkAwareFileDialog::getExistingDirectoryUrl(parent, "Select a directory to import",
		QUrl(""), AMLMSettings::NAFDDialogId::ImportDir);
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
	qDebug() << "Looking for existing MDIModelViewPair of" << open_url;
    auto mv_pair = find_existing_view_func(open_url);
    if(mv_pair.getView())
    {
        Q_ASSERT_X(mv_pair.m_view_was_existing == true, "openFile", "find_existing function returned a view but said it was not pre-existing.");
        qDebug() << "View of" << open_url << "already exists, returning" << mv_pair.getView().data();
        return mv_pair;
    }

	// No existing view.  Open a new one.

	/// @note This should probably be creating an empty View here and then
	/// calling an overridden readFile().

	QPointer<LibraryModel> libmodel;
	if(mv_pair.hasModel())
	{
		Q_ASSERT_X(mv_pair.m_model_was_existing, "openFile", "find_exisiting returned a model but said it was not pre-existing.");

        qDebug() << "Model exists:" << mv_pair.getTopModel().data();
		libmodel = qobject_cast<LibraryModel*>(mv_pair.getRootModel());
	}
	else
	{
		qDebug() << "Opening new model on URL" << open_url;
		libmodel = LibraryModel::openFile(open_url, parent);
	}

	if(libmodel != nullptr)
    {
		// The model has either been found already existing and with no associated View, or it has been newly opened.
		// Either way it's valid and we now create and associate a View with it.

		auto mvpair = MDILibraryView::openModel(libmodel, parent);

		/// @todo This should be done somewhere else, so that the mvpair we get above already has this set correctly.
		mvpair.m_model_was_existing = mv_pair.m_model_was_existing;

		/// @note Need this cast due to some screwyness I mean subtleties of C++'s member access control rules.
		/// In very shortened form: Derived member functions can only access "protected" members through
		/// an object of the Derived type, not of the Base type.
        qobject_cast<MDILibraryView*>(mvpair.getView())->setCurrentFile(open_url);
		return mvpair;
    }
    else
    {
		// Library import failed.
        QMessageBox::critical(amlmApp->IMainWindow(), "Error", "Library import failed", QMessageBox::Ok);
		return MDIModelViewPair();
    }
}

/**
 * static member function which opens an MDILibraryView on the given model.
 * @param model  The model to open.  Must exist and must be valid.
 */
MDIModelViewPair MDILibraryView::openModel(QPointer<LibraryModel> model, QWidget* parent)
{
	MDIModelViewPair retval;
	retval.appendModel(model);

	retval.appendView(new MDILibraryView(parent));
	/// @todo DELETE
	// qobject_cast<MDILibraryView*>(retval.getView())->setModel(model);

	return retval;
}

void MDILibraryView::setModel(QAbstractItemModel* model)
{
	// Keep a ref to the real model.
	m_underlying_model = qobject_cast<LibraryModel*>(model);

	// Set our "current file" to the root dir of the model.
	setCurrentFile(m_underlying_model->getLibRootDir());

	m_sortfilter_model->setSourceModel(model);
	auto old_sel_model = selectionModel();
	// This will create a new selection model.
	MDITreeViewBase::setModel(m_sortfilter_model);
	Q_ASSERT((void*)m_sortfilter_model != (void*)old_sel_model);
    if(old_sel_model != nullptr)
    {
        old_sel_model->deleteLater();
    }


	// Set up the TreeView's header.
	header()->setStretchLastSection(false);
	header()->setSectionResizeMode(QHeaderView::Stretch);
	header()->setContextMenuPolicy(Qt::CustomContextMenu);

	// Set the resize behavior of the header's columns based on the columnspecs.
	int num_cols = m_underlying_model->columnCount();
	for(int c = 0; c < num_cols; ++c)
	{
		if(m_underlying_model->headerData(c, Qt::Horizontal, ModelUserRoles::HeaderViewSectionShouldFitWidthToContents) == true)
		{
			header()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
		}
	}

	//
	// Set up delegates.
	//

	// Find the "Length" column.
	auto len_col = m_underlying_model->getColFromSection(SectionID::Length);
	// Set the delegate on it.
	setItemDelegateForColumn(len_col, m_length_delegate);

	// Mime type column.
    auto mimetype_col = m_underlying_model->getColFromSection(SectionID::MIMEType);
	setItemDelegateForColumn(mimetype_col, m_mimetype_delegate);

	/// @note By default, QHeaderView::ResizeToContents causes the View to query every property of every item in the model.
	/// By setting setResizeContentsPrecision() to 0, it only looks at the visible area when calculating row widths.
	header()->setResizeContentsPrecision(0);
}

LibraryModel* MDILibraryView::underlyingModel() const
{
	return m_underlying_model;
}

LibrarySortFilterProxyModel* MDILibraryView::proxy_model() const
{
	return m_sortfilter_model;
}

void MDILibraryView::setEmptyModel()
{
    // M_WARNING("TODO");
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

void MDILibraryView::serializeDocument(QFileDevice& file)
{
//	m_underlying_model->serializeToFile(file);
	Q_ASSERT(0);
// M_WARNING("TODO: Serialization, remove or replace.");

	QString database_filename = QDir::homePath() + "/AMLMDatabaseSerDes.xml";

	qIn() << "###### WRITING" << database_filename;

	XmlSerializer xmlser;
	xmlser.set_default_namespace("http://xspf.org/ns/0/", "1");
	xmlser.save(*m_underlying_model, QUrl::fromLocalFile(database_filename), "the_library_model_from_view");

	qIn() << "###### WROTE" << database_filename;

}

void MDILibraryView::deserializeDocument(QFileDevice& file)
{
	// M_WARNING("TODO: Serialization, remove or replace.");
	Q_ASSERT(0);

//	m_underlying_model->deserializeFromFile(file);
}

bool MDILibraryView::isModified() const
{
	return false;
}

bool MDILibraryView::onBlankAreaToolTip(QHelpEvent* event)
{
	// Return True if you handle it, False if you don't.
	// Blank-area tooltip, for debugging.
// M_WARNING("TODO: Get/print more library stats")
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
	if(!playlistviews.empty())
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


void MDILibraryView::onContextMenuSelectedRows(QContextMenuEvent* event, const QPersistentModelIndexVec& row_indexes)
{
	// Open context menu for a seletion of one or more items.
	auto context_menu = new LibraryContextMenu(tr("Library Context Menu"), row_indexes, this);
	auto selected_action = context_menu->exec(event->globalPos());

	if(selected_action == context_menu->m_act_append_to_now_playing)
	{
		// User wants to append tracks to "Now Playing".
		LibraryEntryMimeData* mime_data = selectedRowsToMimeData(row_indexes);
		mime_data->m_drop_target_instructions = { DropTargetInstructions::IDAE_APPEND, DropTargetInstructions::PA_START_PLAYING };

		// Send tracks to the "Now Playing" playlist and start playing the first one.
		Q_EMIT sendToNowPlaying(mime_data);
	}
	else if(selected_action == context_menu->m_act_replace_playlist)
	{
		// User wants to clear "Now Playing" and replace it with the selection.
		LibraryEntryMimeData* mime_data = selectedRowsToMimeData(row_indexes);
		mime_data->m_drop_target_instructions = { DropTargetInstructions::IDAE_REPLACE, DropTargetInstructions::PA_START_PLAYING };

		// Replace tracks in the "Now Playing" playlist and start playing the first one.
		Q_EMIT sendToNowPlaying(mime_data);
	}
}

void MDILibraryView::onContextMenuViewport(QContextMenuEvent* event)
{
	// Open the blank area context menu.
	qDebug() << "Viewport";

	auto context_menu = new LibraryContextMenu(tr("Library Context Menu"), this);
	context_menu->exec(event->globalPos());
}

/**
 * Slot called when the user activates (hits Enter or double-clicks) on an item or items.
 * In the Library view, activating items appends the items to the "Now Playing" playlist
 * and then starts playing the first one.
 */
void MDILibraryView::onActivated(const QModelIndex& index)
{
	// Should always be valid.
	Q_ASSERT(index.isValid());

	// In the Library view, activating an item sends that item to the "Now Playing" playlist
	// which then starts playing it.

	qDebug() << "Activated index:" << index;

	// The activated item should be in the current selection.
	/// @todo Add a check for that.
	auto selected_row_pindexes = selectedRowPindexes();

	if(selected_row_pindexes.empty())
	{
		qWarning() << "Should have more than one selected row, got 0";
	}

	// Send the tracks to the "Now Playing" playlist, by way of MainWindow.
	LibraryEntryMimeData* mime_data = selectedRowsToMimeData(selected_row_pindexes);
	mime_data->m_drop_target_instructions = { DropTargetInstructions::IDAE_APPEND, DropTargetInstructions::PA_START_PLAYING };
	Q_EMIT sendToNowPlaying(mime_data);
}

/**
 * Handler which gets invoked by a double-click on a Library Model item.
 * Sends the clicked-on item to the "Now Playing" playlist to be played.
 */
void MDILibraryView::onDoubleClicked(const QModelIndex &index)
{
	Q_UNUSED(index);
#if 0 /// We're handling this in the onActivated() handler at the moment.
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
	Q_EMIT sendToNowPlaying(item);
#endif
}


std::vector<MDIPlaylistView*> MDILibraryView::getAllMdiPlaylistViews()
{
	auto subwindows = getQMdiSubWindow()->mdiArea()->subWindowList(QMdiArea::ActivationHistoryOrder);
	std::vector<MDIPlaylistView*> retval;
	for(const auto& s : std::as_const(subwindows))
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
