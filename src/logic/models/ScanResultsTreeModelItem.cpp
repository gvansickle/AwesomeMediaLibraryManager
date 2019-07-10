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

/**
 * @file ScanResultsTreeModelItem.cpp
 */

#include "ScanResultsTreeModelItem.h"

// Std C++
#include <memory>

// Qt5
#include <QXmlStreamReader>

// Ours
#include <utils/DebugHelpers.h>
#include <logic/DirScanResult.h>
#include "ScanResultsTreeModel.h"
#include <LibraryEntry.h>
#include <serialization/SerializationHelpers.h>

std::shared_ptr<ScanResultsTreeModelItem> ScanResultsTreeModelItem::construct(const DirScanResult& dsr, const std::shared_ptr<ScanResultsTreeModel> model, bool is_root)
{
	std::shared_ptr<ScanResultsTreeModelItem> self(new ScanResultsTreeModelItem(dsr, model, is_root));
	baseFinishConstruct(self);
	return self;
}

std::shared_ptr<ScanResultsTreeModelItem> ScanResultsTreeModelItem::construct(const QVariant& variant,
																			  std::shared_ptr<ScanResultsTreeModel> model, bool is_root)
{
	std::shared_ptr<ScanResultsTreeModelItem> self(new ScanResultsTreeModelItem(model, is_root));
	baseFinishConstruct(self);
	self->fromVariant(variant);

	return self;
}

ScanResultsTreeModelItem::ScanResultsTreeModelItem(const DirScanResult& dsr, const std::shared_ptr<ScanResultsTreeModel> model, bool is_root)
	: BASE_CLASS(model, is_root), m_dsr(dsr)
{
}

ScanResultsTreeModelItem::ScanResultsTreeModelItem(const std::shared_ptr<ScanResultsTreeModel> model, bool is_root)
	: BASE_CLASS(model, is_root)
{
}

ScanResultsTreeModelItem::~ScanResultsTreeModelItem()
{
}

QVariant ScanResultsTreeModelItem::data(int column, int role) const
{
	// Map column and role to the corresponding data.

	if((role != Qt::ItemDataRole::DisplayRole) && (role != Qt::ItemDataRole::EditRole))
	{
		return QVariant();
	}

	switch(column)
	{
	case 0:
		return toqstr(m_dsr.getDirProps());
	case 1:
		return QUrl(m_dsr.getMediaExtUrl());
	case 2:
		return QUrl(m_dsr.getSidecarCuesheetExtUrl());
	default:
		qWr() << "data() request for unknown column:" << column;
		return BASE_CLASS::data(column, role);
		break;
	}

	return QVariant();
}

int ScanResultsTreeModelItem::columnCount() const
{
	return 3;
}


#define M_DATASTREAM_FIELDS(X) \
	/* TAG_IDENTIFIER, tag_string, member_field, var_name */ \
	X(XMLTAG_DIRSCANRESULT, m_dsr, nullptr) \
	/*X(XMLTAG_NUM_COLUMNS, num_columns, (qulonglong)m_item_data.size())*/ \
	/*X(XMLTAG_ITEM_DATA_SIZE, item_data_size, (qulonglong)m_item_data.size())*/ \
	/*X(XMLTAG_NUM_CHILDREN, num_children, (qulonglong)m_child_items.size())*/ \
	X(XMLTAG_CHILD_NODE_LIST, child_node_list, nullptr)


/// Strings to use for the tags.
using strviw_type = QLatin1Literal;

///// Strings to use for the tags.
#define X(field_tag, tag_string, var_name) static const strviw_type field_tag ( # tag_string );
	M_DATASTREAM_FIELDS(X);
#undef X



QVariant ScanResultsTreeModelItem::toVariant() const
{
	QVariantInsertionOrderedMap map;

	// Defer to the base class for streaming out common data.
	map = BASE_CLASS::toVariant();

	// Overwrite any class info added by the above.
	set_map_class_info(this, &map);

	/// @todo Will be more fields, justifying the map vs. value?
	/// @todo Need the parent here too?  Probably needs to be handled by the parent, but maybe for error detection.

	map_insert_or_die(map, XMLTAG_DIRSCANRESULT, m_dsr);

#define X(field_tag, tag_string, var_name) map_insert_or_die(map, field_tag, var_name);
	M_DATASTREAM_FIELDS(X);
#undef X

	return map;
}

