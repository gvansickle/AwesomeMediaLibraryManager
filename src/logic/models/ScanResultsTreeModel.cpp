/*
 * Copyright 2018, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
#include "ScanResultsTreeModel.h"

// Qt5
#include <QAbstractItemModelTester>


// Ours
#include "AbstractTreeModel.h"
#include "ScanResultsTreeModelItem.h"
#include "AbstractTreeModelHeaderItem.h"

ScanResultsTreeModel::ScanResultsTreeModel(QObject *parent) : BASE_CLASS(parent)
{
}

// static
std::shared_ptr<ScanResultsTreeModel> ScanResultsTreeModel::construct(QObject* parent)
{
	std::shared_ptr<ScanResultsTreeModel> retval(new ScanResultsTreeModel(parent));
	// Create the root item, which is a HeaderItem.
	Q_ASSERT(retval->m_root_item == nullptr);
	retval->m_root_item = AbstractTreeModelHeaderItem::construct({}, retval);
	/// @todo Need on/off, this slows things way down.
//	retval->m_model_tester = new QAbstractItemModelTester(retval.get(), QAbstractItemModelTester::FailureReportingMode::Fatal, retval.get());
	return retval;
}

void ScanResultsTreeModel::setBaseDirectory(const QUrl &base_directory)
{
	m_base_directory = base_directory;
}

std::shared_ptr<AbstractTreeModelItem> ScanResultsTreeModel::getItemByIndex(const QModelIndex& index)
{
//	std::shared_lock read_lock(m_rw_mutex);
	std::unique_lock write_lock(m_rw_mutex);

	// Get a pointer to the indexed item.
	auto retval = BASE_CLASS::getItemById(UUIncD(index.internalId()));

	return retval;
}

std::shared_ptr<AbstractTreeModelItem> ScanResultsTreeModel::getItemById(const UUIncD& id) const
{
//	std::shared_lock read_lock(m_rw_mutex);
	std::unique_lock write_lock(m_rw_mutex);

	return BASE_CLASS::getItemById(id);
}

QVariant ScanResultsTreeModel::data(const QModelIndex& index, int role) const
{
//	std::shared_lock read_lock(m_rw_mutex);
	std::unique_lock write_lock(m_rw_mutex);

	/// @todo Specialize behavior in here if needed.
	return BASE_CLASS::data(index, role);
}

bool ScanResultsTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	std::unique_lock write_lock(m_rw_mutex);

	std::shared_ptr<AbstractTreeModelItem> item = getItemByIndex(index);
	/// @todo Kden looks like it's only handling a rename here:
	// if (item->rename(value.toString(), index.column())) {
	//	emit dataChanged(index, index, {role});
	//	return true;
	//}
	//// Item name was not changed
	//return false;

	return BASE_CLASS::setData(index, value, role);
}

Qt::ItemFlags ScanResultsTreeModel::flags(const QModelIndex& index) const
{
//	std::shared_lock read_lock(m_rw_mutex);
	std::unique_lock write_lock(m_rw_mutex);

	/// @note Kden does a switch on the derived item type and returns ItemIsEnabled/Selectable/DropEnabled/etc. here.

	return BASE_CLASS::flags(index);
}

QVariant ScanResultsTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
//	std::shared_lock read_lock(m_rw_mutex);
	std::unique_lock write_lock(m_rw_mutex);

	/// @todo KDen is doing a switch(section) and sending out the column name here.

	return BASE_CLASS::headerData(section, orientation, role);
}

int ScanResultsTreeModel::columnCount(const QModelIndex& parent) const
{
//	std::shared_lock read_lock(m_rw_mutex);
	std::unique_lock write_lock(m_rw_mutex);

	/// @note KDen does this slightly different:
	///     if (parent.isValid()) {
	//	return getBinItemByIndex(parent)->supportedDataCount();
	//}
	//return std::static_pointer_cast<ProjectFolder>(rootItem)->supportedDataCount();

	return BASE_CLASS::columnCount(parent);
}

QMimeData* ScanResultsTreeModel::mimeData(const QModelIndexList& indices) const
{
//	std::shared_lock read_lock(m_rw_mutex);
	std::unique_lock write_lock(m_rw_mutex);

	/// @todo Return mime data here.

	return BASE_CLASS::mimeData(indices);
}

bool ScanResultsTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
	std::unique_lock write_lock(m_rw_mutex);

	/// KDen does requestAddWhatever()'s here.

	return BASE_CLASS::dropMimeData(data, action, row, column, parent);
}

bool ScanResultsTreeModel::requestAppendItem(const std::shared_ptr<ScanResultsTreeModelItem>& item, UUIncD parent_uuincd, Fun& undo, Fun& redo)
{
	std::unique_lock write_lock(m_rw_mutex);

	/// @note KDenLive does a number of id checks here, in its requestAddXxxx() members in ProjectItemModel.
	/// They take an id instead of the already-existing item we have here, then call <whatever>::create(...); to create
	/// the new item.
	/// So I'm not sure this request func is actually needed.
//Q_ASSERT(0);
//	std::shared_ptr<ScanResultsTreeModelItem> new_item = ScanResultsTreeModelItem::construct(DirScanResult(), std::static_pointer_cast<ScanResultsTreeModel>(shared_from_this()));
	return addItem(item, parent_uuincd, undo, redo);
}

bool ScanResultsTreeModel::requestAppendItems(std::vector<std::shared_ptr<ScanResultsTreeModelItem>> items,
											  UUIncD parent_uuincd, Fun& undo, Fun& redo)
{
	std::unique_lock write_lock(m_rw_mutex);
	bool retval = true;

	for(auto sptr : items)
	{
		/// @todo Batch undo/redo?
		bool status = requestAppendItem(sptr, parent_uuincd, noop_undo_redo_lambda, noop_undo_redo_lambda);
		if(status == false)
		{
			retval = false;
		}
	}

	return retval;
}

bool ScanResultsTreeModel::requestAddScanResultsTreeModelItem(const DirScanResult& dsr, UUIncD parent_uuincd, Fun& undo, Fun& redo)
{
	std::unique_lock write_lock(m_rw_mutex);

	std::shared_ptr<ScanResultsTreeModelItem> new_item
			= ScanResultsTreeModelItem::construct(dsr, std::static_pointer_cast<ScanResultsTreeModel>(shared_from_this()));
	return addItem(new_item, parent_uuincd, undo, redo);
}

bool ScanResultsTreeModel::requestAddSRTMItem_LibEntry(const std::shared_ptr<LibraryEntry>& libentry, const DirScanResult& dsr,
		UUIncD parent_uuincd, Fun& undo, Fun& redo)
{
	std::unique_lock write_lock(m_rw_mutex);

	std::shared_ptr<SRTMItem_LibEntry> new_item
			= SRTMItem_LibEntry::construct(libentry, dsr, std::static_pointer_cast<ScanResultsTreeModel>(shared_from_this()));
	return addItem(new_item, parent_uuincd, undo, redo);
}

/**
 * ScanResultsTreeModel XML tags.
 */
