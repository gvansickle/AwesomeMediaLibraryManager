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

#ifndef MDILIBRARYVIEW_H
#define MDILIBRARYVIEW_H

// Std C++
#include <memory>
#include <functional>

// Qt
#include <QUrl>

// Ours
#include "MDITreeViewBase.h"
#include "logic/models/LibraryModel.h"  // Needed for covariant return type.
#include "utils/DebugHelpers.h"

class ItemDelegateLength;
class MimeTypeDelegate;

class LibrarySortFilterProxyModel;
class MDIPlaylistView;
class LibraryEntry;
class PlaylistModel;


class MDILibraryView : public MDITreeViewBase
{
    Q_OBJECT

    using BASE_CLASS = MDITreeViewBase;

Q_SIGNALS:
	/**
	 * Signal emitted when the user has selected one or more tracks in this view, and wants to
	 * append/replace them to the "Now Playing" playlist, and possibly start playing them.
	 */
	void sendToNowPlaying(LibraryEntryMimeData*);

	void sendEntryToPlaylist(std::shared_ptr<LibraryEntry>, QPointer<PlaylistModel>);
    void playTrackNowSignal(QUrl);

public:
	explicit MDILibraryView(QWidget *parent = Q_NULLPTR);

	QString getDisplayName() const override;

    /**
     * Pop up an 'Open file" dialog and open a new View on the file specified by the user.
     * ~= "File->Open..."
     *
     * @param find_existing_view_func  Function which, if specified, should search for an existing instance of
     *                                 a view with the same open_url open, and return a pointer to it, or null if none was found.
     */
	static MDIModelViewPair open(QWidget* parent, std::function<MDIModelViewPair(QUrl)> find_existing_view_func = nullptr);

    /**
     * Open the specified QUrl.  Called by open().
     * @param find_existing_view_func  Function which, if specified, should search for an existing instance of
     *                                 a view with the same open_url open, and return a pointer to it, or null if none was found.
     */
	static MDIModelViewPair openFile(QUrl open_url, QWidget* parent,
									 std::function<MDIModelViewPair(QUrl)> find_existing_view_func = nullptr);

    /**
     * Open a new view on the given model.
	 *
	 * @param model  The model to open.  Must exist and must be valid.
     */
	static MDIModelViewPair openModel(QPointer<LibraryModel> model, QWidget* parent);

	void setModel(QAbstractItemModel* model) override;

	LibraryModel* underlyingModel() const override;

	LibrarySortFilterProxyModel* proxy_model() const { return m_sortfilter_model; }


protected:
	QPointer<LibraryModel> m_underlying_model;

	LibrarySortFilterProxyModel* m_sortfilter_model;

	ItemDelegateLength* m_length_delegate;
	MimeTypeDelegate* m_mimetype_delegate;

	///
	/// Pure virtual function overrides.
	///

    void setEmptyModel() override;

	virtual QString getNewFilenameTemplate() const override;
	virtual QString defaultNameFilter() override;

	/// @name Serialization
	/// @{

	/**
	 * Called by openFile().
	 */
    virtual bool readFile(QUrl load_url) override;

	///@todo Override writeFile?

	virtual void serializeDocument(QFileDevice& file) override;
	virtual void deserializeDocument(QFileDevice& file) override;

	/// @}

	/// For GUI purposes, a Library is never in a modified state.
	bool isModified() const override;

	bool onBlankAreaToolTip(QHelpEvent* event) override;

	/// Helper function to convert from incoming proxy QModelIndexes to actual underlying model indexes.
	QModelIndex to_underlying_qmodelindex(const QModelIndex &proxy_index) override;
	/// Helper function to convert from underlying model indexes to proxy QModelIndexes.
	QModelIndex from_underlying_qmodelindex(const QModelIndex& underlying_index) override;

protected Q_SLOTS:

	void onContextMenuSelectedRows(QContextMenuEvent* event, const QPersistentModelIndexVec& row_indexes) override;
	void onContextMenuViewport(QContextMenuEvent* event) override;

	/**
	 * Slot called when the user activates (hits Enter or double-clicks) on an item.
	 * In the Library view, activating an item sends that item to the "Now Playing" playlist
	 * which then starts playing it.
	 */
	void onActivated(const QModelIndex& index) override;

	/// @note OBSOLETE
	/// Invoked when user double-clicks on an entry.
	/// According to Qt docs, index will always be valid:
	/// https://doc.qt.io/qt-6/qabstractitemview.html#doubleClicked:
	/// "The [doubleClicked] signal is only emitted when the index is valid."
	void onDoubleClicked(const QModelIndex &index);

private:
	Q_DISABLE_COPY(MDILibraryView)

	std::vector<MDIPlaylistView*> getAllMdiPlaylistViews();
	void addSendToMenuActions(QMenu* menu);

	virtual LibrarySortFilterProxyModel* getTypedModel();
};

#endif // MDILIBRARYVIEW_H
