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
#include "logic/LibraryModel.h"

#include <QUrl>

#include <memory>
#include <functional>

class ItemDelegateLength;
class LibrarySortFilterProxyModel;
class MDIPlaylistView;
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
	static MDIModelViewPair openModel(QSharedPointer<LibraryModel> model, QWidget* parent,
									  std::function<MDIModelViewPair(QUrl)> find_existing_model_func = nullptr,
									  MDIModelViewPair mvpair = MDIModelViewPair());

    void setModel(QAbstractItemModel* model) override;
    void setModel(QSharedPointer<LibraryModel> model);

    LibraryModel* underlyingModel() const override;
    QSharedPointer<LibraryModel> underlyingModelSharedPtr() const;

	LibrarySortFilterProxyModel* proxy_model() const { return m_sortfilter_model; }


protected:
    QSharedPointer<LibraryModel> m_underlying_model;

	LibrarySortFilterProxyModel* m_sortfilter_model;
	ItemDelegateLength* m_length_delegate;

	///
	/// Pure virtual function overrides.
	///

    void setEmptyModel() override;

	virtual QString getNewFilenameTemplate() const override;
	virtual QString defaultNameFilter() override;

	/// @name Serialization
	/// @{

    virtual bool readFile(QUrl load_url) override;
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
