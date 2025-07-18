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

/** @file LibraryModel.cpp */

#include "LibraryModel.h"

// Stc C++
#include <vector>
#include <memory>

// Qt
#include <QFileDevice>
#include <QStandardPaths>
#include <QIcon>
#include <QDebug>
#include <QTemporaryFile>
#include <QDir>
#include <QFileIconProvider>

// Ours
#include <AMLMApp.h>
#include <utils/RegisterQtMetatypes.h>
#include "logic/LibraryRescanner.h"
#include "logic/LibraryRescannerMapItem.h"
#include "LibraryEntryMimeData.h"
#include "utils/StringHelpers.h"
#include "utils/DebugHelpers.h"
#include "logic/Library.h"
#include "logic/ModelUserRoles.h"
#include <logic/PerfectDeleter.h>

#include <gui/Theme.h>
#include <logic/jobs/LibraryEntryLoaderJob.h>
#include <logic/jobs/LibraryRescannerJob.h>
#include <logic/serialization/SerializationHelpers.h>


AMLM_QREG_CALLBACK([](){
    qIn() << "Registering LibraryModel types";
	qRegisterMetaType<LibraryModel>();
    qRegisterMetaType<VecOfUrls>();
//    qRegisterMetaType<VecOfLEs>();
	qRegisterMetaType<std::vector<std::shared_ptr<LibraryEntry>>>("VecOfLEs");
    qRegisterMetaType<VecOfPMIs>();
    });


LibraryModel::LibraryModel(QObject *parent) : QAbstractItemModel(parent)
{
	// App-specific cache directory.
	m_cachedir = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
	// Make sure it ends in a "/".
	if(!m_cachedir.toLocalFile().endsWith("/"))
	{
		m_cachedir = QUrl::fromLocalFile(m_cachedir.toLocalFile()+"/");
	}
	qDebug() << "Cachedir:" << m_cachedir;

	// Set up the columns.
	m_columnSpecs.clear();
    m_columnSpecs.push_back({SectionID::Status, "?", QStringList("status_icon"), true});
	m_columnSpecs.push_back({SectionID::Title, "Title", QStringList("track_name")});
	m_columnSpecs.push_back({SectionID::Artist, "Artist", QStringList({"track_performer", "track_artist", "album_artist"})});
	m_columnSpecs.push_back({SectionID::Album, "Album", QStringList("album_name")});
	m_columnSpecs.push_back({SectionID::Length, "Length", {"length"}, true});
	m_columnSpecs.push_back({SectionID::MIMEType, "Type", {"filetype"}, true});
	m_columnSpecs.push_back({SectionID::Filename, "Filename", {"filename"}});

	// Pre-fabbed Icons
	m_IconError = QVariant(QIcon::fromTheme("dialog-error"));
	m_IconOk = QVariant(QIcon::fromTheme("audio-x-generic"));
	m_IconUnknown = QVariant(QIcon::fromTheme("dialog-question"));

	// Create the asynchronous rescanner.
	m_rescanner = new LibraryRescanner(this);

	// Connections.
}

LibraryModel::~LibraryModel()
{
//    delete m_rescanner;
    m_rescanner->deleteLater();
}

// static
QPointer<LibraryModel> LibraryModel::openFile(QUrl open_url, QObject* parent)
{
    // Create the new LibraryModel.
	auto lib = QPointer<LibraryModel>(new LibraryModel(parent));

// M_MESSAGE("TODO: Find a better way to start async operations and/or connect");
    lib->setLibraryRootUrl(open_url);

    return lib;
}

QModelIndex LibraryModel::index(int row, int column, const QModelIndex &parent) const
{
	if(!parent.isValid())
	{
		if((!m_library.m_lib_entries.empty())
				&& (row >= 0 && row < rowCount())
				&& (column >= 0 && column < static_cast<int>(m_columnSpecs.size())))
		{
			return createIndex(row, column, m_library.m_lib_entries[row].get());
		}
	}
	//logger.warning("Returning invalid index: {}/{}/{}".format(row, column, parent))
	return QModelIndex();
}

QModelIndex LibraryModel::parent(const QModelIndex &index) const
{
	// No index has a parent.
	(void)index;
	return QModelIndex();
}

QModelIndex LibraryModel::sibling(int row, int column, const QModelIndex& idx) const
{
//	qDebug() << "sibling called" << row << column << idx;
	return QAbstractItemModel::sibling(row, column, idx);
}

QSize LibraryModel::span(const QModelIndex& index) const
{
	qDebug() << "SPAN CALLED";
	return QAbstractItemModel::span(index);
}

