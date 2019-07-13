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

// sqlite_orm
#include <third_party/sqlite_orm/include/sqlite_orm/sqlite_orm.h>

// Ours
#include "AbstractTreeModel.h"
#include "ScanResultsTreeModelItem.h"
#include "AbstractTreeModelHeaderItem.h"
#include "SRTMItemLibEntry.h"
#include <logic/serialization/SerializationHelpers.h>

#include <serialization/XmlSerializer.h>


ScanResultsTreeModel::ScanResultsTreeModel(QObject *parent)
    : BASE_CLASS(parent)
{
void ScanResultsTreeModel::setup()
{
	// We connect the signals of the abstractitemmodel to a more generic one.
	connect_or_die(this, &ScanResultsTreeModel::columnsMoved, this, &ScanResultsTreeModel::modelChanged);
	connect_or_die(this, &ScanResultsTreeModel::columnsRemoved, this, &ScanResultsTreeModel::modelChanged);
	connect_or_die(this, &ScanResultsTreeModel::columnsInserted, this, &ScanResultsTreeModel::modelChanged);
	connect_or_die(this, &ScanResultsTreeModel::rowsMoved, this, &ScanResultsTreeModel::modelChanged);
	connect_or_die(this, &ScanResultsTreeModel::rowsRemoved, this, &ScanResultsTreeModel::modelChanged);
	connect_or_die(this, &ScanResultsTreeModel::rowsInserted, this, &ScanResultsTreeModel::modelChanged);
	connect_or_die(this, &ScanResultsTreeModel::modelReset, this, &ScanResultsTreeModel::modelChanged);
	connect_or_die(this, &ScanResultsTreeModel::dataChanged, this, &ScanResultsTreeModel::modelChanged);
	connect_or_die(this, &ScanResultsTreeModel::modelChanged, this, &ScanResultsTreeModel::sendModification);
}

void ScanResultsTreeModel::sendModification()
{
//	if (auto ptr = this)
//	{
//		Q_ASSERT(m_pmindex.isValid());
//		QString name = ptr->data(m_pmindex, AssetParameterModel::NameRole).toString();
//		if (m_paramType == ParamType::KeyframeParam || m_paramType == ParamType::AnimatedRect || m_paramType == ParamType::Roto_spline)
//		{
//			m_lastData = getAnimProperty();
//			ptr->setParameter(name, m_lastData, false);
//		}
//		else
//		{
//			Q_ASSERT(false); // Not implemented, TODO
//		}
//	}
}

// static
std::shared_ptr<ScanResultsTreeModel> ScanResultsTreeModel::construct(QObject* parent)
{
	std::shared_ptr<ScanResultsTreeModel> retval(new ScanResultsTreeModel(parent));
	// Create the root item, which is a HeaderItem.
	Q_ASSERT(retval->m_root_item == nullptr);
	retval->m_root_item = AbstractTreeModelHeaderItem::construct({}, retval);
	/// @todo Need on/off, this slows things way down.
	retval->m_model_tester = new QAbstractItemModelTester(retval.get(), QAbstractItemModelTester::FailureReportingMode::Fatal, retval.get());
	return retval;
}

void ScanResultsTreeModel::setBaseDirectory(const QUrl &base_directory)
{
	std::unique_lock write_lock(m_rw_mutex);

	m_base_directory = base_directory;
}

void ScanResultsTreeModel::LoadDatabase(const QString& database_filename)
{
	qIn() << "###### READING" << database_filename;

	XmlSerializer xmlser;
	xmlser.set_default_namespace("http://xspf.org/ns/0/", "1");
	xmlser.HACK_skip_extra(false);
	xmlser.load(*this, QUrl::fromLocalFile(database_filename));

	qIn() << "###### TREEMODELPTR HAS NUM ROWS:" << rowCount();
	qIn() << "###### READ" << database_filename;
}

void ScanResultsTreeModel::SaveDatabase(const QString& database_filename)
{
	qIn() << "###### WRITING" << database_filename;
	qIn() << "###### TREEMODELPTR HAS NUM ROWS:" << rowCount();

	XmlSerializer xmlser;
	xmlser.set_default_namespace("http://xspf.org/ns/0/", "1");
	xmlser.save(*this, QUrl::fromLocalFile(database_filename), "playlist");

	qIn() << "###### WROTE" << database_filename;
}

UUIncD ScanResultsTreeModel::requestAddScanResultsTreeModelItem(const QVariant& variant, UUIncD parent_id, Fun undo, Fun redo)
{
	std::unique_lock write_lock(m_rw_mutex);

	// ::construct() a new tree model item from variant.
	std::shared_ptr<ScanResultsTreeModelItem> new_item = ScanResultsTreeModelItem::construct(variant,
																							 std::static_pointer_cast<ScanResultsTreeModel>(shared_from_this()));
	bool status = addItem(new_item, parent_id, undo, redo);

	if(!status)
	{
		// Add failed for some reason, return a null UUIncD.
		return UUIncD::null();
	}

	new_item->fromVariant(variant);

	return new_item->getId();
}

UUIncD ScanResultsTreeModel::requestAddSRTMLibEntryItem(const QVariant& variant, UUIncD parent_id, Fun undo, Fun redo)
{
	std::unique_lock write_lock(m_rw_mutex);

	auto new_item = SRTMItem_LibEntry::construct(variant, std::static_pointer_cast<ScanResultsTreeModel>(shared_from_this()));
	bool status = addItem(new_item, parent_id, undo, redo);

	if(!status)
	{
		// Add failed for some reason, return a null UUIncD.
		return UUIncD::null();
	}

	new_item->fromVariant(variant);

	return new_item->getId();
}

//UUIncD ScanResultsTreeModel::requestAddExistingTreeModelItem(std::shared_ptr<AbstractTreeModelItem> new_item, UUIncD parent_id, Fun undo, Fun redo)
//{
//	std::unique_lock write_lock(m_rw_mutex);

//	// ::construct() a new tree model item from variant.
////	std::shared_ptr<AbstractTreeModelItem> new_item = make_item_from_variant(variant);

//	bool status = addItem(new_item, parent_id, undo, redo);

//	if(!status)
//	{
//		// Add failed for some reason, return a null UUIncD.
//		return UUIncD::null();
//	}
//	return new_item->getId();
//}

void ScanResultsTreeModel::toOrm(std::string filename) const
{
#if 0
	using namespace sqlite_orm;
	auto storage = make_storage(filename,
								make_table("items",
										   make_column("id",
													   &ScanResultsTreeModelItem::setId,
													   &ScanResultsTreeModelItem::getId, /*autoincrement(),*/ primary_key()),
										   make_column("model_item_map", &ScanResultsTreeModel::m_model_item_map),
										   make_column("m_root_item", &ScanResultsTreeModel::m_root_item)/*,
										   make_column("last_name", &User::lastName),
										   make_column("birth_date", &User::birthDate),
										   make_column("image_url", &User::imageUrl),
										   make_column("type_id", &User::typeId))*//*,
								make_table("item_types",
										   make_column("id", &UserType::id, autoincrement(), primary_key()),
										   make_column("name", &UserType::name, default_value("name_placeholder"))*/));
	storage.sync_schema();
#endif
}

void ScanResultsTreeModel::fromOrm(std::string filename)
{

}

/// Qt5 ids for the TreeItems it can hold.
static const int f_atmi_id = qMetaTypeId<AbstractTreeModelItem>();
static const int f_strmi_id = qMetaTypeId<ScanResultsTreeModelItem>();
static const int f_strmile_id = qMetaTypeId<SRTMItem_LibEntry>();


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

	// Timestamps for the start and end of the last full scan.
	map_insert_or_die(map, XMLTAG_SRTM_TS_LAST_SCAN_START, QVariant(QDateTime()));
	map_insert_or_die(map, XMLTAG_SRTM_TS_LAST_SCAN_END, QVariant(QDateTime()));

	// Insert the invisible root item, which will recursively add all children.
	/// @todo It also serves as the model's header, not sure that's a good overloading.
	qDb() << "START tree serialize";
	map_insert_or_die(map, XMLTAG_SRTM_ROOT_ITEM, m_root_item->toVariant());
	qDb() << "END tree serialize";

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

	/// @todo Read these in.
	QDateTime last_scan_start, last_scan_end;
	map_read_field_or_warn(map, XMLTAG_SRTM_TS_LAST_SCAN_START, &last_scan_start);
	map_read_field_or_warn(map, XMLTAG_SRTM_TS_LAST_SCAN_END, &last_scan_end);

	/// @note This is a QVariantMap, contains abstract_tree_model_header as a QVariantList.
	QVariantInsertionOrderedMap root_item_map;
	map_read_field_or_warn(map, XMLTAG_SRTM_ROOT_ITEM, &root_item_map);
	m_root_item->fromVariant(root_item_map);
	dump_map(map);
//	requestAddTreeModelItem()


#warning @todo INCOMPLETE/error handling
}
