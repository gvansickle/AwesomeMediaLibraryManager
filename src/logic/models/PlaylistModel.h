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

/// @file

#include "LibraryModel.h"
#include "PlaylistModelItem.h"

#include <memory>

class QMediaPlaylist;


class PlaylistSectionID : public SectionID
{
	Q_GADGET

public:
	enum Enumerator
	{
		Rating = int(SectionID::PLAYLIST_1) + 0,
		Blacklist
	};

	Q_ENUM(Enumerator)

	PlaylistSectionID() = default;
	PlaylistSectionID(PlaylistSectionID::Enumerator e) { m_val = e; }
	operator int() const { return m_val; }
//	operator SectionID() { return SectionID(m_val); }
};


class PlaylistModel : public LibraryModel
{
	Q_OBJECT

public:
	explicit PlaylistModel(QObject* parent);
    ~PlaylistModel() override = default;
	M_GH_POLYMORPHIC_SUPPRESS_COPYING_C67(PlaylistModel)

	/**
	 * Open a new LibraryModel on the specified QUrl.
	 */
	static QPointer<LibraryModel> openFile(QUrl open_url, QObject* parent);

	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant data(const QModelIndex &index, int role) const override;

	/// Override this in derived classes to return a newly-allocated, default-constructed instance
	/// of an entry derived from LibraryEntry.  Used by insertRows().
	std::shared_ptr<LibraryEntry> createDefaultConstructedEntry() const override;

	std::shared_ptr<LibraryEntry> getItem(const QModelIndex& index) const override;

	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

	/// @name Drag and drop support.
	/// @{

	QStringList mimeTypes() const override;
	Qt::DropActions supportedDragActions() const override;
	Qt::DropActions supportedDropActions() const override;
	bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;

	/**
	 * @todo Per @link http://www.qtcentre.org/threads/5910-QTreeWidget-Drag-and-drop:
	 * "For QTreeWidget derived class dropMimeData() gets called only when there is a "data drop", so to say.
	 *  For move operations, it does not get called. It gets called when you try to copy the data.
	 *  For Move or Internal move operations QTreeWidget::dropEvent() gets called."
	 */
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

	/// @}

	void setLibraryRootUrl(const QUrl& url) override;

	bool serializeToFileAsXSPF(QFileDevice& filedev) const;
	bool deserializeFromFileAsXSPF(QFileDevice& filedev); /// @todo Implement
};

Q_DECLARE_METATYPE(PlaylistModel)
Q_DECLARE_METATYPE(const PlaylistModel*)
Q_DECLARE_METATYPE(std::shared_ptr<PlaylistModel>)

#endif // PLAYLISTMODEL_H