int LibraryModel::rowCount(const QModelIndex &parent) const
{
	if(!parent.isValid())
	{
		return m_library.m_lib_entries.size();
	}
	return 0;
}

int LibraryModel::columnCount(const QModelIndex &parent) const
{
	if(!parent.isValid())
	{
		return m_columnSpecs.size();
	}
	return 0;
}

Qt::ItemFlags LibraryModel::flags(const QModelIndex &index) const
{
	auto defaultFlags = this->QAbstractItemModel::flags(index);
	if(index.isValid())
	{
		return defaultFlags | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsDragEnabled;
	}
	else
	{
        return Qt::NoItemFlags;
	}
}

QVariant LibraryModel::data(const QModelIndex &index, int role) const
{
	/// @warn Per https://doc.qt.io/qt-6/qabstractitemmodel.html#multiData
	/// "Note: It is illegal to pass an invalid model index to [multiData()]."

	// Handle invalid indexes.
	if (!index.isValid())
	{
		if (role == Qt::UserRole)
		{
			// Global UserRole override for accessing model metadata.
			return getLibraryName();
		}
		else
		{
			return QVariant();
		}
	}

	QModelRoleData roleData(role);
	multiData(index, roleData);
	return roleData.data();
}

void LibraryModel::multiData(const QModelIndex& index, QModelRoleDataSpan roleDataSpan) const
{
	/// @warn Per https://doc.qt.io/qt-6/qabstractitemmodel.html#multiData
	/// "Note: It is illegal to pass an invalid model index to [multiData()]."
	/// So I'm thinking we should really be bailing out and returning here.
	/// But, our logic below actually handles (!index.isValid && role==Qt::UserRole).
	// AMLM_WARNIF_NOT(checkIndex(index, CheckIndexOption::IndexIsValid));
	Q_ASSERT(checkIndex(index, CheckIndexOption::IndexIsValid));

	// Get data for each role at the given index.
	for (QModelRoleData& roleData : roleDataSpan)
	{
		int role = roleData.role();

	    // Handle invalid indexes.
		if(!index.isValid())
		{
			if(role == Qt::UserRole)
			{
				// Global UserRole override for accessing model metadata.
				roleData.setData(getLibraryName());
			}
			else
			{
				roleData.clearData();
			}
			// Go to next role.
			continue;
		}

	    // index is valid.

		// Clear any stale data that is in this QModelRoleData.
		roleData.clearData();

		if (role == ModelUserRoles::PointerToItemRole)
		{
			if(index.column() == 0)
			{
				// Return a pointer to the item.
				std::shared_ptr<LibraryEntry> item = getItem(index);
				qDebug() << "Returning pointer to item with Url:" << item->getUrl();
				roleData.setData(item);
			}
		}

		if (role == Qt::DecorationRole)
		{
			auto sectionid = getSectionFromCol(index.column());
	        if(SectionID::Status == sectionid)
	        {
	            // Return an icon indicating the populated status of this entry.
	            auto item = getItem(index);
	            if(item->isPopulated())
	            {
	                if(item->isError())
	                {
	                    roleData.setData(m_IconError);
	                }
	                else
	                {
	                    roleData.setData(m_IconOk);
					}
	            }
	            else
	            {
	                roleData.setData(m_IconUnknown);
	            }
	        }
	        else if(SectionID::MIMEType == sectionid)
	        {
	            // Return an icon for the MIME type of the file containing the track.
	            auto item = getItem(index);
	            QMimeType mime = item->getMimeType();
	            QIcon mime_icon = Theme::iconFromTheme(mime);
	            // return QVariant::fromValue(mime_icon);
	            roleData.setData(mime_icon);
			}
			else
			{
				continue;
				// return QVariant();
	        }
	    }
		if(role == Qt::DisplayRole || role == Qt::ToolTipRole)
		{
			auto item = getItem(index);
			auto sec_id = getSectionFromCol(index.column());
	        if(item->isPopulated())
			{
	            // Item has data.
				QVariant metaentry;
				if(sec_id == SectionID::Status)
				{
					// We get a flood of requests for the status column for some reason.
					if(role == Qt::ToolTipRole)
					{
						// Return a big tooltip with all the details of the entry.
						roleData.setData(getEntryStatusToolTip(item.get()));
					}
					else if(role == Qt::DisplayRole)
					{
						roleData.setData(item->hasNoPregap() ? "NoGap" : "");
					}
					// Short-circuit the rest of the logic, we have nothing to return here.
					continue;
				}
				else if(sec_id == SectionID::Length)
				{
					// Return Fraction as a string.
					// return QVariant::fromValue(item->get_length_secs());
					roleData.setData(item->get_length_secs());
				}
				else if(sec_id == SectionID::MIMEType)
				{
					roleData.setData(item->getMimeType());
				}
				else if(sec_id == SectionID::Filename)
				{
					// return item->getFilename();
					roleData.setData(item->getFilename());
				}
				else
				{
					// Get the list of metadata entry names which will work for this column's text,
					// in descending order of preference.
					QStringList metadata_choices = m_columnSpecs[index.column()].metadata_list;
	//                qDebug() << "metadata_choices:" << metadata_choices;
					for(QString& key: metadata_choices)
					{
						QStringList metadata_value_str_list = item->getMetadata(key);
	//                    qDb() << "KEY:" << key << "LIST:" << metadata_value_str_list;
						if(!metadata_value_str_list.isEmpty() && !metadata_value_str_list[0].isEmpty())
						{
							//qDebug() << "was valid: (" << metadata_value_str_list[0] << ")";
							metaentry = QVariant::fromValue(metadata_value_str_list[0]);
							break;
						}
					}
				}
				if(!metaentry.isNull() && metaentry.isValid())
				{
					// return QVariant(metaentry);
					roleData.setData(metaentry);
				}
			}
			else
			{
				// Entry hasn't been populated yet.
			}
		}
	}
}

