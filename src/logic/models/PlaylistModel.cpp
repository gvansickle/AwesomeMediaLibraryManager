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
/// @file

#include "PlaylistModel.h"

// Qt
#include <QMimeData>
#include <QDebug>
#include <QXmlStreamWriter>

// Ours.
#include "utils/StringHelpers.h"
#include "utils/DebugHelpers.h"
#include "LibraryEntryMimeData.h"
#include "logic/ModelUserRoles.h"


PlaylistModel::PlaylistModel(QObject* parent) : LibraryModel(parent)
{
	setNumberedObjectName(this);
	/// @todo Qt5->6: Default playlist to Sequential/Loop mode.
	//m_qmplaylist->setPlaybackMode(QMediaPlaylist::Sequential);
	m_columnSpecs.push_back({SectionID(PlaylistSectionID::Rating), "Rating", {"rating"}});
	m_columnSpecs.push_back({SectionID(PlaylistSectionID::Blacklist), "Blacklist", {"blacklist"}});
}

QPointer<LibraryModel> PlaylistModel::openFile(QUrl open_url, QObject* parent)
{
	QPointer<PlaylistModel> retval = nullptr;
	/// @todo Call deserializeFromFileXspf() here.
	Q_ASSERT(false);
	return retval;
}

Qt::ItemFlags PlaylistModel::flags(const QModelIndex& index) const
{
	auto defaultFlags = LibraryModel::flags(index);
	if(index.isValid())
	{
		// An existing item.  Allow it to be dragged, alow drops onto it.
		return defaultFlags | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
	}
	else
	{
		// Not an item (probably top level/blank area).  Only makes sense to allow drops.
		return defaultFlags | Qt::ItemIsDropEnabled;
	}
}

QVariant PlaylistModel::data(const QModelIndex& index, int role) const
{
	if(!index.isValid())
	{
		if(role == Qt::UserRole)
		{
			// Global UserRole override for accessing model metadata.
			return QVariant(getLibraryName());
		}
		else
		{
			return QVariant();
		}
	}

	if(role == ModelUserRoles::PointerToItemRole)
	{
		if(index.column() == 0)
		{
			// Return a pointer to the item.
			std::shared_ptr<PlaylistModelItem> item = std::dynamic_pointer_cast<PlaylistModelItem>(getItem(index));
			qDebug() << "Returning pointer to PlaylistModelItem with Url:" << item->getM2Url();
			return QVariant::fromValue<std::shared_ptr<PlaylistModelItem>>(item);
		}
	}

	auto sectionid = getSectionFromCol(index.column());
	if(sectionid >= SectionID::PLAYLIST_1)
	{
		std::shared_ptr<PlaylistModelItem> item = std::dynamic_pointer_cast<PlaylistModelItem>(getItem(index));
		switch(role)
		{
		case Qt::DisplayRole:
			if(sectionid == PlaylistSectionID::Rating)
			{
				///@todo return QVariant::fromValue(Rating(item->m_user_rating));
				return QVariant(item->m_user_rating);
			}
			else if(sectionid == PlaylistSectionID::Blacklist)
			{
				return QVariant(item->m_is_blacklisted ? "Yes" : "No");
			}
			break;
		case Qt::ToolTipRole:
		{
			QString tttext;
			switch(sectionid)
			{
			case PlaylistSectionID::Rating:
				break;
			case PlaylistSectionID::Blacklist:
				if(item->m_is_blacklisted)
				{
					tttext = "<b>This song is blacklisted</b><hr>The user never wants to hear this track ever again";
				}
				else
				{
					tttext = "<b>This song is not blacklisted</b><hr>The user wants to hear this track";
				}
				return tttext;
				break;
			default:
				break;
			}
			break;
		}
		case Qt::EditRole:
		{
			switch(sectionid)
			{
			case PlaylistSectionID::Rating:
				qDebug() << "EDITROLE/RATING";
				break;
			default:
				break;
			}
			break;
		}
		default:
		{
			break;
		}
		}
	}

	// Else we didn't handle it.  Send the request to the base class.
	return LibraryModel::data(index, role);
}

std::shared_ptr<LibraryEntry> PlaylistModel::createDefaultConstructedEntry() const
{
	return std::dynamic_pointer_cast<LibraryEntry>(std::make_shared<PlaylistModelItem>());
}

