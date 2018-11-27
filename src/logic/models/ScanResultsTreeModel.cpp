/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
#include "ScanResultsTreeModelXMLTags.h"

ScanResultsTreeModel::ScanResultsTreeModel(QObject *parent)
    : BASE_CLASS(parent)
{
	// Populate the parse factory functions with the first-layer node type names we know about.
	m_parse_factory_functions.emplace_back(&ScanResultsTreeModelItem::parse);
}

void ScanResultsTreeModel::setBaseDirectory(const QUrl &base_directory)
{
	m_base_directory = base_directory;
}


bool ScanResultsTreeModel::appendItems(QVector<AbstractTreeModelItem*> new_items, const QModelIndex& parent)
{
	return BASE_CLASS::appendItems(new_items, parent);
}

QVariant ScanResultsTreeModel::toVariant() const
{
	QVariantMap map;

	// The one piece of data we really need here, non-xspf.
	map.insert(SRTMTagToXMLTagMap[SRTMTag::BASE_DIRECTORY], m_base_directory);

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
	map.insert(SRTMTagToXMLTagMap[SRTMTag::TITLE], "XSPF playlist title goes HERE");

	/// <creator> "Human-readable name of the entity (author, authors, group, company, etc) that authored the playlist. xspf:playlist elements MAY contain exactly one."
	map.insert(SRTMTagToXMLTagMap[SRTMTag::CREATOR], "XSPF playlist CREATOR GOES HERE");

	/// ...
	/// <date>	"Creation date (not last-modified date) of the playlist, formatted as a XML schema dateTime. xspf:playlist elements MAY contain exactly one.
	///	A sample date is "2005-01-08T17:10:47-05:00".
	map.insert(SRTMTagToXMLTagMap[SRTMTag::DATE], QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

	// Insert the invisible root item, which will recursively add all children.
	/// @todo It also serves as the model's header, not sure that's a good overloading.
	map.insert(SRTMTagToXMLTagMap[SRTMTag::ROOT_ITEM], m_root_item->toVariant());

	return map;
}

void ScanResultsTreeModel::fromVariant(const QVariant& variant)
{
	QVariantMap map = variant.toMap();

	/// @todo This should have a list of known base directory paths,
	///         e.g. the file:// URL and the gvfs /run/... mount point, Windows drive letter paths, etc.
	m_base_directory = map.value(SRTMTagToXMLTagMap[SRTMTag::BASE_DIRECTORY]).toUrl();

	auto title = map.value(SRTMTagToXMLTagMap[SRTMTag::TITLE]).toString();
	auto creator = map.value(SRTMTagToXMLTagMap[SRTMTag::CREATOR]).toString();

	/// @todo This is a QDateTime
	auto creation_date = map.value(SRTMTagToXMLTagMap[SRTMTag::DATE]).toString();

	qDb() << M_NAME_VAL(title);
	qDb() << M_NAME_VAL(creator);
	qDb() << M_NAME_VAL(creation_date);

	/// @note This is a QVariantMap, contains abstract_tree_model_header as a QVariantList.
	m_root_item = new AbstractTreeModelHeaderItem();
	m_root_item->fromVariant(map.value(SRTMTagToXMLTagMap[SRTMTag::ROOT_ITEM]));

#warning @todo INCOMPLETE/error handling
}

AbstractTreeModelHeaderItem* ScanResultsTreeModel::make_root_node(QVector<QVariant> rootData)
{
	return new AbstractTreeModelHeaderItem(rootData);
}

