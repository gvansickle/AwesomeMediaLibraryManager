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

#include "MDITreeViewBase.h"

#include <QUrl>
#include <memory>

class ItemDelegateLength;
class LibrarySortFilterProxyModel;
class MDIPlaylistView;
class LibraryModel;
class LibraryEntry;
class PlaylistModel;

class MDILibraryView : public MDITreeViewBase
{
    Q_OBJECT

    using BASE_CLASS = MDITreeViewBase;

signals:
    void sendEntryToPlaylist(std::shared_ptr<LibraryEntry>, std::shared_ptr<PlaylistModel>);
    void sendToNowPlaying(std::shared_ptr<LibraryEntry>);
    void playTrackNowSignal(QUrl);
        
public:
	explicit MDILibraryView(QWidget *parent = Q_NULLPTR);
        
	/**
	* static member function which opens an MDILibraryView on the given model.
	*/
	static MDILibraryView* openModel(QAbstractItemModel* model, QWidget* parent = nullptr);

	void setModel(QAbstractItemModel* model) override;

	LibrarySortFilterProxyModel* proxy_model() const { return m_sortfilter_model; }


protected:
	LibraryModel* m_underlying_model;

	LibrarySortFilterProxyModel* m_sortfilter_model;
	ItemDelegateLength* m_length_delegate;

	///
	/// Pure virtual function overrides.
	///

	virtual QString getNewFilenameTemplate() const override;
	virtual QString defaultNameFilter() override;

	/// @name Serialization
	/// @{

	virtual bool loadFile(QUrl load_url) override;
	virtual void serializeDocument(QFileDevice& file) const override;
	virtual void deserializeDocument(QFileDevice& file) override;

	/// @}

	virtual bool isModified() const override;

	virtual bool onBlankAreaToolTip(QHelpEvent* event) override;

	/// Helper function to convert from incoming proxy QModelIndexes to actual underlying model indexes.
	QModelIndex to_underlying_qmodelindex(const QModelIndex &proxy_index) override;
	/// Helper function to convert from underlying model indexes to proxy QModelIndexes.
	QModelIndex from_underlying_qmodelindex(const QModelIndex& underlying_index) override;

protected slots:

	void onContextMenuIndex(QContextMenuEvent* event, const QModelIndex& index) override;
	void onContextMenuViewport(QContextMenuEvent* event) override;

	/// @obsolete
	virtual void onContextMenu(QPoint pos);

	/// Invoked when user double-clicks on an entry.
	/// According to Qt5 docs, index will always be valid:
	/// http://doc.qt.io/qt-5/qabstractitemview.html#doubleClicked:
	/// "The [doubleClicked] signal is only emitted when the index is valid."
	void onDoubleClicked(const QModelIndex &index);

private:
	Q_DISABLE_COPY(MDILibraryView)

	std::vector<MDIPlaylistView*> getAllMdiPlaylistViews();
	void addSendToMenuActions(QMenu* menu);

	virtual LibrarySortFilterProxyModel* getTypedModel();
};

#endif // MDILIBRARYVIEW_H