std::shared_ptr<LibraryEntry> PlaylistModel::getItem(const QModelIndex& index) const
{
	auto libmodel_item = LibraryModel::getItem(index);
	Q_ASSERT(libmodel_item != nullptr);
	auto playlist_model_item = std::dynamic_pointer_cast<PlaylistModelItem>(libmodel_item);
	if(!playlist_model_item)
	{
		qCritical() << "libmodel_item not a PlaylistModelItem";
	}

	Q_ASSERT(playlist_model_item != nullptr);
	return std::static_pointer_cast<LibraryEntry>(playlist_model_item);
}

bool PlaylistModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	// Only needed because we can't put a derived class pointer into a QVariant and get a base class pointer out.

	qDebug() << "index/value/role:" << index << value << role;

	// Has to be a valid index or the call doesn't make sense.
	if(!index.isValid())
	{
		qCritical() << "SET DATA CALLED WITH AN INVALID INDEX";
		return false;
	}

	// The stock view widgets react only to dataChanged with the DisplayRole.
	// When they edit the data, they call setData with the EditRole.
	if(role != Qt::EditRole && role != ModelUserRoles::PointerToItemRole)
	{
		qDebug() << "NOT Qt::EditRole or ModelUserRoles::PointerToItemRole";
		return false;
	}

	// Currently we only support setData() on the first column.
	if(index.column() != 0 || index.row() < 0)
	{
		qWarning() << "RETURNING FALSE: setData() called with index: valid=" << index.isValid() << ", row=" << index.row() << ", column=" << index.column() << ", parent=" << index.parent();
		return false;
	}

	// Tell anybody that's listening that all data in this row has changed.
	QModelIndex bottom_right_index = index.sibling(index.row(), columnCount() - 1);

	auto retval = false;

	if (value.canConvert<std::shared_ptr<PlaylistModelItem>>())
	{
		std::shared_ptr<LibraryEntry> replacement_item = value.value<std::shared_ptr<PlaylistModelItem>>();
		QVariant casted_value = QVariant::fromValue(replacement_item);
		retval = LibraryModel::setData(index, casted_value, role);
		Q_EMIT dataChanged(index, bottom_right_index, {role});
		return retval;
	}
	else
	{
		qCritical() << "CANT CONVERT to PlaylistModelItem*:" << value;
		//Q_ASSERT(0);
	}

	/// @TODO: Is this even a valid thing to try to do here?
	qDebug() << "PUNTING TO BASE CLASS";

	retval = LibraryModel::setData(index, value, role);

	Q_EMIT dataChanged(index, bottom_right_index, {role});

	return retval;
}

QStringList PlaylistModel::mimeTypes() const
{
	// The MIME types we can accept on a Drop.
	return QStringList({"application/x-grvs-libraryentryref"});
}

Qt::DropActions PlaylistModel::supportedDragActions() const
{
	return Qt::CopyAction | Qt::MoveAction;
}

Qt::DropActions PlaylistModel::supportedDropActions() const
{
	return Qt::CopyAction | Qt::MoveAction;
}

bool PlaylistModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
	// We don't really need to override this function.  QAbstractItemModel handles what's needed as long as we
	// have mimeTypes() and supportedDropActions() overridden correctly:
	// "default implementation only checks if data has at least one format in the list of mimeTypes() and if action is among the model's supportedDropActions()."

	// ...actually we would like to reject copy drops to ourself in here, but we don't have enough info.  Specifically,
	// we don't have the source of the dragged item.
	return QAbstractItemModel::canDropMimeData(data, action, row, column, parent);
}

