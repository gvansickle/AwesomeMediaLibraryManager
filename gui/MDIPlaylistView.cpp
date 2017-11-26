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

#include "ItemDelegateLength.h"

#include <QHeaderView>
#include <QToolTip>
#include <QMessageBox>
#include <QMimeType>
#include <QMimeDatabase>

#include <logic/LibrarySortFilterProxyModel.h>
#include "utils/DebugHelpers.h"

MDIPlaylistView::MDIPlaylistView(QWidget* parent) : MDITreeViewBase(parent)
{
	m_underlying_model = nullptr;

	// The sort and Filter proxy model.
	m_sortfilter_model = new LibrarySortFilterProxyModel(this);
	m_sortfilter_model->setDynamicSortFilter(false);
	m_sortfilter_model->setSortCaseSensitivity(Qt::CaseInsensitive);

	// Delegates.
	m_length_delegate = new ItemDelegateLength(this);

	// Configure selection.
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	// Configure drag and drop.
	// Playlistst can have items dragged into them as well as out of them.
	// Playlists can have their items dragged around inside them.
	setDragEnabled(true);
	setAcceptDrops(true);
	setDragDropMode(QAbstractItemView::DragDrop);  // View supports both dragging and dropping.
	setDragDropOverwriteMode(false);
	setDefaultDropAction(Qt::MoveAction);
	setDropIndicatorShown(true);

	// Hook up the context menu.
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &MDIPlaylistView::customContextMenuRequested, this, &MDIPlaylistView::onContextMenu);

	// Call selectionChanged when the user changes the selection.
	/// @todo selectionModel().selectionChanged.connect(selectionChanged)
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
	QString("<b>Debug Info</b><hr>"
	"Num items in model: %1").arg(model()->rowCount()));
	return true;
}

/*!
	If the event hasn't already been accepted, determines the index to drop on.
	if (row == -1 && col == -1)
		// append to this drop index
	else
		// place at row, col in drop index
	If it returns \c true a drop can be done, and dropRow, dropCol and dropIndex reflects the position of the drop.
	\internal
  */
bool MDIPlaylistView::dropOn(QDropEvent *event, int *dropRow, int *dropCol, QModelIndex *dropIndex)
{
	//Q_Q(QAbstractItemView);
	if (event->isAccepted())
		return false;

	QModelIndex root = QModelIndex();

	QModelIndex index;
	// rootIndex() (i.e. the viewport) might be a valid index
	if (viewport()->rect().contains(event->pos())) {
		index = /*q->*/indexAt(event->pos());
		if (!index.isValid() || !/*q->*/visualRect(index).contains(event->pos()))
			index = root;
	}

	// If we are allowed to do the drop
	if (model()->supportedDropActions() & event->dropAction())
	{
		int row = -1;
		int col = -1;
		if (index != root)
		{
//			auto dip = position(event->pos(), /*q->*/visualRect(index), index);
//			switch (dropIndicatorPosition) {
//			case QAbstractItemView::AboveItem:
				row = index.row();
				col = index.column();
				index = index.parent();
//				break;
//			case QAbstractItemView::BelowItem:
//				row = index.row() + 1;
//				col = index.column();
//				index = index.parent();
//				break;
//			case QAbstractItemView::OnItem:
//			case QAbstractItemView::OnViewport:
//				break;
//			}
		}
		else
		{
//			dropIndicatorPosition = QAbstractItemView::OnViewport;
		}
		*dropIndex = index;
		*dropRow = row;
		*dropCol = col;
//		if (!droppingOnItself(event, index))
			return true;
	}
	return false;
}

