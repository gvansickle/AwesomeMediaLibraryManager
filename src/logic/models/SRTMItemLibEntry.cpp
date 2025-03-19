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
#include "ScanResultsTreeModel.h"


//std::shared_ptr<SRTMItem_LibEntry> SRTMItem_LibEntry::construct(std::shared_ptr<LibraryEntry> libentry,
//                                                                const std::shared_ptr<AbstractTreeModelItem>& parent_item, UUIncD id)
//{
//	std::shared_ptr<SRTMItem_LibEntry> self(new SRTMItem_LibEntry(libentry, parent_item, id));
//	self->postConstructorFinalization();
//	return self;
//}

//std::shared_ptr<SRTMItem_LibEntry> SRTMItem_LibEntry::construct(const QVariant& variant, const std::shared_ptr<AbstractTreeModelItem>& parent_item, UUIncD id)
//{
//	std::shared_ptr<SRTMItem_LibEntry> self(new SRTMItem_LibEntry(variant, parent_item, id));
//	self->postConstructorFinalization();
//	return self;
//}

SRTMItem_LibEntry::SRTMItem_LibEntry(const std::shared_ptr<AbstractTreeModel>& model, UUIncD id)
	: BASE_CLASS(model, id)
{

}

SRTMItem_LibEntry::SRTMItem_LibEntry(std::shared_ptr<LibraryEntry> libentry, const std::shared_ptr<AbstractTreeModel>& model, UUIncD id)
	: BASE_CLASS(model, id), m_library_entry(libentry)
{

}

//SRTMItem_LibEntry::SRTMItem_LibEntry(const QVariant& variant, const std::shared_ptr<AbstractTreeModelItem>& parent, UUIncD id)
//	: BASE_CLASS(parent, id)
//{
//
//}

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
using strviw_type = QLatin1String;

///// Strings to use for the tags.
#define X(field_tag, tag_string, var_name) static constexpr strviw_type field_tag ( # tag_string );
	M_DATASTREAM_FIELDS(X);
#undef X

QVariant SRTMItem_LibEntry::toVariant() const
{
	InsertionOrderedMap<QString, QVariant> map;

	// Overwrite any class info added by the above.
	set_map_class_info(this, &map);

	// Set the xml:id.
	map.insert_attributes({{"xml:id", get_prefixed_uuid()}});

	QVariantHomogenousList list(XMLTAG_LIBRARY_ENTRIES, "m_library_entry");
	if(auto libentry = m_library_entry.get(); libentry != nullptr)
	{
		list_push_back_or_die(list, m_library_entry->toVariant());
	}
	map_insert_or_die(map, XMLTAG_LIBRARY_ENTRIES, list);

    // Serialize the data members of the base class.
    QVariant base_class = static_cast<const BASE_CLASS*>(this)->BASE_CLASS::toVariant();

    map_insert_or_die(map, "baseclass", base_class);

	return map;
}

void SRTMItem_LibEntry::fromVariant(const QVariant& variant)
{
	InsertionOrderedMap<QString, QVariant> map = variant.value<InsertionOrderedMap<QString, QVariant>>();
//	dump_map(map);

	try
	{
		auto uuid = map.get_attr("xml:id");
		set_prefixed_uuid(uuid);
	}
	catch(...)
	{
		qWr() << "NO XML:ID:";
	}

#define X(field_tag, tag_string, var_name) map_read_field_or_warn(map, field_tag, var_name);
	M_DATASTREAM_FIELDS(X);
#undef X

	// Load LibraryEntry's.
	QVariantHomogenousList list(XMLTAG_LIBRARY_ENTRIES, "m_library_entry");
	map_read_field_or_warn(map, XMLTAG_LIBRARY_ENTRIES, &list);

	/// There should only be one I think....
	AMLM_ASSERT_EQ(list.size(), 1);

#if 0///
	append_children_from_variant<LibraryEntry>(this, list);
#else///old
	for(const QVariant& it : list)
	{
		/// @todo First doesn't work for some reason.

//		m_library_entry = std::make_shared<LibraryEntry>(it.value<LibraryEntry>());
		m_library_entry = std::make_shared<LibraryEntry>();
		m_library_entry->fromVariant(it);
	}
#endif

    // QVariantHomogenousList child_var_list(XMLTAG_CHILD_NODE_LIST, "child");
    // child_var_list = map.at(XMLTAG_CHILD_NODE_LIST).value<QVariantHomogenousList>();

    // append_children_from_variant(m_model, this, child_var_list);


#if 0///
	auto model_ptr_base = m_model.lock();
	Q_ASSERT(model_ptr_base);
	auto model_ptr = std::dynamic_pointer_cast<ScanResultsTreeModel>(model_ptr_base);
	auto parent_id = getId();

	/// NEEDS TO BE IN MODEL HERE.
	Q_ASSERT(isInModel());

	Q_ASSERT(child_list.size() == 0);
	for(const QVariant& child : child_list)
	{
		qDb() << "READING CHILD ITEM INTO SRTMItem_LibEntry:" << child;

//		bool ok = appendChild(new_child_item);
//		Q_ASSERT(ok);
		// WRONG: model_ptr->requestAddSRTMLibEntryItem(child, parent_id);
	}
#endif
}

//std::shared_ptr<ScanResultsTreeModel> SRTMItem_LibEntry::getTypedModel()
//{
//	return std::dynamic_pointer_cast<ScanResultsTreeModel>(m_model);
//}

/////////// @todo SRTMItem_LibEntry