bool PlaylistModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    /// @brief Drag and Drop shouldn't be this hard.
    /// To get drag and drop functioning completely correctly requires a lot of completely unintuitive and undocumented work.
    /// For example, Qt5's own QTreeWidget:
    ///    https://github.com/qt/qtbase/blob/5.10/src/widgets/itemviews/qtreewidget.cpp
    /// Examine the dance that QTreeWidget and its "private" model QTreeModel have to do for drag and drop.
    /// One thing you'll notice is that QTreeModel::dropMimeData() *does not perform the drop of the MimeData*.
    /// All it does adjust the drop row if it's a drop to {-1,-1}, then it calls *the view's* dropMimeData() function,
    /// which then gets the drop-parent item's index and then calls down to QAbstractItemModel::dropMimeData(), which
	/// then (hopefully) does the actual drop.  This if course completely breaks the notion of the model and view being independent entities.
	///
	/// Note that the  QAbstractItemModel::dropMimeData() implementation looks like it's not what we need.
	/// It ultimately calls QAbstractItemModel::decodeData() and deserializes a QDataStream coming from the QMimeData,
	/// and it does a ton of work to get every row and column separately entered into the model.

	/**
	 *  Also, Per @link http://www.qtcentre.org/threads/5910-QTreeWidget-Drag-and-drop:
	 * "For QTreeWidget derived class dropMimeData() gets called only when there is a "data drop", so to say.
	 *  For move operations, it does not get called. It gets called when you try to copy the data.
	 *  For Move or Internal move operations QTreeWidget::dropEvent() gets called."
	 */

	// Per example code here: http://doc.qt.io/qt-5/model-view-programming.html#using-drag-and-drop-with-item-views, "Inserting dropped data into a model".
	// "The model first has to make sure that the operation should be acted on,
	// the data supplied is in a format that can be used,
	// and that its destination within the model is valid"
	if(!canDropMimeData(data, action, row, column, parent))
	{
		// Not a format we can use.
		return false;
	}
	if (action == Qt::IgnoreAction)
	{
		// Not sure why we would be told to ignore the drop, but we've been told to ignore the drop, so ignore it.
        // Return true here because the drop was "handled" by the model.
		return true;
	}

	// "The data to be inserted into the model is treated differently depending on whether it is dropped onto an existing
	// item or not. In this simple example, we want to allow drops between existing items, before the first item in the
	// list, and after the last item.
	// When a drop occurs, the model index corresponding to the parent item will either be valid, indicating that the
	// drop occurred on an item, or it will be invalid, indicating that the drop occurred somewhere in the view that
	// corresponds to top level of the model."

	// "We initially examine the row number supplied to see if we can use it to insert items into the model,
	// regardless of whether the parent index is valid or not."
	int beginRow;

	if(row != -1)
	{
		// Valid row number.
		beginRow = row;
	}
	else if(parent.isValid())
	{
		// Valid parent, drop occurred on an item.
		// "If the parent model index is valid, the drop occurred on an item. In this simple list model, we find out
		// the row number of the item and use that value to insert dropped items into the top level of the model."
		beginRow = parent.row();
	}
	else
	{
		// Must have been a drop on the top level of the model.
		// "When a drop occurs elsewhere in the view, and the row number is unusable, we append items to the top level
		// of the model."
		beginRow = rowCount(QModelIndex());
	}

	// "In hierarchical models, when a drop occurs on an item, it would be better to insert new items into the model as
	// children of that item. In the simple example shown here, the model only has one level, so this approach is not
	// appropriate."

    // Now unpack the data from the QMimeData and put in into the model.

    /// @note From here: https://stackoverflow.com/questions/23388612/drag-and-drop-in-qtreeview-removerows-not-called
    /// "Short answer: If everything is correctly configured, the target should not remove the source item, the
    /// initiator of the drag should remove the source item if a Qt::MoveAction is performed."
    /// So if we're in the model here with a Qt::MoveAction, we shouldn't need to do the remove.

	auto libentries = qobject_cast<const LibraryEntryMimeData*>(data)->m_lib_item_list;
	const auto rows = libentries.size();
	const auto startRow = beginRow;

	// Insert the default-constructed rows.
	insertRows(beginRow, rows, QModelIndex());

	bool success = false;
	if(action == Qt::CopyAction)
	{
		for(const auto& libentry : std::as_const(libentries))
		{
			qDebug() << "Inserting Copies";
			// Create a new PlaylistModelItem to put in the model.
			std::shared_ptr<PlaylistModelItem> plmi = PlaylistModelItem::createFromLibraryEntry(libentry);

			setData(index(beginRow, 0), QVariant::fromValue(plmi));
			beginRow += 1;
		}
		qDebug() << QString("Inserted and setData");
		// Drop was successful.
		success = true;
	}
	else if(action == Qt::MoveAction)
	{
        qDebug() << "MoveAction START";
            for(const auto& libentry: std::as_const(libentries))
            {
                    qDebug() << "Moving";
                    // The dropped libentries should actually be PlaylistEntries.
                    std::shared_ptr<PlaylistModelItem> plmi = std::dynamic_pointer_cast<PlaylistModelItem>(libentry);
                    Q_ASSERT(plmi != nullptr);
                    setData(index(beginRow, 0), QVariant::fromValue(plmi));
                    beginRow += 1;
            }
        qDebug() << "MoveAction END";
		success = true;
	}

	if (success)
	{
		// We've successfully inserted the data.  Tell the view to update.
		Q_EMIT dataChanged(index(startRow, 0), index(startRow + rows - 1, 0));
	}

	return success;
}

