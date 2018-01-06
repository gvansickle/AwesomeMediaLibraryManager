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

#include "MDIPlaylistView.h"

#include "DragDropTreeViewStyleProxy.h"
#include "DropMenu.h"
#include "ItemDelegateLength.h"

#include <QHeaderView>
#include <QToolTip>
#include <QMessageBox>
#include <QMimeData>
#include <QMimeType>
#include <QMimeDatabase>
#include <QClipboard>
#include <QPoint>

#include <logic/LibrarySortFilterProxyModel.h>
#include "utils/DebugHelpers.h"
#include "logic/LibraryEntryMimeData.h"
#include "utils/ModelHelpers.h"
#include "menus/PlaylistContextMenuViewport.h"
#include "menus/PlaylistContextMenu.h"

MDIPlaylistView::MDIPlaylistView(QWidget* parent) : MDITreeViewBase(parent)
{
	// Set up a Style Proxy to draw a more natural drop indicator.
	this->setStyle(new DragDropTreeViewStyleProxy);


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
	connect(this, &MDIPlaylistView::doubleClicked, this, &MDIPlaylistView::onDoubleClicked);

	// Configure drag and drop.
    // http://doc.qt.io/qt-5/model-view-programming.html#using-drag-and-drop-with-item-views
	// Playlists can have items dragged into them as well as out of them.
	// Playlists can have their items dragged around inside them.

    // Enable dragging.
	setDragEnabled(true);
    // Enable dropping of internal or external items within the view.
    setAcceptDrops(true);
	// View supports both dragging and dropping.
	setDragDropMode(QAbstractItemView::DragDrop);
    // Not entirely sure what's correct here.  From the docs:
    // "  If its value is \c true, the selected data will overwrite the
    //    existing item data when dropped, while moving the data will clear
    //    the item. If its value is \c false, the selected data will be
    //    inserted as a new item when the data is dropped. When the data is
    //    moved, the item is removed as well."
    // At the time of this writing, the behavior appears to be the same regardless of the setting here.
    // Per this: https://github.com/d1vanov/PyQt5-reorderable-list-model
    // "if our model properly implements the removeRows method and the view's dragDropOverwriteMode property
    // is set to false (as it is by default in QListView and QTreeView but not in QTableView), Qt would call
    // it automatically after dropMimeData method if the latter one returns true"
    //
    // Per this: https://stackoverflow.com/a/43963264
    // "[false] specifies if the source item should be removed (typical in a tree view) or cleared [true] (typical in a table view)"
    setDragDropOverwriteMode(false);

    // Default to a Copy drop action.  If it ends up we're dropping onto ourselves, we'll convert this to a Qt::MoveAction
    // in the dropEvent() handler.
    setDefaultDropAction(Qt::CopyAction);

    // Show the user where the item will be dropped.
	setDropIndicatorShown(true);
}

MDIPlaylistView* MDIPlaylistView::openModel(QAbstractItemModel* model, QWidget* parent)
{
	auto view = new MDIPlaylistView(parent);
	view->setModel(model);
	return view;
}


QMediaPlaylist* MDIPlaylistView::getQMediaPlaylist()
{
	return m_underlying_model->qmplaylist();
}

void MDIPlaylistView::setModel(QAbstractItemModel* model)
{
	// Keep a ref to the real model.
	m_underlying_model = qobject_cast<PlaylistModel*>(model);

	// Set our model URLs to the "current file".
	/// @todo This is FBO the Playlist sidebar.  Should we keep the playlist model as a member of this class instead?
	m_underlying_model->setLibraryRootUrl(m_current_url);

	m_sortfilter_model->setSourceModel(model);
	auto old_sel_model = selectionModel();
	MDITreeViewBase::setModel(m_sortfilter_model);
	// Call selectionChanged when the user changes the selection.
	/// @todo selectionModel().selectionChanged.connect(selectionChanged)
	old_sel_model->deleteLater();

	// Connect to the QMediaPlaylist's index changed notifications,
	connect(m_underlying_model->qmplaylist(), &QMediaPlaylist::currentIndexChanged, this, &MDIPlaylistView::playlistPositionChanged);

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

	// Find the "Rating" column and set a delegate on it.
	auto user_rating_col = m_underlying_model->getColFromSection(SectionID(PlaylistSectionID::Rating));
    /// @todo setItemDelegateForColumn(user_rating_col, m_user_rating_delegate);

	setEditTriggers(QAbstractItemView::DoubleClicked|QAbstractItemView::SelectedClicked);
}