void MDIPlaylistView::dropEvent(QDropEvent* event)
{
	qDebug() << "dropEvent()" << event;

	// Based on this: https://github.com/qt/qtbase/blob/5.10/src/widgets/itemviews/qtreewidget.cpp
	if(event->source() == this)
	{
		// We're doing a move drop onto ourself.
		qDebug() << "dropEvent(): source is ourself:" << event;

#if 1
		// Drop source is ourself.  We want to turn any proposed copies into moves.
		if(event->proposedAction() == Qt::CopyAction)
		{
			// It's a copy, is move available?
			if(event->possibleActions() & Qt::MoveAction)
			{
				// Yes, tell Qt that we'll accept a move instead.
				qDebug() << "dropEvent(): converting to MoveAction:" << event;
				event->setDropAction(Qt::MoveAction);
				//event->accept();
				qDebug() << "dropEvent(): converted to MoveAction:" << event;
			}
			else
			{
				// Copy drop from ourself with no move option.  Reject it.
				qDebug() << "dropEvent(): no MoveAction option, ignoring:" << event;
				event->ignore();
				return;
			}
		}
#else

		QModelIndex topIndex;
		int col = -1;
		int row = -1;
		if (dropOn(event, &row, &col, &topIndex))
		{
			// Get the list of model indexes in the source of the drop.
			QList<QModelIndex> idxs = selectedIndexes();
			QList<QPersistentModelIndex> indexes;
			for (int i = 0; i < idxs.count(); i++)
			{
				indexes.append(idxs.at(i));
			}

			if (indexes.contains(topIndex))
			{
				return;
			}

			// When removing items the drop location could shift
			QPersistentModelIndex dropRow = model()->index(row, col, topIndex);

			// Remove the items
			QList<LibraryEntry*> taken;
			for (int i = indexes.count() - 1; i >= 0; --i)
			{
				LibraryEntry* item = underlyingModel()->getItem(indexes[i]);
				taken.append(item);
				model()->removeRow(indexes[i].row(), indexes[i].parent());
			}

			// insert them back in at their new positions
			for (int i = 0; i < indexes.count(); ++i)
			{
				if(row == -1)
				{
					// Append a top-level item.
					underlyingModel()->appendRow(taken[i]);
				}
				else
				{
					// Insert a top level item.
					int r = dropRow.row() >= 0 ? dropRow.row() : row;
					underlyingModel()->insertRow(r, QModelIndex());
					underlyingModel()->setData(underlyingModel()->index(r, 0, QModelIndex()), QVariant::fromValue(taken[i]), Qt::EditRole);
				}
			}

			event->accept();
			// Don't want QAbstractItemView to delete it because it was "moved" we already did it
			event->setDropAction(Qt::CopyAction);
		}
#endif
	}

	qDebug() << "dropEvent(): Calling base class with event:" << event;
	return MDITreeViewBase::dropEvent(event);
}

PlaylistModel* MDIPlaylistView::underlyingModel() const
{
	return m_underlying_model;
}

void MDIPlaylistView::onContextMenu(QPoint pos)
{
	/// @todo Implement a playlist context menu.

	// Position to put the menu.
//	auto globalPos = mapToGlobal(pos);
	// The QModelIndex() that was right-clicked.
	auto modelindex = indexAt(pos);
	qDebug() << QString("INDEX: {} {} ").arg(modelindex.row()).arg(modelindex.column());
	if(!modelindex.isValid())
	{
		qDebug() << "Invalid model index, not showing context menu.";
		return;
	}
	//menu = self.createContextMenu(modelindex)
}

//	# @pyqtSlot(QItemSelection, QItemSelection)
//	# def selectionChanged(self, newSelection: QItemSelection, oldSelection: QItemSelection) -> None:
//	#     #self.playlistSelectionChanged.emit(newSelection, oldSelection)
//	#     pass


void MDIPlaylistView::playlistPositionChanged(qint64 position)
{
	// Notification from the QMediaPlaylist that the current selection has changed.
	// Since we have a QSortFilterProxyModel between us and the underlying model, we need to convert the position,
	// which is in underlying-model coordinates, to proxy model coordinates.
	auto proxy_model_index = qobject_cast<LibrarySortFilterProxyModel*>(model())->mapFromSource(m_underlying_model->index(position, 0));
	setCurrentIndex(proxy_model_index);
}