#define M_DATASTREAM_FIELDS(X) \
	X(XMLTAG_SRTM_ROOT_ITEM, tree_model_root_item) \
	X(XMLTAG_SRTM_BASE_DIRECTORY, base_directory) \
	X(XMLTAG_SRTM_TITLE, title) \
	X(XMLTAG_SRTM_CREATOR, creator) \
	X(XMLTAG_SRTM_DATE, date) \
	X(XMLTAG_SRTM_TS_LAST_SCAN_START, ts_last_scan_start) \
	X(XMLTAG_SRTM_TS_LAST_SCAN_END, ts_last_scan_end)

/// Strings to use for the tags.
#define X(field_tag, member_field) static const QLatin1Literal field_tag ( # member_field );
	M_DATASTREAM_FIELDS(X);
#undef X

QVariant ScanResultsTreeModel::toVariant() const
{
	QVariantInsertionOrderedMap map;

	std::unique_lock write_lock(m_rw_mutex);

#define X(field_tag, member_field) map_insert_or_die(map, field_tag, member_field);
//	M_DATASTREAM_FIELDS(X)
#undef X

	// The one piece of data we really need here, non-xspf.
	map_insert_or_die(map, XMLTAG_SRTM_BASE_DIRECTORY, m_base_directory);

	/// @todo Start of xspf-specific stuff.
//		XmlElement playlist("playlist", [=](XmlElement* e, QXmlStreamWriter* out){
//			auto& xml = *out;
//			xml.writeDefaultNamespace("http://xspf.org/ns/0/");
//			xml.writeAttribute("version", "1");
//			xml.writeNamespace("http://amlm/ns/0/", "amlm"); // Our extension namespace.
	//		xml.writeAttribute("version", "1");

		// No DTD for xspf.

	/// @todo XSPF Playlist metadata here.
	/// http://www.xspf.org/xspf-v1.html#rfc.section.2.3.1
	/// <title> "A human-readable title for the playlist. xspf:playlist elements MAY contain exactly one."
	map_insert_or_die(map, XMLTAG_SRTM_TITLE, QString("XSPF playlist title goes HERE"));

	/// <creator> "Human-readable name of the entity (author, authors, group, company, etc) that authored the playlist. xspf:playlist elements MAY contain exactly one."
	map_insert_or_die(map, XMLTAG_SRTM_CREATOR, QString("XSPF playlist CREATOR GOES HERE"));

	/// ...
	/// <date>	"Creation date (not last-modified date) of the playlist, formatted as a XML schema dateTime. xspf:playlist elements MAY contain exactly one.
	///	A sample date is "2005-01-08T17:10:47-05:00".
	map_insert_or_die(map, XMLTAG_SRTM_DATE, QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

	// Insert the invisible root item, which will recursively add all children.
	/// @todo It also serves as the model's header, not sure that's a good overloading.
	map_insert_or_die(map, XMLTAG_SRTM_ROOT_ITEM, m_root_item->toVariant());

	// Timestamps for the start and end of the last full scan.
	map_insert_or_die(map, XMLTAG_SRTM_TS_LAST_SCAN_START, QVariant(QDateTime()));
	map_insert_or_die(map, XMLTAG_SRTM_TS_LAST_SCAN_END, QVariant(QDateTime()));

	return map;
}

void ScanResultsTreeModel::fromVariant(const QVariant& variant)
{
	std::unique_lock write_lock(m_rw_mutex);

	QVariantInsertionOrderedMap map;
	qviomap_from_qvar_or_die(&map, variant);

	/// @todo This should have a list of known base directory paths,
	///         e.g. the file:// URL and the gvfs /run/... mount point, Windows drive letter paths, etc.
	map_read_field_or_warn(map, XMLTAG_SRTM_BASE_DIRECTORY, &m_base_directory);
//	m_base_directory = map.value(SRTMTagToXMLTagMap[SRTMTag::BASE_DIRECTORY]).toUrl();

	QString title, creator;
	map_read_field_or_warn(map, XMLTAG_SRTM_TITLE, &title);//.toString();
	map_read_field_or_warn(map, XMLTAG_SRTM_CREATOR, &creator);//.toString();

	/// @todo This is a QDateTime
	QString creation_date;
	map_read_field_or_warn(map, XMLTAG_SRTM_DATE, &creation_date);//.toString();

	/// @note This is a QVariantMap, contains abstract_tree_model_header as a QVariantList.
	if(m_root_item != nullptr)
	{
		m_root_item.reset();
	}
	M_TODO("");
//	m_root_item = std::make_shared<AbstractTreeModelHeaderItem>();
//	m_root_item->fromVariant(map.value(SRTMTagToXMLTagMap[SRTMTag::ROOT_ITEM]));
M_WARNING("TODO: There sometimes isn't a root item in the map.");
	map_read_field_or_warn(map, XMLTAG_SRTM_ROOT_ITEM, m_root_item);
	/// @todo Read these in.
	// SRTMItemTagToXMLTagMap[SRTMItemTag::TS_LAST_SCAN_START]
	// SRTMItemTagToXMLTagMap[SRTMItemTag::TS_LAST_SCAN_END]

#warning @todo INCOMPLETE/error handling
}

void ScanResultsTreeModel::register_item(const std::shared_ptr<AbstractTreeModelItem>& item)
{
	std::unique_lock write_lock(m_rw_mutex);
	BASE_CLASS::register_item(item);
}

void ScanResultsTreeModel::deregister_item(UUIncD id, AbstractTreeModelItem* item)
{
	std::unique_lock write_lock(m_rw_mutex);
	/// Per KDenLive comment: TODO : here, we should suspend jobs belonging to the item we delete. They can be restarted if the item is reinserted by undo
	BASE_CLASS::deregister_item(id, item);
}

bool ScanResultsTreeModel::addItem(const std::shared_ptr<ScanResultsTreeModelItem>& item, UUIncD parent_uuincd, Fun& undo, Fun& redo)
{
	// Acquire a write lock.
	std::unique_lock write_lock(m_rw_mutex);

	// Get ptr to the specified parent item.
	auto parent_item_by_id = getItemById(parent_uuincd);
	std::shared_ptr<AbstractTreeModelItem> parent_item = std::dynamic_pointer_cast<AbstractTreeModelItem>(parent_item_by_id);

	Q_ASSERT(parent_item);

	/// @todo KDen does some type checking in here of what item is and if it can be added to parent.

	// Create an addItem lambda which will be what ultimately adds the item to the parent.
	Fun operation = addItem_lambda(item, parent_item->getId());

	UUIncD item_id = item->getId();
	Fun reverse = removeItem_lambda(item_id);
	// Run the addItem_lambda() we created a few lines above to add the item to the model.
	bool retval = operation();
	Q_ASSERT(item->isInModel());
	if(retval)
	{
		// It was added, update the undo/redo state.
		UPDATE_UNDO_REDO(m_rw_mutex, operation, reverse, undo, redo);
	}
	return retval;
}
