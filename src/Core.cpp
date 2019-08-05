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
 * @file Core.cpp
 */
#include <Core.h>

// Std C++
#include <memory>

// Ours
#include "AMLMApp.h"
#include <logic/models/ColumnSpec.h>
#include <logic/models/treeitem.h>


namespace AMLM
{

std::unique_ptr<Core> Core::m_self;

/// Private constructor.
Core::Core()
{
}

//void Core::prepareShutdown()
//{
//	m_guiConstructed = false;
//}

Core::~Core()
{
}

void Core::build()
{
	if(m_self)
	{
		return;
	}
	m_self.reset(new Core());

	// Create the single (at this point) ScanResultsTreeModel.
	/// @note In KDenLive, this is the same, no parent QObject given to ProjectItemModel::construct();
M_TODO("Improve ColumnSpecs, not sure I like how we do this and then need to erase it on a LoadModel().");
	std::initializer_list<ColumnSpec> column_specs = {ColumnSpec(SectionID(0), "DirProps"), {SectionID{0}, "MediaURL"}, {SectionID{0}, "SidecarCueURL"}};
	m_self->m_srtm_instance = std::make_shared<ScanResultsTreeModel>(column_specs, nullptr);
//	m_self->m_srtm_instance = std::make_shared<ScanResultsTreeModel>();
//	m_self->m_srtm_instance = ScanResultsTreeModel::construct({ColumnSpec(SectionID(0), "DirProps"), {SectionID{0}, "MediaURL"}, {SectionID{0}, "SidecarCueURL"}});
	// Create and set the root item / headers
	m_self->m_srtm_instance->setColumnSpecs({ColumnSpec(SectionID(0), "DirProps"), {SectionID{0}, "MediaURL"}, {SectionID{0}, "SidecarCueURL"}});
	// Let's add two more columns
	m_self->m_srtm_instance->insertColumns(3, 2);

	/// ETM
	QStringList headers;
	headers << tr("Title") << tr("Description");
	m_self->m_etm_instance = std::make_shared<TreeModel>(headers);

	auto root_item = m_self->getEditableTreeModel()->getItem(QModelIndex());
	std::shared_ptr<TreeItem> new_child = m_self->getEditableTreeModel()->insertChild();

	QVector<QVariant> fields({QString("ABC"), QString("DEF")});
//	std::shared_ptr<TreeItem> new_grandchild = std::make_shared<TreeItem>(fields, new_child);
//	/*auto new_grandchild =*/ new_child->insertChild(0, new_grandchild);
	auto new_grandchild = m_self->getEditableTreeModel()->append_child(fields, new_child);
	fields.clear();
	fields << QString("GHI") << QString("JKL");
	m_self->getEditableTreeModel()->append_child(fields, new_grandchild);

//	QVector<QVariant> fields2({QString("First"), QString("Second")});
//	auto new_unparented_child = std::make_shared<TreeItem>(fields2);


//	new_child->setData(0, fields[0]);
//	new_child->setData(1, fields[1]);
//	TreeItem* new_item = new TreeItem(fields, root_item);
//	root_item->insertChildren();

	/// @todo experimental
//	UUIncD new_id = m_self->m_srtm_instance->requestAddItem({"Artist1", "B", "C", "D"}, m_self->m_srtm_instance->getRootItem()->getId());
//	auto new_child_id_1 = m_self->m_srtm_instance->requestAddItem({"Album1", "F", "GHI", "J"}, new_id);
//	auto new_grandchild_id_1 = m_self->m_srtm_instance->requestAddItem({"Track1", "F", "GHI", "J"}, new_child_id_1);
//	auto new_child_id_2 = m_self->m_srtm_instance->requestAddItem({"Album2", "F", "GHI", "J"}, new_id);
//	auto new_grandchild_id_2 = m_self->m_srtm_instance->requestAddItem({"Track1", "F", "GHI", "J"}, new_child_id_2);
//
//	Q_ASSERT(m_self->m_srtm_instance->checkConsistency());
}

void Core::initGUI()
{
	/// @todo This is a KdenLive member.  Sets up and shows the GUI.  Not sure we need it, we have that going on in AMLMApp.
}

std::unique_ptr<Core>& Core::self()
{
	if (!m_self)
	{
		qDb() << "Error : Core has not been created";
		Q_ASSERT(0);
	}
	return m_self;
}

std::shared_ptr<ScanResultsTreeModel> Core::getScanResultsTreeModel()
{
	Q_CHECK_PTR(m_srtm_instance);
	return m_srtm_instance;
};

std::shared_ptr<TreeModel> Core::getEditableTreeModel()
{
	Q_CHECK_PTR(m_etm_instance);
	return m_etm_instance;
}


void Core::clean()
{
	m_self.reset();
}

} /* namespace AMLM */