QMap<int, QVariant> LibraryModel::itemData(const QModelIndex& index) const
{
	auto retval = QAbstractItemModel::itemData(index);
	if (index.column() == 0)
	{
		auto vardata = data(index, ModelUserRoles::PointerToItemRole);
		if (vardata.isValid())
		{
			retval.insert(ModelUserRoles::PointerToItemRole, vardata);
		}
	}
	return retval;
}

QHash<int, QByteArray> LibraryModel::roleNames() const
{
	auto retval = QAbstractItemModel::roleNames();

	// Append our user role names.
	for (int i = 0; i < ModelUserRoles::keyCount(); i++)
	{
		//		qDebug() << "ENUM:" << ModelUserRoles::key(i) << "Val:" << ModelUserRoles::value(i);
		retval.insert(ModelUserRoles::value(i), ModelUserRoles::valueToKey(ModelUserRoles::value(i)));
	}
	return retval;
}

QVariant LibraryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_ASSERT(section >= -1);
	if (orientation == Qt::Horizontal)
	{
		switch (role)
		{
		case Qt::DisplayRole:
			{
				if (section + 1 > static_cast<int>(m_columnSpecs.size()))
				{
					return QVariant();
				}
				auto dn = m_columnSpecs[section].m_display_name;
				return QVariant(dn);
			}
		case ModelUserRoles::HeaderViewSectionID:
			{
				return QVariant::fromValue(m_columnSpecs[section].m_section_id);
			}
		case ModelUserRoles::HeaderViewSectionShouldFitWidthToContents:
			{
				return m_columnSpecs[section].m_should_fit_column_width_to_contents;
			}
		default:
			// Punt to base class.
			return BASE_CLASS::headerData(section, orientation, role);
		}
	}

	return QVariant();
}

bool LibraryModel::hasChildren(const QModelIndex& parent) const
{
	if (!parent.isValid())
	{
		// This is the root node, this is the only node with children.
		return true;
	}
	else
	{
		return false;
	}
}

std::shared_ptr<LibraryEntry> LibraryModel::createDefaultConstructedEntry() const
{
	return std::make_shared<LibraryEntry>();
}

std::shared_ptr<LibraryEntry> LibraryModel::getItem(const QModelIndex& index) const
{
	if (index.isValid())
	{
		std::shared_ptr<LibraryEntry> item = m_library[index.row()];
		if (item)
		{
			return item;
		}
		else
		{
			qWro() << "NULL internalPointer, returning 'None' item";
			Q_ASSERT(0);
			return nullptr;
		}
	}
	else
	{
		qWro() << "Invalid index, returning 'None' item";
		return nullptr;
	}
}