QString MDIPlaylistView::getNewFilenameTemplate() const
{
	return QString("Playlist%1.json");
}

QString MDIPlaylistView::defaultNameFilter()
{
	return "M3U8 (*.m3u8);;M3U (*.m3u);;PLS (*.pls);;Windows media player playlist (*.wpl);;XSPF (*.xspf)";
}

void MDIPlaylistView::serializeDocument(QFileDevice& file) const
{
	// Determine which format we're supposed to serialize the playlist to.
	QString fn = file.fileName();
	if(fn.isEmpty())
	{
		QMessageBox::critical(0/*this*/, "Save Error", QString("Unable to determine what format to save playlist as: Filename was empty"));
	}

	// Filename to mimetype.
	QMimeType mt = QMimeDatabase().mimeTypeForFile(fn, QMimeDatabase::MatchExtension);
	if(!mt.isValid())
	{
		QMessageBox::critical(0/*this*/, "Save Error", QString("Unable to determine what format to save playlist as: Filename was '%1'").arg(fn));
	}

	qDebug() << "Mime type is:" << mt << mt.preferredSuffix() << mt.suffixes() << mt.comment();

	if(mt.inherits("application/xspf+xml"))
	{
		// Save it in XSPF format.
		underlyingModel()->serializeToFileAsXSPF(file);
	}
	else if(mt.inherits("application/vnd.apple.mpegurl"))
	{
		// Save in MU8U format.
M_WARNING("TODO: Save in MU8U format")
		underlyingModel()->serializeToFile(file);
	}
}

void MDIPlaylistView::deserializeDocument(QFileDevice& file)
{
	Q_ASSERT(0);
}

bool MDIPlaylistView::isModified() const
{
M_WARNING("TODO: isModified")
	return false;
}

QString MDIPlaylistView::getSaveAsDialogKey() const
{
	return "save_playlist";
}

bool MDIPlaylistView::onBlankAreaToolTip(QHelpEvent* event)
{
	// Return True if you handle it, False if you don't.

	// Blank-area tooltip, for debugging.
	QToolTip::showText(event->globalPos(),
    QString("<b>Playlist Info</b><hr>"
    "Total number of entries: %1").arg(model()->rowCount()));
	return true;
}

//
// Drag and Drop
//
// MoveAction vs. CopyAction vs. who does the remove of the original on a move?
// Per the Qt5 docs, (http://doc.qt.io/qt-5/qtwidgets-draganddrop-fridgemagnets-example.html, 'Dragging" et al),
// it appears that the ::mousePressEvent() handler is ultimately responsible for
// the disposition of the dragged-from item, based on what is returned by the QDrag::exec() call:
//	void DragWidget::mousePressEvent(QMouseEvent *event)
//	{
//		[...]
//		QDrag *drag = new QDrag(this);
//		[...]
//		if (drag->exec(Qt::MoveAction | Qt::CopyAction, Qt::CopyAction) == Qt::MoveAction)
//			child->close(); // ==> "if this action is equal to Qt::MoveAction we will close the activated fridge magnet
//									widget because we will create a new one to replace it (see dropEvent())"
//		else
//			child->show();
//	}
//
// This looks like it is mostly applicable to QAbstractItemView, which does this:
//	void QAbstractItemView::startDrag(Qt::DropActions supportedActions)
//	{
//      [...]
//      QDrag *drag = new QDrag(this);
//		[...]
//		if (drag->exec(supportedActions, defaultDropAction) == Qt::MoveAction)
//			d->clearOrRemove();
//      [...]
//	}
// clearOrRemove() then looks like this:
// if (!overwrite) {
//	[..]
//	model->removeRows((*it).top(), count, parent);
//	} else {
//	// we can't remove the rows so reset the items (i.e. the view is like a table)
//	[...]
//	model->setItemData(index, roles);
//	}
//

