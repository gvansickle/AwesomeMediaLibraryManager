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

#include "LibraryEntryMimeData.h"
#include "PlaylistModel.h"

#include <QMediaPlaylist>
#include <QMimeData>
#include <QDebug>
#include <QXmlStreamWriter>

#include "utils/StringHelpers.h"
#include "utils/DebugHelpers.h"

#include "logic/ModelUserRoles.h"


PlaylistModel::PlaylistModel(QObject* parent) : LibraryModel(parent)
{
	// The QMediaPlaylist we need for QMediaPlayer's consumption.
	m_qmplaylist = new QMediaPlaylist(this);
	m_qmplaylist->setPlaybackMode(QMediaPlaylist::Sequential);
	m_columnSpecs.push_back({SectionID(PlaylistSectionID::Rating), "Rating", {"rating"}});
	m_columnSpecs.push_back({SectionID(PlaylistSectionID::Blacklist), "Blacklist", {"blacklist"}});
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
			qDebug() << "Returning pointer to item with Url:" << item->getUrl();
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

	if(value.canConvert<std::shared_ptr<PlaylistModelItem>>())
	{
		qDebug() << "Can convert to PlaylistModelItem*: true";

		std::shared_ptr<LibraryEntry> replacement_item = value.value<std::shared_ptr<PlaylistModelItem>>();
		QVariant casted_value = QVariant::fromValue(replacement_item);
		return LibraryModel::setData(index, casted_value, role);
	}
	else
	{
		qCritical() << "CANT CONVERT:" << value;
		//Q_ASSERT(0);
	}
	qDebug() << "PUNTING TO BASE CLASS";
	return LibraryModel::setData(index, value, role);
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
    /// when then gets the drop-parent item's index and then calls down to QAbstractItemModel::dropMimeData(), which
	/// then (hopefully) does the actual drop.  This if course completely breaks the notion of the model and view being independent entities.
	///
	/// Note that the  QAbstractItemModel::dropMimeData() implementation looks like it's not what we need.
	/// It ultimately calls QAbstractItemModel::decodeData() and deserializes a QDataStream coming from the QMimeData,
	/// and it does a ton of work to get every row and column separately entered into the model.

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
	auto rows = libentries.size();

	insertRows(beginRow, rows, QModelIndex());
	if(action == Qt::CopyAction)
	{
		for(auto libentry : libentries)
		{
			qDebug() << "Inserting Copies";
			// Create a new PlaylistModelItem to put in the model.
			std::shared_ptr<PlaylistModelItem> plmi = PlaylistModelItem::createFromLibraryEntry(libentry);
			setData(index(beginRow, 0), QVariant::fromValue(plmi));
			beginRow += 1;
		}
		qDebug() << QString("Inserted and setData");
		// Drop was successful.
		return true;
	}
	else if(action == Qt::MoveAction)
	{
        qDebug() << "MoveAction START";
            for(auto libentry : libentries)
            {
                    qDebug() << "Moving";
                    // The dropped libentries should actually be PlaylistEntries.
                    std::shared_ptr<PlaylistModelItem> plmi = std::dynamic_pointer_cast<PlaylistModelItem>(libentry);
                    Q_ASSERT(plmi != 0);
                    setData(index(beginRow, 0), QVariant::fromValue(plmi));
                    beginRow += 1;
            }
        qDebug() << "MoveAction END";
		return true;
	}
	return false;

}

void PlaylistModel::setLibraryRootUrl(const QUrl &url)
{
    beginResetModel();

    // Give the library a starting point.
    m_library.setRootUrl(url);

M_WARNING("TODO: This is here to stop the model populator thread from starting, and we don't need a cache file.")
//    // Create a cache file for this Library.
//    createCacheFile(url);

//    connectSignals();
//    emit statusSignal(LibState::ScanningForFiles, 0, 0);
//    emit startFileScanSignal(m_library->rootURL);

    endResetModel();
}

QMediaPlaylist* PlaylistModel::qmplaylist()
{
	return m_qmplaylist;
}

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
			stream.writeTextElement("duration", "");
			stream.writeTextElement("trackNum", "");
			stream.writeTextElement("image", "");
		stream.writeEndElement(); // track
	}
	stream.writeEndDocument();

	return true;

}

void PlaylistModel::onRowsInserted(QModelIndex parent, int first, int last)
{
	qDebug() << QString("Inserting rows %1 - %2 into QMPlaylist, mediaCount=%3").arg(first).arg(last).arg(m_qmplaylist->mediaCount());
	for(auto i = first; i<last+1; ++i)
	{
		auto child_index = index(i, 0, QModelIndex());
		std::shared_ptr<PlaylistModelItem> playlist_entry = std::dynamic_pointer_cast<PlaylistModelItem>(getItem(child_index));
		QMediaContent* qmediacontent = new QMediaContent(playlist_entry->getM2Url());
		if(!m_qmplaylist->insertMedia(i, *qmediacontent))
		{
			qFatal("Insertion failed: %s", m_qmplaylist->errorString().toStdString().c_str());
		}
	}
	qDebug() << "complete";
}

void PlaylistModel::onRowsRemoved(QModelIndex parent, int first, int last)
{
	qDebug() << QString("Removing rows %1 through %2 from QMPlaylist").arg(first).arg(last);
	if(!m_qmplaylist->removeMedia(first, last))
	{
		qFatal("Insertion failed: %s", m_qmplaylist->errorString().toStdString().c_str());
	}
	qDebug("complete");
}

void PlaylistModel::onSetData(QModelIndex index, QVariant value, int role)
{
//	qDebug() << "onSetData()" << index << Qt::ItemDataRole(role);

	// QMediaPlaylist has no analog to setData(), so we have to remove and insert here.

	qDebug() << QString("Replacing row %1 of QMPlaylist, mediaCount=%2").arg(index.row()).arg(m_qmplaylist->mediaCount());
	if(m_qmplaylist->mediaCount() < index.row()+1)
	{
		qFatal("no such row in QMediaPlaylist: %d", index.row());
		return;
	}
	if(!index.isValid())
	{
		qFatal("Invalid index");
	}
	if(!m_qmplaylist->removeMedia(index.row()))
	{
		qFatal("Replace-remove failed:");/// << QString("Replace-remove failed:") << m_qmplaylist->errorString();
	}

	Q_ASSERT(value.canConvert<std::shared_ptr<LibraryEntry>>() == true);

	std::shared_ptr<PlaylistModelItem> playlist_entry = PlaylistModelItem::createFromLibraryEntry(value.value<std::shared_ptr<LibraryEntry>>());
	QMediaContent* qmediacontent = new QMediaContent(playlist_entry->getM2Url());
	if(!m_qmplaylist->insertMedia(index.row(), *qmediacontent))
	{
		qFatal("Replace-insert failed: %s", m_qmplaylist->errorString().toStdString().c_str());
	}
	qDebug("QMediaPlaylist replace-insert complete");
}