bool LibraryModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	//	qDebug() << "SetData, index/value/role:" << index << value << role;

	// Has to be a valid index or the call doesn't make sense.
	if (!index.isValid())
	{
		qCritical() << "SET DATA CALLED WITH AN INVALID INDEX";
		return false;
	}

	// The stock view widgets react only to dataChanged with the DisplayRole.
	// When they edit the data, they call setData with the EditRole.
	if (role != Qt::EditRole && role != ModelUserRoles::PointerToItemRole)
	{
		qDebug() << "NOT Qt::EditRole or ModelUserRoles::PointerToItemRole";
		return false;
	}

	// Currently we only support setData() on the first column.
	if (index.column() != 0 || index.row() < 0)
	{
		qWarning() << "RETURNING FALSE: setData() called with index: valid=" << index.isValid() << ", row=" << index.
			row() << ", column=" << index.column() << ", parent=" << index.parent();
		return false;
	}

	std::shared_ptr<LibraryEntry> replacement_item;

	//	if(role == ModelUserRoles::PointerToItemRole)
	//	{
	// Set a std::shared_ptr<> to the item at this index/role.
	replacement_item = value.value<std::shared_ptr<LibraryEntry>>();
	Q_ASSERT(replacement_item);
	//		qDebug() << "Setting pointer to item with Url:" << replacement_item->getUrl();
	//	}

	///qDebug() << "Can convert to LibraryEntry*:" << value.canConvert<LibraryEntry*>();

	Q_ASSERT(replacement_item);

	m_library.replaceEntry(index.row(), replacement_item);

	// Tell anybody that's listening that all data in this row has changed.
	QModelIndex bottom_right_index = index.sibling(index.row(), columnCount() - 1);
	//	qDebug() << "EMITTING DATACHANGED:" << index << index.parent() << bottom_right_index << bottom_right_index.parent() << Qt::ItemDataRole(role);
	Q_EMIT dataChanged(index, bottom_right_index, {role});
	return true;
}


bool LibraryModel::insertRows(int row, int count, const QModelIndex& parent)
{
	// Insert a default-constructed row into the model.
	qDebug() << "INSERTING ROWS" << row << "to" << row + count - 1 << "UNDER PARENT:" << parent;
	if (parent.isValid())
	{
		qDebug() << "PARENT IS VALID, NOT INSERTING";
		return false;
	}

	beginInsertRows(parent, row, row + count - 1);

	// Add new default-constructed entries.
	for (int i = row; i < row + count; ++i)
	{
		auto default_entry = createDefaultConstructedEntry();
		m_library.insertEntry(i, default_entry);
	}
qDb() << "INSERT END";
	endInsertRows();

	return true;
}

bool LibraryModel::removeRows(int row, int count, const QModelIndex& parent)
{
	qDebug() << "REMOVING" << count << "ROWS STARTING AT ROW" << row << ", PARENT:" << parent;
	if (parent.isValid())
	{
		qWarning() << "PARENT IS VALID, NOT REMOVING";
		return false;
	}

	if (row < 0)
	{
		qCritical() << "ROW WAS < 0";
		return false;
	}

	beginRemoveRows(parent, row, row + count - 1);

	/// @note There's a QSignalBlocker() here in the model for: https://github.com/qt/qtbase/blob/5.10/src/widgets/itemviews/qtreewidget.cpp
	/// Not clear why, doesn't seem like we need it.

	// Remove the indicated rows from the underlying Library.
	m_library.removeRows(row, count);

	endRemoveRows();
	return true;
}

void LibraryModel::appendRow(std::shared_ptr<LibraryEntry> libentry)
{
	std::vector<std::shared_ptr<LibraryEntry>> libentries;
	libentries.push_back(libentry);
	appendRows(libentries);
}

/**
 * Append the items in @a libentries to the model in one operation, i.e. without a separate insertRows(),
 * and with a single beginInsertRows()/endInsertRows() pair.
 * @param libentries
 */
void LibraryModel::appendRows(std::vector<std::shared_ptr<LibraryEntry>> libentries)
{
	auto start_rowcount = rowCount();

	beginInsertRows(QModelIndex(), start_rowcount, start_rowcount+libentries.size()-1);

	m_library.addNewEntries(libentries);

	endInsertRows();
}

int LibraryModel::getColFromSection(SectionID section_id) const
{
	for(size_t i = 0; i<m_columnSpecs.size(); ++i)
	{
		if(m_columnSpecs[i].m_section_id == section_id)
		{
			return i;
		}
	}
    qCritical() << "No such section:" << static_cast<int>(section_id);
	return -1;
}

SectionID LibraryModel::getSectionFromCol(int col) const
{
	return m_columnSpecs[col].m_section_id;
}

QUrl LibraryModel::getLibRootDir() const
{
	return m_library.getRootUrl();
}

QString LibraryModel::getLibraryName() const
{
	return m_library.getLibraryName();
}

qint64 LibraryModel::getLibraryNumEntries() const
{
	return m_library.getNumEntries();
}

