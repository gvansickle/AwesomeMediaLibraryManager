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

// Ours
#include "ScanResultsTreeModelItem.h"
#include "AbstractTreeModelHeaderItem.h"
#include <logic/serialization/SerializationHelpers.h>


ScanResultsTreeModel::ScanResultsTreeModel(QObject *parent)
    : BASE_CLASS(parent)
{
}

// static
std::shared_ptr<ScanResultsTreeModel> ScanResultsTreeModel::construct(QObject* parent)
{
	std::shared_ptr<ScanResultsTreeModel> retval(new ScanResultsTreeModel(parent));
	retval->m_root_item = AbstractTreeModelHeaderItem::construct(retval);
	return retval;
}

void ScanResultsTreeModel::setBaseDirectory(const QUrl &base_directory)
{
	m_base_directory = base_directory;
}

UUIncD ScanResultsTreeModel::requestAddTreeModelItem(const QVariant& variant, UUIncD parent_id, Fun undo, Fun redo)
{
	std::unique_lock write_lock(m_rw_mutex);

	// ::construct() a new tree model item from variant.
	std::shared_ptr<AbstractTreeModelItem> new_item = make_item_from_variant(variant);

	bool status = addItem(new_item, parent_id, undo, redo);

	if(!status)
	{
		// Add failed for some reason, return a null UUIncD.
		return UUIncD::null();
	}
	return new_item->getId();
}

/// Qt5 ids for the TreeItems it can hold.
static const int f_atmi_id = qMetaTypeId<AbstractTreeModelItem>();
static const int f_strmi_id = qMetaTypeId<ScanResultsTreeModelItem>();
static const int f_strmile_id = qMetaTypeId<SRTMItem_LibEntry>();


std::shared_ptr<AbstractTreeModelItem>
ScanResultsTreeModel::make_item_from_variant(const QVariant& variant)
{
	QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();

	// What was the derived class that was actually written?
	std::string metatype_class_str = map.get_attr("class");
	if(metatype_class_str.empty())
	{
		// Get as much info as we can.
		auto vartype = variant.type();
		const char* typename_per_var = variant.typeName();
		auto metatype = QMetaType::typeName(vartype);
		qDb() << QString("ERROR: No class attr:") << M_ID_VAL(metatype) << M_ID_VAL(vartype) << M_ID_VAL(typename_per_var);
		Q_ASSERT(0);
	}
	int metatype_id = QMetaType::type(metatype_class_str.c_str());

	std::shared_ptr<AbstractTreeModelItem> retval;

	auto typed_model_ptr = std::static_pointer_cast<ScanResultsTreeModel>(shared_from_this());

	if(metatype_id == f_atmi_id)
	{
		retval = AbstractTreeModelItem::construct(variant, typed_model_ptr, /*is root*/false);
	}
	else if(metatype_id == f_strmi_id)
	{
		retval = ScanResultsTreeModelItem::construct(variant, typed_model_ptr);
	}
	else if(metatype_id == f_strmile_id)
	{
		retval = SRTMItem_LibEntry::construct(variant, typed_model_ptr);
	}
	else
	{
		qCr() << "Trying to read in unknown class:" << metatype_id << metatype_class_str;
	}

	return retval;
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
	QVariantInsertionOrderedMap map;
	qviomap_from_qvar_or_die(&map, variant);

	using namespace AMLM;

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


#warning @todo INCOMPLETE/error handling
}