/// @note QAbstractItemView::mousePressEvent() is hardcoded to have the left mouse button only initiate drags:
/// @see https://github.com/qt/qtbase/blob/c4f397ee11fc3cea1fc132ebe1db24e3970bb477/src/widgets/itemviews/qabstractitemview.cpp#L1868

void MDIPlaylistView::dragEnterEvent(QDragEnterEvent *event)
{
	if(event->source() == this)
	{
		// Force Qt::MoveAction to change the cursor decoration.
		event->setDropAction(Qt::MoveAction);
	}

	/// QAbstractItemView does this if (mode != InternalMove):
	/// if (d_func()->canDrop(event)) {
	///		event->accept();
	///		setState(DraggingState);
	///	} else {
	///		event->ignore();
	///	}
    MDITreeViewBase::dragEnterEvent(event);
}

void MDIPlaylistView::dragMoveEvent(QDragMoveEvent *event)
{
	if(event->source() == this)
	{
		// Force Qt::MoveAction to change the cursor decoration.
		event->setDropAction(Qt::MoveAction);
	}

	qDebug() << "dropIndicatorPosition:" << dropIndicatorPosition();

	/// @note QTreeView overrides this to start the autoExpandDelay, and then calls the QAbstractItemView::dragMoveEvent(),
	/// https://github.com/qt/qtbase/blob/bbcd4533889b3f3ae45917d638e06bedc9e5c536/src/widgets/itemviews/qabstractitemview.cpp#L1996
	/// ...which does a bunch of work.
	MDITreeViewBase::dragMoveEvent(event);
}

void MDIPlaylistView::dropEvent(QDropEvent* event)
{
	/// https://github.com/qt/qtbase/blob/bbcd4533889b3f3ae45917d638e06bedc9e5c536/src/widgets/itemviews/qabstractitemview.cpp#L2107
	/// Looks like the base class should do everything we need, except we may need to convert
	/// drops-to-self into MoveActions...
	/// ...which doesn't work.  This is the code we're fighting (from QAbstractItemView::dropEvent()):
	///
	/// if (d->dropOn(event, &row, &col, &index)) {
	///	const Qt::DropAction action = dragDropMode() == InternalMove ? Qt::MoveAction : event->dropAction();
	///	if (d->model->dropMimeData(event->mimeData(), action, row, col, index)) {
	///		if (action != event->dropAction()) {
	///			event->setDropAction(action); <== Need action to be MoveAction, and
	///			event->accept();              <== Need this to be called.
	///		} else {
	///			event->acceptProposedAction(); <== Need this to *not* be called, it will accept the CopyAction.
	///		}
	///	}
	///}
	///
	/// So my latest trick here is to detect if we're doing a self-drop, and temporarily switch dragDropMode() to InternalMove.
	/// This seems to make everything work as expected.
	///
	/// @see also this guy: http://www.qtcentre.org/threads/16953-QTreeView-default-drag-action
	///      who reimplements startDrag().

	qDebug() << "Pre-base-class event:" << event << ", Formats:" << event->mimeData()->formats();
	// Save the original dragdrop mode.
	auto original_mode = dragDropMode();
	if(event->source() == this)
	{
		// We're doing a drop onto ourself.
		qDebug() << "Drop Source is ourself, temporarily switching to InternalMove mode";
		setDragDropMode(InternalMove);
	}
	else if(event->possibleActions() & Qt::MoveAction)
	{
		// MoveAction is an option.  Ask the user if they want to copy or move.
		DropMenu dm(tr("Copy or Move?"), this);
		auto selected_action = dm.whichAction(QCursor::pos());
		qDebug() << "Selected action:" << selected_action;
		event->setDropAction(selected_action);
		if(selected_action == Qt::MoveAction)
		{
			// Need to do the same trick here, or the move won't happen.
			qDebug() << "SELECTED MOVE ACTION";
			M_WARNING("/// @todo Doesn't work.");
			setDragDropMode(InternalMove);
		}
	}

	MimeDataDumper mdd(this);
	qDebug() << "QMimeData:";
	mdd.dumpMimeData(event->mimeData());
	MDITreeViewBase::dropEvent(event);
	qDebug() << "Post-base-class event:" << event;

	// Restore the original dragdrop mode.
	setDragDropMode(original_mode);
}

