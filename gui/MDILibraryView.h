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

class ItemDelegateLength;
class LibrarySortFilterProxyModel;
class MDIPlaylistView;
class LibraryModel;
class LibraryEntry;
class PlaylistModel;

class MDILibraryView : public MDITreeViewBase
{
	Q_OBJECT

public:
	MDILibraryView(QWidget *parent = Q_NULLPTR);

	virtual void setModel(QAbstractItemModel* model) override;

	LibrarySortFilterProxyModel* proxy_model() const { return m_sortfilter_model; }

signals:
	void sendEntryToPlaylist(LibraryEntry*, PlaylistModel*);
	void playTrackNowSignal(QUrl);

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

	virtual bool loadFile(QUrl load_url);
	virtual void serializeDocument(QFileDevice& file) const override;
	virtual void deserializeDocument(QFileDevice& file) override;

	/// @}

	virtual bool isModified() const override;

	virtual bool onBlankAreaToolTip(QHelpEvent* event) override;

protected slots:

	virtual void onContextMenu(QPoint pos);

private:
	Q_DISABLE_COPY(MDILibraryView)

	std::vector<MDIPlaylistView*> getAllMdiPlaylistViews();
	void addSendToMenuActions(QMenu* menu);

	virtual LibrarySortFilterProxyModel* getTypedModel();
};

#endif // MDILIBRARYVIEW_H
