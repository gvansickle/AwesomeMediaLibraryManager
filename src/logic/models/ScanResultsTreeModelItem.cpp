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

// Qt5
#include <QXmlStreamReader>

// Ours
#include <utils/DebugHelpers.h>
#include <logic/DirScanResult.h>
#include "ScanResultsTreeModelXMLTags.h"


ScanResultsTreeModelItem::ScanResultsTreeModelItem(const DirScanResult& dsr, AbstractTreeModelItem* parent)
	: AbstractTreeModelItem(parent)
{
	m_dsr = dsr;
}

ScanResultsTreeModelItem::~ScanResultsTreeModelItem()
{
}

QVariant ScanResultsTreeModelItem::data(int column) const
{
	/// Map column and @todo role to the corresponding data.

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


QVariant ScanResultsTreeModelItem::toVariant() const
{
	QVariantMap map;

	/// @todo Will be more fields, justifying the map vs. value?
	/// @todo Need the parent here too?  Probably needs to be handled by the parent, but maybe for error detection.

	map.insert(SRTMItemTagToXMLTagMap[SRTMItemTag::DIRSCANRESULT], m_dsr.toVariant());

	return map;
}

void ScanResultsTreeModelItem::fromVariant(const QVariant &variant)
{
	QVariantMap map = variant.toMap();

M_MESSAGE("The cast should work though, right?");
//	m_dsr = map.value("dirscanresult").value<DirScanResult>();

	auto dsr_in_variant = map.value(SRTMItemTagToXMLTagMap[SRTMItemTag::DIRSCANRESULT]);
	m_dsr.fromVariant(dsr_in_variant);
}

ScanResultsTreeModelItem *
ScanResultsTreeModelItem::create_default_constructed_child_item(AbstractTreeModelItem *parent)
{
	ScanResultsTreeModelItem* child_item;

	child_item = new ScanResultsTreeModelItem(parent);

	return child_item;
}

