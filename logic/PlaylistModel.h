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

#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include "LibraryModel.h"
#include "PlaylistModelItem.h"

#include <memory>

class QMediaPlaylist;


struct PlaylistSectionID : public SectionID
{
	enum Enumerator
	{
		Rating = int(SectionID::PLAYLIST_1) + 0,
		Blacklist
	};

	PlaylistSectionID() = default;
	PlaylistSectionID(PlaylistSectionID::Enumerator e) { m_val = e; }
	operator int() const { return m_val; }
	operator SectionID() { return SectionID(m_val); }
};

class PlaylistModel : public LibraryModel
{
	Q_OBJECT

public:
	PlaylistModel(QObject* parent);

	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant data(const QModelIndex &index, int role) const override;

	/// Override this in derived classes to return a newly-allocated, default-constructed instance
	/// of an entry derived from LibraryEntry.  Used by insertRows().
	virtual std::shared_ptr<LibraryEntry> createDefaultConstructedEntry() const override;

	virtual std::shared_ptr<LibraryEntry> getItem(const QModelIndex& index) const override;

	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

	/// @name Drag and drop support.
	/// @{

	virtual QStringList mimeTypes() const override;
	Qt::DropActions supportedDragActions() const override;
	Qt::DropActions supportedDropActions() const override;
	virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
	virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

	/// @}

    virtual void setLibraryRootUrl(const QUrl& url) override;


	QMediaPlaylist* qmplaylist();

	bool serializeToFileAsXSPF(QFileDevice& filedev) const;

protected:
	virtual void onRowsInserted(QModelIndex parent, int first, int last) override;
	virtual void onRowsRemoved(QModelIndex parent, int first, int last) override;
	virtual void onSetData(QModelIndex index, QVariant value, int role = Qt::EditRole) override;

private:
	Q_DISABLE_COPY(PlaylistModel)

	QMediaPlaylist* m_qmplaylist;
};

Q_DECLARE_METATYPE(const PlaylistModel*)
Q_DECLARE_METATYPE(std::shared_ptr<PlaylistModel>)

#endif // PLAYLISTMODEL_H
