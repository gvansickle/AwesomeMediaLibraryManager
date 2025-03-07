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

#ifndef MDIPLAYLISTVIEW_H
#define MDIPLAYLISTVIEW_H

#include "MDITreeViewBase.h"
#include <logic/PlaylistModel.h>

class QMediaPlaylist;
class LibrarySortFilterProxyModel;
class ItemDelegateLength;
class DragDropTreeViewStyleProxy;


class MDIPlaylistView : public MDITreeViewBase
{
    Q_OBJECT

    using BASE_CLASS = MDITreeViewBase;

Q_SIGNALS:
	/// @name Signals for player-connected messages.
	/// @{
	/// Start playing the current song.
	void play();
	/// @}

public:
    explicit MDIPlaylistView(QWidget *parent = Q_NULLPTR);
    ~MDIPlaylistView() override;


    /**
     * static member function which opens an MDILibraryView on the given model.
     */
    static MDIModelViewPair openModel(QPointer<PlaylistModel> model, QWidget* parent);

    QMediaPlaylist* getQMediaPlaylist();

    void setModel(QAbstractItemModel* model) override;

    PlaylistModel* underlyingModel() const override;

    /// Playlists are not read-only.
    bool isReadOnly() const override { return false; }

	// QMediaPlaylist-like member functions.
	QUrl currentMedia() const;

public Q_SLOTS:

	/// @name Slots for player-connected messages.
	/// @{

	/**
	 * Start next song.
	 * Makes the next item in the model the current item in the view.
	 */
    void next();

	/// Start previous song.
    void previous();
	/// @}

    /// @name Edit slots.
    /// Related const operations (copy, select all, etc) are in MDITreeViewBase.
    /// @{

    /// Copy the current selection to the clipboard and delete it.
    void onCut() override;

    /// Paste from clipboard.
    void onPaste() override;

    /// Delete the currently selected rows.
    void onDelete() override;

    /// @}

	/**
	 * Slot which accepts a LibraryEntryMimeData* from a signal.
	 * Appends or replaces the incoming tracks in @a mime_data and possibly starts playing the first one.
	 * @note Deletes mime_data.
	 */
	void onSendToNowPlaying(LibraryEntryMimeData* mime_data);

protected:

    ///
    /// Pure virtual function overrides.
    ///
    QString getNewFilenameTemplate() const override;
    QString defaultNameFilter() override;

    void setEmptyModel() override;

    void serializeDocument(QFileDevice& file) override;
    void deserializeDocument(QFileDevice& file) override;

    bool isModified() const override;

    QString getSaveAsDialogKey() const override;

    bool onBlankAreaToolTip(QHelpEvent* event) override;

    /// Drag and Drop
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent* event) override;

    /// Helper function to convert from incoming proxy QModelIndexes to actual underlying model indexes.
    QModelIndex to_underlying_qmodelindex(const QModelIndex &proxy_index) override;
    /// Helper function to convert from underlying model indexes to proxy QModelIndexes.
    QModelIndex from_underlying_qmodelindex(const QModelIndex& underlying_index) override;

	/**
	 * keyPressEvent() override.  Not sure we need this anymore.  Qt5 docs:
	 * "This function is called with the given event when a key event is sent to the widget. The default implementation
	 * handles basic cursor movement, e.g. Up, Down, Left, Right, Home, PageUp, and PageDown; the activated() signal
	 * is emitted if the current index is valid and the activation key is pressed (e.g. Enter or Return, depending on the platform).
	 * This function is where editing is initiated by key press, e.g. if F2 is pressed."
	 */
    void keyPressEvent(QKeyEvent *event) override;

public Q_SLOTS:
	virtual void playlistPositionChanged(qint64 position);

protected Q_SLOTS:

	void onContextMenuSelectedRows(QContextMenuEvent* event, const QPersistentModelIndexVec& row_indexes) override;
	void onContextMenuViewport(QContextMenuEvent* event) override;

    /// Invoked when user double-clicks on an entry.
    /// According to Qt5 docs, index will always be valid:
    /// http://doc.qt.io/qt-5/qabstractitemview.html#doubleClicked:
    /// "The [doubleClicked] signal is only emitted when the index is valid."
    void onDoubleClicked(const QModelIndex &index);

	/**
	 * Slot called when the user activates (hits Enter) on an item.
	 * @param index
	 */
	void onActivated(const QModelIndex& index) override;

private:
    Q_DISABLE_COPY(MDIPlaylistView)

    /**
     * Tell the player component to start playing the song at @a index.
     * @param index
     */
    void startPlaying(const QModelIndex& index);

    std::unique_ptr<DragDropTreeViewStyleProxy> m_the_dragdropstyleproxy;

    QPointer<PlaylistModel> m_underlying_model;
    LibrarySortFilterProxyModel* m_sortfilter_model;
    ItemDelegateLength* m_length_delegate;

	/// true == shuffle, false == sequential.
	bool m_shuffle {false};

	/// PMI to the current track to be played/is being played.
	QPersistentModelIndex m_current_track_index;
};

#endif // MDIPLAYLISTVIEW_H
