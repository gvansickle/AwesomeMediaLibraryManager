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

#ifndef MDIPLAYLISTVIEW_H
#define MDIPLAYLISTVIEW_H

#include "MDITreeViewBase.h"

#include <QMediaPlaylist>

#include <logic/PlaylistModel.h>

class LibrarySortFilterProxyModel;
class ItemDelegateLength;

class MDIPlaylistView : public MDITreeViewBase
{
	Q_OBJECT

public:
	MDIPlaylistView(QWidget *parent = Q_NULLPTR);

	QMediaPlaylist* getQMediaPlaylist();

	virtual void setModel(QAbstractItemModel* model) override;

	virtual PlaylistModel* underlyingModel() const;

protected:

	///
	/// Pure virtual function overrides.
	///
	virtual QString getNewFilenameTemplate() const override;
	virtual QString defaultNameFilter() override;

	virtual void serializeDocument(QFileDevice& file) const override;
	virtual void deserializeDocument(QFileDevice& file) override;

	virtual bool isModified() const override;

	virtual QString getSaveAsDialogKey() const override;

	virtual bool onBlankAreaToolTip(QHelpEvent* event) override;

	/// Drag and Drop
	virtual void dropEvent(QDropEvent* event) override;

protected slots:
	virtual void playlistPositionChanged(qint64 position);

private slots:
	void onContextMenu(QPoint pos);

private:
	Q_DISABLE_COPY(MDIPlaylistView)

	bool dropOn(QDropEvent *event, int *dropRow, int *dropCol, QModelIndex *dropIndex);

	PlaylistModel* m_underlying_model;
	LibrarySortFilterProxyModel* m_sortfilter_model;
	ItemDelegateLength* m_length_delegate;
};

#endif // MDIPLAYLISTVIEW_H