void PlaylistModel::setLibraryRootUrl(const QUrl &url)
{
    beginResetModel();

    // Give the library a starting point.
    m_library.setRootUrl(url);

// M_WARNING("TODO: This is here to stop the model populator thread from starting, and we don't need a cache file.")
//    // Create a cache file for this Library.
//    createCacheFile(url);

//    connectSignals();
//    emit statusSignal(LibState::ScanningForFiles, 0, 0);
//    emit startFileScanSignal(m_library->rootURL);

    endResetModel();
}

void writeXspfMetaElement(QXmlStreamWriter& stream, QAnyStringView key, QAnyStringView value)
{
	stream.writeStartElement("meta");
	stream.writeAttribute("rel", key);
	stream.writeCharacters(value);
	stream.writeEndElement();
}

template<typename T>
requires std::integral<T>
void writeXspfMetaElement(QXmlStreamWriter& stream, QAnyStringView key, T value)
{
	auto value_as_qstr = QString::number(value);
	writeXspfMetaElement(stream, key, value_as_qstr);
}

/**
 * @page xspf Notes on XSPF playlist support.
 * - Audacious
 *   - Decent support of multi-track flac files.
 *   - Uses a lot of <meta> to capture subtrack info (they call them "subsongs").
 *   - Example:
\code{.xml}
<?xml version="1.0" encoding="UTF-8"?>
<playlist version="1" xmlns="http://xspf.org/ns/0/">
  <title>GRVSPlaylist1</title>
  <trackList>
  <!-- ..... -->
	<track>
	  <location>file:///run/user/[...]/Wham%21/1984%20-%20Make%20It%20Big/Wham%21%20-%20Make%20It%20Big.cue?7</location>
	  <title>Credit Card Baby</title>
	  <creator>Wham!</creator>
	  <album>Make It Big</album>
	  <meta rel="album-artist">Wham!</meta>
	  <annotation>CUERipper v2.1.6 Copyright (C) 2008-13 Grigory Chudov</annotation>
	  <meta rel="year">1984</meta>
	  <trackNum>7</trackNum>
	  <duration>309934</duration>
	  <meta rel="bitrate">789</meta>
	  <meta rel="codec">Free Lossless Audio Codec (FLAC)</meta>
	  <meta rel="quality">lossless</meta>
	  <meta rel="audio-file">file:///run/user/[...]/Wham%21/1984%20-%20Make%20It%20Big/Wham%21%20-%20Make%20It%20Big.flac</meta>
	  <meta rel="subsong-id">7</meta>    <!-- NOTE this is the ".cue?7" at the end of <location> above -->
	  <meta rel="seg-start">1598226</meta>
	  <meta rel="seg-end">1908160</meta>
	</track>
	<track>
	  <location>file:///run/user/[...]/Wham%21/1984%20-%20Make%20It%20Big/Wham%21%20-%20Make%20It%20Big.cue?8</location>
	  <title>Careless Whisper</title>
	  <creator>Wham!</creator>
	  <album>Make It Big</album>
	  <meta rel="album-artist">Wham!</meta>
	  <annotation>CUERipper v2.1.6 Copyright (C) 2008-13 Grigory Chudov</annotation>
	  <meta rel="year">1984</meta>
	  <trackNum>8</trackNum>
	  <duration>391840</duration>
	  <meta rel="bitrate">789</meta>
	  <meta rel="codec">Free Lossless Audio Codec (FLAC)</meta>
	  <meta rel="quality">lossless</meta>
	  <meta rel="audio-file">file:///run/user/[...]/Wham%21/1984%20-%20Make%20It%20Big/Wham%21%20-%20Make%20It%20Big.flac</meta>
	  <meta rel="subsong-id">8</meta>
	  <meta rel="seg-start">1908160</meta>  <!-- NOTE missing seg-end on last track -->
	</track>
  </trackList>
</playlist>
\endcode
 * - Amarok
 *   - Doesn't support multi-track flac files, simply considers them one long song.
 *   - Example:
\code{.xml}
<?xml version="1.0" encoding="UTF-8"?>
<playlist version="1" xmlns="http://xspf.org/ns/0/">
  <trackList>
	<track>
	  <location>../../run/user/[...different from Audacious ...]/CDRips/Boy George/1993 - At Worst… The Best of Boy George and Culture Club/Boy George - At Worst… The Best of Boy George and Culture Club.flac</location>
	  <identifier>amarok-sqltrackuid://eebf97d1ebb8702e5e6750059dcee579</identifier>
	  <title>At Worst… The Best of Boy George and Culture Club</title>
	  <creator>Boy George</creator>
	  <annotation>CUERipper v2.1.5 Copyright (C) 2008-13 Grigory Chudov</annotation>
	  <album>At Worst… The Best of Boy George and Culture Club</album>
	  <duration>4510827</duration>
	</track>
  </trackList>
  <extension application="http://amarok.kde.org">
	<queue/>
  </extension>
</playlist>
\endcode
 *
 */