PlaylistModel* MDIPlaylistView::underlyingModel() const
{
	return m_underlying_model;
}

//
// Public slots
//

void MDIPlaylistView::next()
{
	// Forward to the QMediaPlaylist.
	m_underlying_model->qmplaylist()->next();
}

void MDIPlaylistView::previous()
{
	// Forward to the QMediaPlaylist.
	m_underlying_model->qmplaylist()->previous();
}

void MDIPlaylistView::onCut()
{
	qDebug() << "CUTTING";

	// Copy selection to clipboard.
	onCopy();

	// Delete the selected items from this view.
	onDelete();
}


void MDIPlaylistView::onPaste()
{
	qDebug() << "PASTING";
    // Get the current selection.
	auto selmodel = selectionModel();
	if(!selmodel)
	{
		qWarning() << "BAD SELECTION MODEL";
		return;
	}

	QModelIndexList mil = selmodel->selectedRows();
M_WARNING("TODO: Paste at current select position")

	QClipboard *clipboard = QGuiApplication::clipboard();
	if(!clipboard)
	{
		qWarning() << "COULD NOT GET CLIPBOARD";
		return;
	}

	m_underlying_model->dropMimeData(clipboard->mimeData(), Qt::CopyAction, -1, -1, QModelIndex());
}

void MDIPlaylistView::onDelete()
{
	// Remove the current selection.
	QModelIndexList mil = selectionModel()->selectedRows();

	// Convert them to persistent model indexes.
	auto pmil = toQPersistentModelIndexList(mil);
	auto m = model();
	for(auto pi : pmil)
	{
		if(pi.isValid())
		{
			m->removeRow(pi.row(), pi.parent());
		}
		else
		{
			// Index somehow became invalid.
			qWarning() << "ATTEMPTED TO DELETE INVALID INDEX:" << pi;
		}
	}
}

/**
 * Slot which appends the incoming library entry and starts playing it.
 */
void MDIPlaylistView::onSendToNowPlaying(std::shared_ptr<LibraryEntry> new_libentry)
{
M_WARNING("TODO: Dedup")

	// We first need to convert the LibraryEntry to a PlaylistModelItem.
	// Create a new PlaylistModelItem to put in the model.
	std::shared_ptr<PlaylistModelItem> new_playlist_entry = PlaylistModelItem::createFromLibraryEntry(new_libentry);

Q_ASSERT(new_playlist_entry != nullptr);

	// Append to underlying model.
	m_underlying_model->appendRow(new_playlist_entry);

	// Find the last row of the underlying model in top-proxy-model coordinates.
	auto proxy_index = from_underlying_qmodelindex(m_underlying_model->index(std::max(0, m_underlying_model->rowCount()-1), 0));

	qDebug() << "Proxy index:" << proxy_index;

	// Pretend the user double-clicked on it.
	emit onDoubleClicked(proxy_index);
}



