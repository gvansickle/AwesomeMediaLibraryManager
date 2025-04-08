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

SRTMItem_LibEntry::SRTMItem_LibEntry(const std::shared_ptr<AbstractTreeModel>& model)
    : BASE_CLASS(model)
{

}

SRTMItem_LibEntry::SRTMItem_LibEntry(std::shared_ptr<LibraryEntry> libentry, const std::shared_ptr<AbstractTreeModel>& model)
    : BASE_CLASS(model), m_library_entry(libentry)
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

#define M_DATASTREAM_FIELDS(X) \
	/* TAG_IDENTIFIER, tag_string, member_field, var_name */ \
	X(XMLTAG_LIBRARY_ENTRIES, library_entries, nullptr) \
	X(XMLTAG_CHILD_ITEM_MAP, child_item_map, nullptr) \
	X(XMLTAG_ITEM_DATA_LIST, item_data_list, nullptr)

#define M_DATASTREAM_FIELDS_CONTSIZES(X) \
	X(XMLTAG_NUM_COLUMNS, num_columns, m_item_data) \
	X(XMLTAG_ITEM_DATA_LIST_SIZE, item_data_list_size, m_item_data) \
	X(XMLTAG_NUM_CHILDREN, num_children, m_child_items)

/// Strings to use for the tags.
using strviw_type = QLatin1String;

///// Strings to use for the tags.
#define X(field_tag, tag_string, var_name) static constexpr strviw_type field_tag ( # tag_string );
	M_DATASTREAM_FIELDS(X);
	M_DATASTREAM_FIELDS_CONTSIZES(X);
#undef X

QVariant SRTMItem_LibEntry::toVariant() const
{
	InsertionOrderedMap<QString, QVariant> map;

	// Overwrite any class info added by the above.
	set_map_class_info(this, &map);

	// Set the xml:id.
	map.insert_attributes({{"xml:id", get_prefixed_uuid()}});

#define X(field_tag, tag_string, var_name) map_insert_or_die(map, field_tag, var_name);
	M_DATASTREAM_FIELDS(X);
#undef X
#define X(field_tag, tag_string, var_name) map_insert_or_die(map, field_tag, (qulonglong)(var_name).size());
	M_DATASTREAM_FIELDS_CONTSIZES(X);
#undef X

	// Add the m_item_data QVariants to the map under key XMLTAG_ITEM_DATA_LIST/"<item_data_list>".
	item_data_to_variant(&map);

	// Insert the LibraryEntry's.
	/// @todo Currently there's only one entry.
	QVariantHomogenousList libentrylist(XMLTAG_LIBRARY_ENTRIES, "m_library_entry");
	if(auto libentry = m_library_entry.get(); libentry != nullptr)
	{
		list_push_back_or_die(libentrylist, m_library_entry->toVariant());
	}
	map_insert_or_die(map, XMLTAG_LIBRARY_ENTRIES, libentrylist);

    // Serialize the data members of the base class.
    QVariant base_class = this->BASE_CLASS::toVariant();

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

// #define X(field_tag, tag_string, var_name) map_read_field_or_warn(map, field_tag, var_name);
// 	M_DATASTREAM_FIELDS(X);
// #undef X

	// Load LibraryEntry's.
	QVariantHomogenousList list(XMLTAG_LIBRARY_ENTRIES, "m_library_entry");
	map_read_field_or_warn(map, XMLTAG_LIBRARY_ENTRIES, &list);

    // There should only be one I think....
	AMLM_ASSERT_EQ(list.size(), 1);

	for(const QVariant& it : list)
	{
		/// @todo First doesn't work for some reason.

//		m_library_entry = std::make_shared<LibraryEntry>(it.value<LibraryEntry>());
		m_library_entry = std::make_shared<LibraryEntry>();
		m_library_entry->fromVariant(it);
	}


    // Deserialize the data members of the base class.
    // Once we get up to the AbstractTreeModelItem base class, this includes child items.
    auto iomap {InsertionOrderedMap<QString, QVariant>()};
    map_read_field_or_warn(map, "baseclass", &iomap);
    Q_ASSERT(m_model.lock());
    this->BASE_CLASS::fromVariant(iomap);

#endif

    // QVariantHomogenousList child_var_list(XMLTAG_CHILD_NODE_LIST, "child");
    // child_var_list = map.at(XMLTAG_CHILD_NODE_LIST).value<QVariantHomogenousList>();

    // append_children_from_variant(m_model, this, child_var_list);

	// Get this item's children.
	qulonglong num_children = 0;
	map_read_field_or_warn(map, XMLTAG_NUM_CHILDREN, &num_children);

	qDb() << XMLTAG_NUM_CHILDREN << num_children;

	// Now read in our children.
	// We need this Item to be in a model for that to work.
	/// @todo Add to model, Will this finally work?
//	auto parent_item_ptr = this->parent_item().lock();
//WRONG:	appendChild(this->shared_from_this());
	Q_ASSERT(isInModel());
	auto model_ptr = m_model.lock();
	Q_ASSERT(model_ptr);

	InsertionOrderedStrVarMap child_map;// (XMLTAG_CHILD_ITEM_MAP, "child");
	child_map = map.value(XMLTAG_CHILD_ITEM_MAP).value<InsertionOrderedStrVarMap>();
	qDb() << M_ID_VAL(child_map.size());

	AMLM_ASSERT_EQ(num_children, child_map.size());

	if(num_children > 0)
	{
		children_from_str_var_map(child_map);
	}

	qDb() << M_ID_VAL(child_map.size());
}