LibraryModel::NumDirsFiles LibraryModel::getLibraryNumDirsFiles() const
{
	NumDirsFiles retval;

	retval.files = m_library.m_num_unpopulated;
	return retval;
}

void LibraryModel::setLibraryRootUrl(const QUrl& url)
{
	beginResetModel();

	// Give the library a starting point.
	m_library.setRootUrl(url);

	// Create a cache file for this Library.
	createCacheFile(url);

	connectSignals();
// M_TODO("SEPARATE SCANNING FROM INIT");
//	Q_EMIT statusSignal(LibState::ScanningForFiles, 0, 0);
	Q_EMIT startFileScanSignal(m_library.m_root_url);

	endResetModel();
}

void LibraryModel::close(bool delete_cache)
{
	qDebug() << "Closing model";

	// Stop the background thread.
	stopAllBackgroundThreads();
	// Disconnect signals so we don't get any pending messages from the thread we just stopped.
	disconnectIncomingSignals();
	if(delete_cache)
	{
		// Delete the cache file.
		deleteCache();
	}
}

QVariant LibraryModel::toVariant() const
{
	InsertionOrderedMap<QString, QVariant> map;

	map_insert_or_die(map, "the_models_library", m_library);

	return map;
}

void LibraryModel::fromVariant(const QVariant& variant)
{
	InsertionOrderedMap<QString, QVariant> map;
	qviomap_from_qvar_or_die(&map, variant);

	QVariant temp = map.at("the_models_library");

	Q_ASSERT((temp.canConvert<InsertionOrderedMap<QString, QVariant>>()));
	InsertionOrderedMap<QString, QVariant> qvar_temp_lib = temp.value<InsertionOrderedMap<QString, QVariant>>();
	Library temp_lib;

	temp_lib.fromVariant(qvar_temp_lib);

// #warning "Do we need to delete any old library here?"
	m_library = temp_lib;
}

Qt::DropActions LibraryModel::supportedDragActions() const
{
	// Only copy out of the LibraryModel, no moves.
	return Qt::CopyAction;
}

Qt::DropActions LibraryModel::supportedDropActions() const
{
	// The LibraryModel currently doesn't support any drops.
	return Qt::IgnoreAction;
}

QStringList LibraryModel::mimeTypes() const
{
	// M_MESSAGE("TODO: Return url type as well?");

	return g_additional_supported_mimetypes;
}


QMimeData* LibraryModel::mimeData(const QModelIndexList& indexes) const
{
	std::vector<std::shared_ptr<LibraryEntry>> row_items;
	QList<QUrl> urls;

	for(auto i : indexes)
	{
		if(i.column() == 0)
		{
			auto e = getItem(i);
			row_items.push_back(e);
			urls.push_back(e->getM2Url());
		}
	}

    if(!row_items.empty())
	{
		qDebug() << QString("Returning %1 row(s)").arg(row_items.size());
		LibraryEntryMimeData* e = new LibraryEntryMimeData();
        e->setData(*mimeTypes().cbegin(), QByteArray());
		e->m_lib_item_list = row_items;
		e->setUrls(urls);
		return e;
	}

	// "If the list of indexes is empty, or there are no supported MIME types, 0 is returned rather than a serialized empty list.".
	return nullptr;
}


void LibraryModel::SLOT_onIncomingFilename(QString filename)
{
    auto new_entry = LibraryEntry::fromUrl(filename);
//	qDb() << "URL:" << new_entry->getUrl();
	appendRow(new_entry);
}