bool PlaylistModel::serializeToFileAsXSPF(QFileDevice& filedev) const
{
	QXmlStreamWriter stream(&filedev);

	stream.setAutoFormatting(true);
	stream.writeStartDocument();
	// <playlist version="1" xmlns="http://xspf.org/ns/0/">
	stream.writeStartElement("http://xspf.org/ns/0/", "playlist");
	stream.writeAttribute("version", "1");

	/// @todo Add Playlist metadata here.
	/// http://www.xspf.org/xspf-v1.html#rfc.section.2.3.1
	/// <title> "A human-readable title for the playlist. xspf:playlist elements MAY contain exactly one."
	/// <creator> "Human-readable name of the entity (author, authors, group, company, etc) that authored the playlist. xspf:playlist elements MAY contain exactly one."
	/// ...
	/// <date>	"Creation date (not last-modified date) of the playlist, formatted as a XML schema dateTime. xspf:playlist elements MAY contain exactly one.
	///	A sample date is "2005-01-08T17:10:47-05:00".

	stream.writeStartElement("trackList");
	// Write the tracks.
	for(qint64 row = 0; row < rowCount(); ++row)
	{
		QModelIndex mi = index(row, 0, QModelIndex());
		std::shared_ptr<PlaylistModelItem> pmi = std::dynamic_pointer_cast<PlaylistModelItem>(getItem(mi));
		Q_ASSERT(pmi != nullptr);
		stream.writeStartElement("track");
			// Location
			// "URI of resource to be rendered. Probably an audio resource, but MAY be any type of resource with a well-known duration, such as video,
			// a SMIL document, or an XSPF document. The duration of the resource defined in this element defines the duration of rendering. xspf:track
			// elements MAY contain zero or more location elements, but a user-agent MUST NOT render more than one of the named resources.
			stream.writeTextElement("location", pmi->getUrl().toString());
			stream.writeTextElement("title", toqstr(pmi->metadata()["track_name"]));
			stream.writeTextElement("creator", "");
			stream.writeTextElement("album", toqstr(pmi->metadata()["album_name"]));
			stream.writeTextElement("duration", std::to_string(static_cast<double>(pmi->get_length_secs())*1000.0));
			stream.writeTextElement("trackNum", std::to_string(pmi->getTrackNumber()));
			stream.writeTextElement("image", "");
			if(pmi->isSubtrack() /** @todo & PlaylistSubformat == Audacious */)
			{
				// Subtrack metadata.
				/// @todo Need to get the "rel=" into attributes.
				writeXspfMetaElement(stream, "subsong-id", "??subsong-id??");
				writeXspfMetaElement(stream, "seg-start", FramesToMilliseconds(pmi->get_offset_frames()));
				writeXspfMetaElement(stream, "seg-end", FramesToMilliseconds(pmi->get_offset_frames() + pmi->get_length_frames()));
			}
		stream.writeEndElement(); // track
	}
	stream.writeEndDocument();

	return true;
}

bool PlaylistModel::deserializeFromFileAsXSPF(QFileDevice& filedev)
{
	Q_UNIMPLEMENTED();
	Q_ASSERT(0);
}


