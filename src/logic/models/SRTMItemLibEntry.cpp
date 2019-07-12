/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/**
 * @file SRTMItemLibEntry.cpp
 */
#include "SRTMItemLibEntry.h"

// Ours
#include <logic/LibraryEntry.h>
#include <serialization/ISerializable.h>
#include <serialization/SerializationHelpers.h>

/////////// @todo SRTMItem_LibEntry

//std::shared_ptr<SRTMItem_LibEntry> SRTMItem_LibEntry::construct(const DirScanResult& dsr, const std::shared_ptr<ScanResultsTreeModel>& model, bool is_root)
//{
//	std::shared_ptr<SRTMItem_LibEntry> self(new SRTMItem_LibEntry(dsr, model, is_root));
//	baseFinishConstruct(self);
//	return self;
//}

std::shared_ptr<SRTMItem_LibEntry> SRTMItem_LibEntry::construct(const QVariant& variant, const std::shared_ptr<ScanResultsTreeModel>& model, bool is_root)
{
	std::shared_ptr<SRTMItem_LibEntry> self(new SRTMItem_LibEntry(model, is_root));
	baseFinishConstruct(self);
	return self;
}

//SRTMItem_LibEntry::SRTMItem_LibEntry(const DirScanResult& dsr, const std::shared_ptr<ScanResultsTreeModel>& model, bool is_root)
//	: BASE_CLASS(dsr, std::static_pointer_cast<ScanResultsTreeModel>(model), is_root)
//{

//}

SRTMItem_LibEntry::SRTMItem_LibEntry(const std::shared_ptr<ScanResultsTreeModel>& model, bool is_root)
	: BASE_CLASS(std::static_pointer_cast<ScanResultsTreeModel>(model), is_root)
{

}

QVariant SRTMItem_LibEntry::data(int column, int role) const
{
	if((role != Qt::ItemDataRole::DisplayRole) && (role != Qt::ItemDataRole::EditRole))
	{
		return QVariant();
	}

	// We should have a valid LibraryEntry pointer.
	Q_ASSERT(m_library_entry);

#if 0
	switch(column)
	{
		case 0:
			return QVariant::fromValue(toqstr(m_key));
			break;
		case 1:
			return QVariant::fromValue(toqstr(m_val));
			break;
		default:
			return QVariant();
			break;
	}
#else

	if(!m_library_entry->isPopulated())
	{
		return QVariant("???");
	}

	if(role == Qt::DisplayRole /*|| role == Qt::ToolTipRole*/)
	{
		switch(column)
		{
			case 0:
				return m_library_entry->getFilename();
				break;
			case 1:
				return m_library_entry->getFileType();
				break;
			default:
				return QVariant();
				break;
		}
	}
	return QVariant();
#endif

}

int SRTMItem_LibEntry::columnCount() const
{
	return 2;
}

// Redefined because two classes in one file.
#define M_DATASTREAM_FIELDS(X) \
	/* TAG_IDENTIFIER, tag_string, member_field, var_name */ \
	X(XMLTAG_CHILD_NODE_LIST, child_node_list, nullptr) \
	X(XMLTAG_LIBRARY_ENTRIES, library_entries, nullptr)


/// Strings to use for the tags.
using strviw_type = QLatin1Literal;

///// Strings to use for the tags.
#define X(field_tag, tag_string, var_name) static const strviw_type field_tag ( # tag_string );
	M_DATASTREAM_FIELDS(X);
#undef X

QVariant SRTMItem_LibEntry::toVariant() const
{
	QVariantInsertionOrderedMap map;

	// Overwrite any class info added by the above.
	set_map_class_info(this, &map);

	QVariantHomogenousList list(XMLTAG_LIBRARY_ENTRIES, "m_library_entry");
	if(auto libentry = m_library_entry.get(); libentry != nullptr)
	{
		list_push_back_or_die(list, m_library_entry->toVariant());
	}
	map_insert_or_die(map, XMLTAG_LIBRARY_ENTRIES, list);

	QVariantHomogenousList child_var_list(XMLTAG_CHILD_NODE_LIST, "child");
	for(auto& it : m_child_items)
	{
		list_push_back_or_die(child_var_list, it->toVariant());
	}
	map_insert_or_die(map, XMLTAG_CHILD_NODE_LIST, child_var_list);

	return map;
}

void SRTMItem_LibEntry::fromVariant(const QVariant& variant)
{
	QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();

#define X(field_tag, tag_string, var_name) AMLM::map_read_field_or_warn(map, field_tag, var_name);
	M_DATASTREAM_FIELDS(X);
#undef X

	// Load LibraryEntry's.
	AMLM::map_read_field_or_warn(map, XMLTAG_LIBRARY_ENTRIES, &m_library_entry);
#if 0
	QVariantHomogenousList child_list = map.value(XMLTAG_CHILD_NODE_LIST).value<QVariantHomogenousList>();

	auto model_ptr = m_model.lock();
	Q_ASSERT(model_ptr);

	auto parent_id = getId();

	for(const QVariant& child : child_list)
	{
		qDb() << "READING CHILD ITEM INTO SRTMItem_LibEntry:" << child;

//		std::shared_ptr<AbstractTreeModelItem> new_child_item = model_ptr->make_item_from_variant(child);
//		bool ok = appendChild(new_child_item);
//		Q_ASSERT(ok);
		model_ptr->requestAddTreeModelItem(child, parent_id);
	}
#endif
}

//std::shared_ptr<ScanResultsTreeModel> SRTMItem_LibEntry::getTypedModel()
//{
//	return std::dynamic_pointer_cast<ScanResultsTreeModel>(m_model);
//}

/////////// @todo SRTMItem_LibEntry