void LibraryModel::SLOT_processReadyResults(MetadataReturnVal lritem_vec)
{
    // We got one of ??? things back:
    // - A single pindex and associated LibraryEntry*, maybe new, maybe a rescan..
    // - A single pindex and more than one LibraryEntry*, the result of the first scan after the file was found.
    // - Multiple pindexs and LibraryEntry*'s.  The result of a multi-track file rescan.

    if(lritem_vec.m_num_tracks_found == 0)
    {
        qCro() << "RESULT WAS EMPTY";
    }

    if(lritem_vec.m_num_tracks_found > 1
       && lritem_vec.m_original_pindexes.size() == 1
            && lritem_vec.m_new_libentries.size() == lritem_vec.m_num_tracks_found)
    {
        // It's a valid, new, multi-track entry.
        SLOT_onIncomingPopulateRowWithItems_Multiple(lritem_vec.m_original_pindexes[0], lritem_vec.m_new_libentries);
    }
    else if(lritem_vec.m_new_libentries.size() == lritem_vec.m_num_tracks_found
            && lritem_vec.m_original_pindexes.size() == lritem_vec.m_num_tracks_found)
    {
        // It's a matching set of pindexes and libentries.

        for(int i=0; i<lritem_vec.m_num_tracks_found; ++i)
        {
            if (!lritem_vec.m_original_pindexes[i].isValid())
            {
                qWarning() << "Invalid persistent index, ignoring update";
                return;
            }

            // None of the returned entries should be null.
            Q_ASSERT(lritem_vec.m_new_libentries[i] != nullptr);

            qDebug() << "Replacing entry"; // << item->getUrl();
            // item is a single song which has its metadata populated.
            // Reconstruct the QModelIndex we sent out.
            auto initial_row_index = QModelIndex(lritem_vec.m_original_pindexes[i]);
            auto row = initial_row_index.row();
            qDebug() << QString("incoming single item, row %1").arg(row);
            // Metadata's been populated.
            setData(initial_row_index, QVariant::fromValue(lritem_vec.m_new_libentries[i]));
        }
    }
    else
    {
        // Not sure what we got.
        qCritical() << "pindexes/libentries/num_new_entries:" << lritem_vec.m_original_pindexes.size()
                                                              << lritem_vec.m_new_libentries.size();
                                                              // lritem_vec.m_new_libentries;
        Q_ASSERT_X(0, "Scanning", "Not sure what we got");
    }
}

void LibraryModel::SLOT_processReadyResults(LibraryEntryLoaderJobResult loader_results)
{
    // We got one of ??? things back:
    // - A single pindex and associated LibraryEntry*, maybe new, maybe a rescan..
    // - A single pindex and more than one LibraryEntry*, the result of the first scan after the file was found.
    // - Multiple pindexs and LibraryEntry*'s.  The result of a multi-track file rescan.

    if(loader_results.m_num_tracks_found == 0)
    {
        // Found nothing, error out the entry.
        qCro() << "RESULT WAS EMPTY";
        /// @todo
        Q_ASSERT(0);
    }

    /// @todo Eventually can go away.
    AMLM_ASSERT_EQ(loader_results.m_new_libentries.size(), loader_results.m_num_tracks_found);

    if(loader_results.m_new_libentries.size() > 1)
    {
        // It's a single file/multi-track.
        SLOT_onIncomingPopulateRowWithItems_Multiple(loader_results.m_original_pindex, loader_results.m_new_libentries);
    }
    else if(loader_results.m_new_libentries.size() == 1)
    {
        // It's a single file/single track.
        SLOT_onIncomingPopulateRowWithItems_Single(loader_results.m_original_pindex, loader_results.m_new_libentries[0]);

//        for(int i=0; i<loader_results.m_num_tracks_found; ++i)
//        {
//            if (!loader_results.m_original_pindex.isValid())
//            {
//                qWarning() << "Invalid persistent index, ignoring update";
//                Q_ASSERT(0);
//                return;
//            }

//            // None of the returned entries should be null.
//            Q_ASSERT(loader_results.m_new_libentries[i] != nullptr);

//            qDebug() << "Replacing entry"; // << item->getUrl();
//            // item is a single song which has its metadata populated.
//            // Reconstruct the QModelIndex we sent out.
//            auto initial_row_index = QModelIndex(loader_results.m_original_pindexes[i]);
//            auto row = initial_row_index.row();
//            qDebug() << QString("incoming single item, row %1").arg(row);
//            // Metadata's been populated.
//            setData(initial_row_index, QVariant::fromValue(loader_results.m_new_libentries[i]));
//        }
    }
    else
    {
        // Not sure what we got.
        qCritical() << "pindexes/libentries/num_new_entries:" << loader_results.m_original_pindex
                                                              << loader_results.m_new_libentries.size();
                                                              // lritem_vec.m_new_libentries;
        Q_ASSERT_X(0, "Scanning", "Not sure what we got");
    }
}

void LibraryModel::SLOT_onIncomingPopulateRowWithItems_Single(QPersistentModelIndex pindex, std::shared_ptr<LibraryEntry> item)
{
	// item is a single song which has its metadata populated.
	// Reconstruct the QModelIndex we sent out.
    Q_ASSERT(pindex.isValid());
    Q_ASSERT(pindex.model() == this);
    const QModelIndex& initial_row_index = pindex;

    // Metadata's been populated.
	setData(initial_row_index, QVariant::fromValue(item));
//	setData(initial_row_index, QVariant::fromValue(item), ModelUserRoles::PointerToItemRole);

	finishIncoming();
}

