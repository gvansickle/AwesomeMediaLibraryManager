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
#include "AbstractTreeModelHeaderItem.h"
#include "ScanResultsTreeModel.h"
#include <LibraryEntry.h>


std::shared_ptr<ScanResultsTreeModelItem> ScanResultsTreeModelItem::construct(const DirScanResult& dsr, const std::shared_ptr<ScanResultsTreeModel> model, bool is_root)
{
	std::shared_ptr<ScanResultsTreeModelItem> self(new ScanResultsTreeModelItem(dsr, model, is_root));
	baseFinishConstruct(self);
	return self;
}

ScanResultsTreeModelItem::ScanResultsTreeModelItem(const DirScanResult& dsr, const std::shared_ptr<ScanResultsTreeModel> model, bool is_root)
	: BASE_CLASS({}, model, is_root), m_dsr(dsr)
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
		break;
	}

	return QVariant();
}

int ScanResultsTreeModelItem::columnCount() const
{
	return 3;
}

std::shared_ptr<AbstractTreeModelHeaderItem> ScanResultsTreeModelItem::parent() const
{
	return std::static_pointer_cast<AbstractTreeModelHeaderItem>(m_parent_item.lock());
}

using strviw_type = QLatin1Literal;

#define M_DATASTREAM_FIELDS(X) \
	X(XMLTAG_DIRSCANRESULT, m_dsr)

/// Strings to use for the tags.
#define X(field_tag, member_field) static const strviw_type field_tag ( # member_field );
	M_DATASTREAM_FIELDS(X);
#undef X

QVariant ScanResultsTreeModelItem::toVariant() const
{
	QVariantInsertionOrderedMap map;

	/// @todo Will be more fields, justifying the map vs. value?
	/// @todo Need the parent here too?  Probably needs to be handled by the parent, but maybe for error detection.

#define X(field_tag, member_field) map_insert_or_die(map, field_tag, member_field);
	M_DATASTREAM_FIELDS(X);
#undef X

	// Children to variant list.
	QVariantHomogenousList vl("children", "child");
	for(int i=0; i<childCount(); i++)
	{
		auto child_ptr = child(i);
//		list_push_back_or_die(vl, child_ptr);
		vl.push_back(child_ptr->toVariant());
	}
	map.insert("children", vl);

	return map;
}

void ScanResultsTreeModelItem::fromVariant(const QVariant &variant)
{
	QVariantInsertionOrderedMap map = variant.value<QVariantInsertionOrderedMap>();

#define X(field_tag, member_field) map_read_field_or_warn(map, field_tag, &(member_field));
	M_DATASTREAM_FIELDS(X);
#undef X

	// Children to variant list.
	QVariantHomogenousList vl("children", "child");
	map_read_field_or_warn(map, "children", &vl);
//	for(const auto& ch : vl)
//	{
//		appendChild(ch.fromVariant());
	//	}
}

/// AbsProjItem
void ScanResultsTreeModelItem::updateParent(std::shared_ptr<AbstractTreeModelItem> new_parent)
{
	m_last_parent_uuincd = UUIncD::null();
	if(new_parent)
	{
		m_last_parent_uuincd = new_parent->getId();
	}
	BASE_CLASS::updateParent(new_parent);
}

void ScanResultsTreeModelItem::setDirscanResults(const DirScanResult& dsr)
{
	m_dsr = dsr;
}


/////////// @todo SRTMItem_LibEntry

std::shared_ptr<SRTMItem_LibEntry> SRTMItem_LibEntry::construct(const std::shared_ptr<LibraryEntry>& libentry,
		const DirScanResult& dsr, const std::shared_ptr<ScanResultsTreeModel>& model)
{
	std::shared_ptr<SRTMItem_LibEntry> self(new SRTMItem_LibEntry(libentry, dsr, model, false));
	baseFinishConstruct(self);
	return self;
}

SRTMItem_LibEntry::SRTMItem_LibEntry(const std::shared_ptr<LibraryEntry>& libentry, const DirScanResult& dsr,
		const std::shared_ptr<ScanResultsTreeModel>& model, bool is_root)
	: BASE_CLASS(dsr, model, is_root)
{
	m_library_entry = libentry;
}

QVariant SRTMItem_LibEntry::data(int column, int role) const
{
	if((role != Qt::ItemDataRole::DisplayRole) && (role != Qt::ItemDataRole::EditRole))
	{
		return QVariant();
	}
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
}

int SRTMItem_LibEntry::columnCount() const
{
	return 2;
}

QVariant SRTMItem_LibEntry::toVariant() const
{
	QVariantHomogenousList list("library_entries", "m_library_entry");

	/// @todo Need the parent here too?  Probably needs to be handled by the parent, but maybe for error detection.

	if(auto libentry = m_library_entry.get(); libentry != nullptr)
	{
		list_push_back_or_die(list, m_library_entry->toVariant());
	}

	return list;
}

void SRTMItem_LibEntry::fromVariant(const QVariant& variant)
{
	QVariantHomogenousList list = variant.value<QVariantHomogenousList>();

	/// @todo Incomplete.
	Q_ASSERT(0);
}

/////////// @todo SRTMItem_LibEntry