void MDIPlaylistView::playlistPositionChanged(qint64 position)
{
	// Notification from the QMediaPlaylist that the current selection has changed.
	// Since we have a QSortFilterProxyModel between us and the underlying model, we need to convert the position,
	// which is in underlying-model coordinates, to proxy model coordinates.
	auto proxy_model_index = from_underlying_qmodelindex(m_underlying_model->index(position, 0));

	// @todo or should this change the selected index?
	setCurrentIndex(proxy_model_index);
}

void MDIPlaylistView::onContextMenuIndex(QContextMenuEvent* event, const QModelIndex& index)
{
	// Open a context menu on the clicked-on row.
	auto context_menu = new PlaylistContextMenu(tr("Playlist Context Menu"), this);
	context_menu->exec(event->globalPos());
}

void MDIPlaylistView::onContextMenuViewport(QContextMenuEvent* event)
{
	// Open the blank area (viewport) context menu.
	// Note that there may be e.g. rows selected in the view, which may affect what menu items are/should be displayed/enabled.
	auto context_menu = new PlaylistContextMenuViewport(tr("Playlist Context Menu - Viewport"), this);
	context_menu->exec(event->globalPos());
}

void MDIPlaylistView::onDoubleClicked(const QModelIndex& index)
{
	// Should always be valid.
	Q_ASSERT(index.isValid());

M_WARNING("TODO: Fix assumptions");
	if(true) // we're the playlist connected to the player.
	{
		// Tell the player to start playing the song at index.
		qDebug() << "Double-clicked index:" << index;
		auto underlying_model_index = to_underlying_qmodelindex(index);

		Q_ASSERT(underlying_model_index.isValid());

		qDebug() << "Underlying index:" << underlying_model_index;

		// Since m_underlying_model->qmplaylist() is connected to the player, we should only have to setCurrentIndex() to
		// start the song.
		/// @note See "jump()" etc in the Qt5 MediaPlyer example.

		m_underlying_model->qmplaylist()->setCurrentIndex(underlying_model_index.row());

		// If the player isn't already playing, the index change above won't start it.  Send a signal to it to
		// make sure it starts.
		emit play();
	}
}

QModelIndex MDIPlaylistView::to_underlying_qmodelindex(const QModelIndex &proxy_index)
{
	auto underlying_model_index = qobject_cast<LibrarySortFilterProxyModel*>(model())->mapToSource(proxy_index);
	Q_ASSERT(underlying_model_index.isValid());

	return underlying_model_index;
}

QModelIndex MDIPlaylistView::from_underlying_qmodelindex(const QModelIndex &underlying_index)
{
	auto proxy_model_index = qobject_cast<LibrarySortFilterProxyModel*>(model())->mapFromSource(underlying_index);
	return proxy_model_index;
}

void MDIPlaylistView::keyPressEvent(QKeyEvent* event)
{
	// QAbstractItemView::keyPressEvent() ->ignore()'s this event if it's the Delete key:
	// https://github.com/qt/qtbase/blob/c4f397ee11fc3cea1fc132ebe1db24e3970bb477/src/widgets/itemviews/qabstractitemview.cpp#L2433

	if(event->matches(QKeySequence::Delete))
	{
		qDebug() << "DELETE KEY IN PLAYLISTVIEW:" << event;
		onDelete();

		// Don't call the parent class' keyPressEvent().
		// We've done what we need to here, the Qt5 docs say not to do it,
		// and it's possible to delete (or at least enable editing on)
		// another unintended row if the tree's edit trigger is set up to AnyKeyPressed.
		event->accept();
		return;
	}
	else if(event->matches(QKeySequence::Copy))
	{
		qDebug() << "Copy Key";
	}
	else if(event->matches(QKeySequence::Paste))
	{
		qDebug() << "Paste Key";
	}
	else if(event->matches(QKeySequence::Cut))
	{
		qDebug() << "Cut key";
	}
	MDITreeViewBase::keyPressEvent(event);
}