void LibraryModel::SLOT_onIncomingPopulateRowWithItems_Multiple(QPersistentModelIndex pindex, std::vector<std::shared_ptr<LibraryEntry> > items)
{
	if(!pindex.isValid())
	{
		qCritical() << QString("invalid pindex, item list len: %1, first item: %2").arg(items.size()).arg(items[0]->getUrl().toString());
		return;
	}
	// Reconstruct the QModelIndex we sent out.
	auto initial_row_index = QModelIndex(pindex);
	auto row = initial_row_index.row();
	Q_ASSERT(row >= 0);

	// This was a multi-track file which was split into its subtracks.
	// items is a list which needs to be added to the model, pindex needs to be removed.

	insertRows(row + 1, items.size());

	// Update the internal model data.
	// Replace the default-constructed items we just inserted after the old one.
	for(int i = 0; i < static_cast<int>(items.size()); ++i)
	{
		setData(index(row + 1 + i, 0), QVariant::fromValue(items[i]), Qt::EditRole);
//		setData(index(row + 1 + i, 0), QVariant::fromValue(items[i]), ModelUserRoles::PointerToItemRole);
	}

	// Delete the original LibraryEntry which pointed to the entire album.
    removeRow(row);
}


void LibraryModel::createCacheFile(QUrl root_url)
{
	// Make sure the cache directory exists.
	QDir dummy_dir_obj(".");
	if(!dummy_dir_obj.exists(m_cachedir.toLocalFile()))
	{
		if(!dummy_dir_obj.mkpath(m_cachedir.toLocalFile()))
		{
			qCritical() << QString("Couldn't create cache directory") << m_cachedir.toLocalFile() << ":" << m_cachedir.errorString();
			return;
		}
	}

	// Set up a cache file.
	qDebug() << "Cachedir:" << m_cachedir;
	QUrl cacheurl = m_cachedir.resolved(QUrl(QString("library_cache_%1_XXXXXX.json").arg(root_url.fileName())));
	qDebug() << "Cacheurl:" << cacheurl;
	QTemporaryFile temp_lib_cache_file(cacheurl.toLocalFile());
	if(!temp_lib_cache_file.open())
	{
		qCritical() << QString("Couldn't create cache file") << temp_lib_cache_file.fileName() << ":" << temp_lib_cache_file.errorString();
		return;
	}
	temp_lib_cache_file.setAutoRemove(false);
	m_lib_cache_file.setFileName(temp_lib_cache_file.fileName());
	temp_lib_cache_file.close();
	// Note here that we're depending on this behavior of QTemporaryFile:
	// "Reopening a QTemporaryFile after calling close() is safe. For as long as the QTemporaryFile object itself is not destroyed,
	// the unique temporary file will exist and be kept open internally by QTemporaryFile."
	// (http://doc.qt.io/qt-5/qtemporaryfile.html)
	if(!m_lib_cache_file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qCritical() << "Couldn't open cache file" << m_lib_cache_file.fileName() << ":" << m_lib_cache_file.errorString();
		return;
	}
	qDebug() << "Cache file for Library '" << root_url << "' is '" << m_lib_cache_file.fileName() << "'";
}

void LibraryModel::stopAllBackgroundThreads()
{
	qDebug() << "Stopping background thread...";
	/// @todo
	qDebug() << "Background thread stopped.";
}

void LibraryModel::deleteCache()
{
	m_lib_cache_file.cancelWriting();
	if(QFile::remove(m_lib_cache_file.fileName()))
	{
		qDebug() << "Successfully removed cache file";
	}
	else
	{
        qWarning() << QString("Failed to remove cache file '%1': %2")
                          .arg(m_lib_cache_file.fileName(), m_lib_cache_file.errorString());
    }
}

void LibraryModel::connectSignals()
{
    connect_or_die(this, &LibraryModel::SIGNAL_selfSendReadyResults,
                   this, qOverload<LibraryEntryLoaderJobResult>(&LibraryModel::SLOT_processReadyResults));

	// Connect model signal to start scanning a URL to the async filesystem traverser.
	/// @todo This is clunky, refactor.
	connect(this, &LibraryModel::startFileScanSignal, m_rescanner, &LibraryRescanner::startAsyncDirectoryTraversal);
}

void LibraryModel::disconnectIncomingSignals()
{
}

void LibraryModel::finishIncoming()
{
	// Tell anyone listening our current status.
    qDbo() << QString("Status: %1 populated, %2 rows").arg(m_library.getNumPopulatedEntries()).arg(rowCount());
//	Q_EMIT statusSignal(LibState::PopulatingMetadata, m_library.getNumPopulatedEntries(), rowCount());
}

