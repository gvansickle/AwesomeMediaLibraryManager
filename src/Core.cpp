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
#include <vector>

// Ours
#include "AMLMApp.h"
#include <logic/models/ColumnSpec.h>
#include <logic/models/treeitem.h>
#include <logic/LibraryRescanner.h>


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
	qDb() << "Deleting Core singleton";
}

void Core::build()
{
	// If this singleton has already been built, simply return.
	if(m_self)
	{
		return;
	}
	// Else create the singleton.
	m_self.reset(new Core());

	/// @note KDEN does this at this point:
	/// - Calls to qRegisterMetaType()
	/// - (N/A) If AppImage, resets some paths.  Else opens a connection to MLT.
	/// - (N/A?) Loads "profiles", which looks like an MLT-specific thing.
	/// - (N/A?) Initializes a default unavailable media "producer".
	/// - Constructs the ProjectItemModel (with no QObject parent).
	/// - Constructs a new JobManager (with m_self as parent).
	/// - returns.

	// Create the Database asynchronous rescanner.
	m_self->m_db_scan_job = std::make_shared<DatabaseScanJob>();


	// Create the single (at this point) ScanResultsTreeModel.
	/// @note In KDenLive, this is the same, no parent QObject given to ProjectItemModel::construct();
M_TODO("Improve ColumnSpecs, not sure I like how we do this and then need to erase it on a LoadModel().");
	std::vector<ColumnSpec> column_specs = {ColumnSpec(SectionID(0), "DirProps"), {SectionID{0}, "MediaURL"}, {SectionID{0}, "SidecarCueURL"}};
	m_self->m_srtm_instance = ScanResultsTreeModel::make_ScanResultsTreeModel({ColumnSpec(SectionID(0), "DirProps"), {SectionID{0}, "MediaURL"}, {SectionID{0}, "SidecarCueURL"}});
	// Create and set the root item / headers
//	m_self->m_srtm_instance->setColumnSpecs({ColumnSpec(SectionID(0), "DirProps"), {SectionID{0}, "MediaURL"}, {SectionID{0}, "SidecarCueURL"}});
	// Let's add two more columns
	m_self->m_srtm_instance->insertColumns(3, 2);

	Q_ASSERT(m_self->m_srtm_instance->checkConsistency());


	/// ETM
	QStringList headers;
	headers << tr("Title") << tr("Description");
	m_self->m_etm_instance = std::make_shared<TreeModel>(headers);
	auto the_etm = m_self->m_etm_instance;

	auto root_item = the_etm->getItem(QModelIndex());
	std::shared_ptr<TreeItem> new_child = the_etm->insertChild();

	QVector<QVariant> fields({QString("ABC"), QString("DEF")});
//	std::shared_ptr<TreeItem> new_grandchild = std::make_shared<TreeItem>(fields, new_child);
//	/*auto new_grandchild =*/ new_child->insertChild(0, new_grandchild);
	auto new_grandchild = the_etm->append_child(fields, new_child);
	fields.clear();
	fields << QString("GHI") << QString("JKL");
	the_etm->append_child(fields, new_grandchild);

	QVector<QVariant> fields2({QString("First"), QString("Second")});
	auto new_unparented_child = std::make_shared<TreeItem>(fields2);


	/// ATM
	/// std::shared_ptr<AbstractTreeModel> m_atm_instance;
	/// experimental
	{
		std::vector<ColumnSpec> column_specs = {ColumnSpec(SectionID(0), "Tag"), {SectionID{0}, "Value"}, {SectionID{0}, "Type"}, {SectionID{0}, "Class"}};
		m_self->m_atm_instance = AbstractTreeModel::make_AbstractTreeModel(column_specs);

		//	new_child->setData(0, fields[0]);
		//	new_child->setData(1, fields[1]);
		//	TreeItem* new_item = new TreeItem(fields, root_item);
		//	root_item->insertChildren();

		std::vector<QVariant> fields_atm{QString("First"), QString("Second")};
		auto new_child_id_1 = std::make_shared<AbstractTreeModelItem>(fields_atm);
		m_self->m_atm_instance->getRootItem()->appendChild(new_child_id_1);
//	UUIncD new_id = m_self->m_atm_instance->requestAddItem({"Artist1", "B", "C", "D"}, m_self->m_srtm_instance->getRootItem()->getId());
//	auto new_child_id_1 = m_self->m_atm_instance->requestAddItem({"Album1", "F", "GHI", "J"}, new_id);
		auto new_child_id_2 = std::make_shared<AbstractTreeModelItem>(std::vector<QVariant>{"Album2", "F", "GHI", "J"});

		m_self->m_atm_instance->getRootItem()->appendChild(new_child_id_2);
		auto new_grandchild_id_1 = std::make_shared<AbstractTreeModelItem>(std::vector<QVariant>{{"Track1"}, {"F"}, {"GHI"}, {"J"}});
		new_child_id_1->appendChild(new_grandchild_id_1);

//		auto new_child_id_2 = m_self->m_atm_instance->requestAddItem({"Album2", "F", "GHI", "J"}, new_id);
		auto new_grandchild_id_2 = std::make_shared<AbstractTreeModelItem>(std::vector<QVariant>{"Track2", "K", "LMN", "OP"});
		new_child_id_2->appendChild(new_grandchild_id_2);

		Q_ASSERT(m_self->m_atm_instance->checkConsistency());

		m_self->m_atm_instance->SaveDatabase("/home/gary/AMLM_Exp.xml");

		// Try to reload
		auto loaded_atm = AbstractTreeModel::make_AbstractTreeModel(column_specs);
		bool succeeded = loaded_atm->LoadDatabase("/home/gary/AMLM_Exp.xml");
		Q_ASSERT(succeeded);
		loaded_atm->SaveDatabase("/home/gary/AMLM_Exp_RT.xml");

	}
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

std::shared_ptr<AbstractTreeModel> Core::getAbstractTreeModel()
{
	Q_CHECK_PTR(m_atm_instance);

	return std::dynamic_pointer_cast<AbstractTreeModel>(m_atm_instance);
}

std::shared_ptr<ScanResultsTreeModel> Core::getScanResultsTreeModel()
{
	Q_CHECK_PTR(m_srtm_instance);

	return std::dynamic_pointer_cast<ScanResultsTreeModel>(m_srtm_instance);
}

#if 1///
std::shared_ptr<ScanResultsTreeModel> Core::swapScanResultsTreeModel(const std::shared_ptr<ScanResultsTreeModel>& new_model)
{
//	AMLM_ASSERT_X(0, "TODO");
	auto old_model = m_srtm_instance;
//	old_model->beginResetModel();

	m_srtm_instance = new_model;

	return old_model;
}
#endif

std::vector<ColumnSpec> Core::getDefaultColumnSpecs()
{
	std::vector<ColumnSpec> column_specs = {ColumnSpec(SectionID(0), "DirProps"), {SectionID{0}, "MediaURL"}, {SectionID{0}, "SidecarCueURL"}};
	return column_specs;
};

std::shared_ptr<TreeModel> Core::getEditableTreeModel()
{
	Q_CHECK_PTR(m_etm_instance);
	return m_etm_instance;
}

std::shared_ptr<DatabaseScanJob> Core::getDatabaseRescanner()
{
	Q_CHECK_PTR(m_db_scan_job);
	return m_db_scan_job;
}

void Core::clean()
{
	m_self.reset();
}



} /* namespace AMLM */