void ScanResultsTreeModelItem::fromVariant(const QVariant &variant)
{
	QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();

#define X(field_tag, tag_string, var_name) AMLM::map_read_field_or_warn(map, field_tag, var_name);
	M_DATASTREAM_FIELDS(X);
#undef X

	AMLM::map_read_field_or_warn(map, XMLTAG_DIRSCANRESULT, &m_dsr);

//	if(auto model = getTypedModel()) //std::static_pointer_cast<ScanResultsTreeModel>(m_model.lock()))
//	{
//		UUIncD parent_id;
//		if(auto parent_ptr = parent_item().lock())
//		{
//			parent_id = parent_ptr->getId();
//		}
//		else
//		{
//			// No parent.
//			Q_ASSERT(0);
//		}
////		model->requestAddExistingTreeModelItem(AbstractTreeModelItem::downcasted_shared_from_this<ScanResultsTreeModelItem>(), parent_id);
//	}

//	BASE_CLASS::fromVariant(variant);
	QVariantHomogenousList child_list = map.value(XMLTAG_CHILD_NODE_LIST).value<QVariantHomogenousList>();
	/// @todo HERE
//	childrenFromVariant(child_list);
//	qDb() << "READING CHILD ITEM INTO ScanResultsTreeModelItem:" << child->;
	auto model_ptr = getTypedModel();
//	std::shared_ptr<AbstractTreeModelItem> new_child_item = model_ptr->make_item_from_variant(map);
//	bool ok = appendChild(new_child_item);
//	Q_ASSERT(ok);

	// Overwrite any class info added by the above.
	set_map_class_info(this, &map);
}

std::shared_ptr<ScanResultsTreeModel> ScanResultsTreeModelItem::getTypedModel() const
{
	return std::dynamic_pointer_cast<ScanResultsTreeModel>(m_model.lock());
}

void ScanResultsTreeModelItem::setDirscanResults(const DirScanResult& dsr)
{
	m_dsr = dsr;
}


/////////// @todo SRTMItem_LibEntry

std::shared_ptr<SRTMItem_LibEntry> SRTMItem_LibEntry::construct(const DirScanResult& dsr, const std::shared_ptr<ScanResultsTreeModel>& model, bool is_root)
{
	std::shared_ptr<SRTMItem_LibEntry> self(new SRTMItem_LibEntry(dsr, model, is_root));
	baseFinishConstruct(self);
	return self;
}

std::shared_ptr<SRTMItem_LibEntry> SRTMItem_LibEntry::construct(const QVariant& variant, const std::shared_ptr<ScanResultsTreeModel>& model, bool is_root)
{
	std::shared_ptr<SRTMItem_LibEntry> self(new SRTMItem_LibEntry(model, is_root));
	self->fromVariant(variant);
	baseFinishConstruct(self);
	return self;
}

SRTMItem_LibEntry::SRTMItem_LibEntry(const DirScanResult& dsr, const std::shared_ptr<ScanResultsTreeModel>& model, bool is_root)
	: BASE_CLASS(dsr, std::static_pointer_cast<ScanResultsTreeModel>(model), is_root)
{

}

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

	// Defer to the base class for streaming out common data.
//	map = BASE_CLASS::toVariant();

	// Overwrite any class info added by the above.
	set_map_class_info(this, &map);

	map_insert_or_die(map, XMLTAG_CHILD_NODE_LIST, childrenToVariant());

	QVariantHomogenousList list(XMLTAG_LIBRARY_ENTRIES, "m_library_entry");

	if(auto libentry = m_library_entry.get(); libentry != nullptr)
	{
		list_push_back_or_die(list, m_library_entry->toVariant());
	}

	map_insert_or_die(map, XMLTAG_LIBRARY_ENTRIES, list);

	return map;
}

void SRTMItem_LibEntry::fromVariant(const QVariant& variant)
{
//	QVariantHomogenousList list = variant.value<QVariantHomogenousList>();

	QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();

#define X(field_tag, tag_string, var_name) AMLM::map_read_field_or_warn(map, field_tag, var_name);
	M_DATASTREAM_FIELDS(X);
#undef X

	AMLM::map_read_field_or_warn(map, XMLTAG_LIBRARY_ENTRIES, &m_library_entry);

//	BASE_CLASS::fromVariant(variant);

}

//std::shared_ptr<ScanResultsTreeModel> SRTMItem_LibEntry::getTypedModel()
//{
//	return std::dynamic_pointer_cast<ScanResultsTreeModel>(m_model);
//}

/////////// @todo SRTMItem_LibEntry