static QString table_row(const std::string& s1, const std::string& s2)
{
	QString retval = "<tr>";
    for(const auto& s : {s1, s2})
	{
		retval += "<td>" + toqstr(s) + "</td>";
	}
	retval += "</tr>";
	return retval;
}

QString LibraryModel::getEntryStatusToolTip(LibraryEntry* item) const
{
	QString tttext;

	AMLMTagMap mdff = item->getAllMetadata();
	AMLMTagMap::const_iterator cit;

	tttext =
"<b>Library Entry Info</b><hr>"
"<table>";
	for(cit = mdff.cbegin(); cit != mdff.cend(); ++cit)
	{
		tttext += table_row(tostdstr(cit->first), tostdstr(cit->second));
	}

tttext += "</table>";

	return tttext;
}

QVector<VecLibRescannerMapItems> LibraryModel::getLibRescanItems()
{
    QVector<VecLibRescannerMapItems> items_to_rescan;

    // Get a list of all entries we'll need to do an asynchronous rescan of the library.
    if(rowCount() > 0)
    {
        // At least one row, so we have something to refresh.

        // Collect a snapshot of info to send to the other thread for refreshing.

        VecLibRescannerMapItems multientry;
        std::shared_ptr<LibraryEntry> last_entry = nullptr;

        for(auto i=0; i<rowCount(); ++i)
        {
            auto item = getItem(index(i,0));

            qDebug() << "Item URL:" << i << item->getUrl();

            if(last_entry != nullptr && item->isFromSameFileAs(last_entry.get()))
            {
                // It's from the same file as the last entry we looked at.
                // Queue it up in the current batch.
                multientry.push_back(LibraryRescannerMapItem({QPersistentModelIndex(index(i, 0)), item}));
            }
            else
            {
                // It's the first entry or it's from a different file.  Send out the previous rescan item(s) and start a new batch.
                if(!multientry.empty())
                {
                    items_to_rescan.append(multientry);
                    qDebug() << "PUSHING MULTIENTRY, SIZE:" << multientry.size();
                }
                multientry.clear();
                multientry.push_back(LibraryRescannerMapItem({QPersistentModelIndex(index(i, 0)), item}));
            }
            last_entry = item;
        }

        if(!multientry.empty())
        {
            // It wasn't cleared by the last iteration above, so we have to append it here.
            items_to_rescan.append(multientry);
            qDebug() << "PUSHING LAST MULTIENTRY, SIZE:" << multientry.size();
            multientry.clear();
        }
    }

    qDb() << "RETURNING ITEMS:" << items_to_rescan.size();
    return items_to_rescan;
}

void LibraryModel::startRescan()
{
#if 0
	// Start an asynchronous rescan of the library.
	if(rowCount() > 0)
	{
		// At least one row, so we have something to refresh.

		// Collect a snapshot of info to send to the other thread for refreshing.
		QVector<VecLibRescannerMapItems> items_to_rescan;

		VecLibRescannerMapItems multientry;
		std::shared_ptr<LibraryEntry> last_entry = nullptr;

		for(auto i=0; i<rowCount(); ++i)
		{
			auto item = getItem(index(i,0));

			qDebug() << "Item URL:" << i << item->getUrl();

			if(last_entry != nullptr && item->isFromSameFileAs(last_entry.get()))
			{
				// It's from the same file as the last entry we looked at.
				// Queue it up in the current batch.
                multientry.push_back(LibraryRescannerMapItem({QPersistentModelIndex(index(i, 0)), item}));
			}
			else
			{
				// It's the first entry or It's from a different file.  Send out the previous rescan item(s) and start a new batch.
				if(multientry.size() > 0)
				{
					items_to_rescan.append(multientry);
					qDebug() << "PUSHING MULTIENTRY, SIZE:" << multientry.size();
				}
				multientry.clear();
                multientry.push_back(LibraryRescannerMapItem({QPersistentModelIndex(index(i, 0)), item}));
			}
			last_entry = item;
		}

		if(multientry.size() > 0)
		{
			// It wasn't cleared by the last iteration above, so we have to append it here.
			items_to_rescan.append(multientry);
			qDebug() << "PUSHING LAST MULTIENTRY, SIZE:" << multientry.size();
			multientry.clear();
		}

		// Tell the scanner what to rescan.
		m_rescanner->startAsyncRescan(items_to_rescan);
	}

	// Now we just let it do its thing.  It will call back through the public interface with updated metadata.
#endif
}

void LibraryModel::cancelRescan()
{
	qDebug() << "CANCEL";
	m_rescanner->cancelAsyncDirectoryTraversal();
}

